/*
*
*                   _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
*                  _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
*                 _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
*                _/      _/    _/ _/    _/ _/   _/ _/    _/
*               _/      _/    _/ _/    _/ _/_/_/  _/    _/
*
*             ***********************************************
*                              PandA Project
*                     URL: http://panda.dei.polimi.it
*                       Politecnico di Milano - DEIB
*                        System Architectures Group
*             ***********************************************
*              Copyright(c) 2018 Politecnico di Milano
*
*   This file is part of the PandA framework.
*
*   The PandA framework is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 3 of the License, or
*  (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*/
/**
* @file HardekopfLin_AA.cpp
* @brief This is an implementation of Andersen's inclusion-based pointer
*        analysis (interprocedural, flow-insensitive, context-insensitive, and
*        field-sensitive). Part of the code comes from a LLVM 2.5 implementation
*        done by Andrey Petrov and Ben Hardekopf.
*        The Andersen specific version used here is described in:
*        "The Ant and the Grasshopper: Fast and Accurate Pointer Analysis for
*        Millions of Lines of Code", by Ben Hardekopf & Calvin Lin, in PLDI 2007.
*        To fulfil the sublicense requirements here it is the copyright permission
*        notice:
 *------------------------------------------------------------------------------
 *  Copyright 2008 Andrey Petrov, Ben Hardekopf
 *  - apetrov87@gmail.com, apetrov@cs.utexas.edu
 *  - benh@cs.utexas.edu
 *
 *  Permission is hereby granted, free of charge, to any person
 *  obtaining a copy of this software and associated documentation
 *  files (the "Software"), to deal in the Software without
 *  restriction, including without limitation the rights to use, copy,
 *  modify, merge, publish, distribute, sublicense, and/or sell copies
 *  of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be
 *  included in all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 *  HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 *  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *  DEALINGS IN THE SOFTWARE.
 *------------------------------------------------------------------------------
*
* @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
*
*/

#include "HardekopfLin_AA.hpp"

#include "llvm/IR/Constants.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/GetElementPtrTypeIterator.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/ModuleSlotTracker.h"
#include "llvm/Support/Casting.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Analysis/MemoryBuiltins.h"

#include "boost/range/irange.hpp"
#include <queue>

#include "bvec.h"

namespace std
{
   template <> struct hash<bitmap>
   {
         typedef bitmap argument_type;
         typedef std::size_t result_type;

         result_type operator()(const argument_type &Arg) const
         {
            result_type res=0;
            for(auto el: Arg)
            {
               res ^= el*7;
            }
            return std::hash<result_type>()(res);
         }
   };
} // end namespace std


namespace llvm
{
   template<> struct DenseMapInfo<std::pair<u32, u32> >
   {
         static std::pair<u32, u32> getEmptyKey()
         {
            return std::make_pair(0xffefffff, 0xffefffff);
         }
         static std::pair<u32, u32> getTombstoneKey()
         {
            return std::make_pair(0xffeffffe, 0xffeffffe);
         }
         static unsigned getHashValue(const std::pair<u32, u32> &X)
         {
            return (X.first<<16) ^ (X.first>>16) ^ X.second;
         }
         static unsigned isEqual(const std::pair<u32, u32> &X, const std::pair<u32, u32> &Y)
         {
            return X == Y;
         }
   };
}



//Check for constraints with "undefined" sources?
#define CHECK_CONS_UNDEF 1

//LCD runs when there are at least (lcd_size) candidates for cycle detection,
//  or if it has not run for (lcd_period) solve_node runs.
const u32 lcd_size= 600, lcd_period= 999999999;

//The offsets from a function's obj node to the return value and first arg.
const u32 func_node_off_ret= 1, func_node_off_arg0= 2;
//The starting union-find rank of any node.
const u32 node_rank_min= 0xf0000000;
//The max. number of entries in bv_cache, and how many to remove at once
//  when it gets full (which may be faster than one at a time).
const u32 bvc_sz= 5000000, bvc_remove= 10000;


//Special node IDs: 0 - no node, i2p - unknown target of pointers cast from int,
//  p_i2p - constant ptr to i2p,
//  first_var_node - the 1st node representing a real variable.
static const u32 i2p= 1, p_i2p= 2, first_var_node= 3;

static std::set<const llvm::BasicBlock*> bb_seen; // for processBlock()

static std::vector<u32>* bdd2vec(bdd x);
static void clear_bdd2vec();


//------------------------------------------------------------------------------
//A node for the offline constraint graph.
//This graph has VAL nodes for the top-level pointer variables (aka value
//  nodes) and AFP nodes for the parameters and return values of address-taken
//  functions (which are object nodes in the main graph but are used as normal
//  variables within the function). There is also a corresponding
//  REF node for the dereference (*p) of any VAL/AFP node (p).
class OffNode
{
   public:
      //Outgoing constraint edges: X -> Y for any cons. X = Y (where X/Y may
      //  be VAR or REF nodes).
      bitmap edges;
      //Outgoing implicit edges:
      //  any cons. edge X -> Y has a corresponding impl. edge *X -> *Y.
      bitmap impl_edges;
      //The union-find rank of this node if >= node_rank_min, else the number of
      //  another node in the same SCC.
      u32 rep;
      //The set of pointer-equivalence labels (singleton for HVN, any size for HU).
      //  This is empty for unlabeled nodes, and contains 0 for non-pointers.
      bitmap ptr_eq;
      //The node of the main graph corresponding to us (for HCD).
      u32 main_node;
      //The number of the DFS call that first visited us (0 if unvisited).
      u32 dfs_id;
      //True if this is the root of an already processed SCC.
      bool scc_root;
      //A VAL node is indirect if it's the LHS of a load+offset constraint;
      //  the LHS of addr_of and GEP are pre-labeled when building the graph
      //  and so don't need another unique label.
      //All AFP and REF nodes are indirect.
      bool indirect;

      OffNode(bool ind= 0) : rep(node_rank_min), dfs_id(0), scc_root(0),
         indirect(ind) {}

      bool is_rep() const{
         return rep >= node_rank_min;
      }
};

//The nodes of the offline graph: | null | AFP | VAL | REF |.
static std::vector<OffNode> off_nodes;
//The offline node corresponding to each node of the main graph (0 = none).
static std::vector<u32> main2off;
//The number of each type of node and the index of the first of each type.
static u32 nVAL, nAFP, nREF, firstVAL, firstAFP, firstREF;
//Use REF(p) to get the REF node for any VAL/AFP node (p).
#define REF(p) ((p)-firstAFP+firstREF)
//The ptr_eq label to use next.
static u32 next_ptr_eq;
//The number of the current DFS.
static u32 curr_dfs;
//The non-root nodes in the current SCC.
static std::stack<u32> dfs_stk;
//The ptr_eq for each set of incoming labels (HVN only).
static std::unordered_map<bitmap, u32> lbl2pe;
//The RHS of any GEP constraint is mapped to a label.
static llvm::DenseMap<std::pair<u32, u32>, u32> gep2pe;


struct ei_pair
{
      const char *n;
      extf_t t;
};

//Each (name, type) pair will be inserted into the map.
//All entries of the same type must occur together (for error detection).
static const ei_pair ei_pairs[]=
{
   //The current llvm-gcc puts in the \01.
   {"\01creat64", EFT_NOOP},
   {"\01fseeko64", EFT_NOOP},
   {"\01fstat64", EFT_NOOP},
   {"\01fstatvfs64", EFT_NOOP},
   {"\01ftello64", EFT_NOOP},
   {"\01getrlimit64", EFT_NOOP},
   {"\01lstat64", EFT_NOOP},
   {"\01open64", EFT_NOOP},
   {"\01stat64", EFT_NOOP},
   {"\01statvfs64", EFT_NOOP},
   {"Gpm_GetEvent", EFT_NOOP},
   {"Gpm_Open", EFT_NOOP},
   {"RAND_seed", EFT_NOOP},
   {"SSL_CTX_free", EFT_NOOP},
   {"SSL_CTX_set_default_verify_paths", EFT_NOOP},
   {"SSL_free", EFT_NOOP},
   {"SSL_get_error", EFT_NOOP},
   {"SSL_get_fd", EFT_NOOP},
   {"SSL_pending", EFT_NOOP},
   {"SSL_read", EFT_NOOP},
   {"SSL_set_bio", EFT_NOOP},
   {"SSL_set_connect_state", EFT_NOOP},
   {"SSL_shutdown", EFT_NOOP},
   {"SSL_state", EFT_NOOP},
   {"SSL_write", EFT_NOOP},
   {"Void_FreeCore", EFT_NOOP},
   {"X509_STORE_CTX_get_error", EFT_NOOP},
   {"XAllocColor", EFT_NOOP},
   {"XCloseDisplay", EFT_NOOP},
   {"XCopyArea", EFT_NOOP},
   {"XCreateColormap", EFT_NOOP},
   {"XCreatePixmap", EFT_NOOP},
   {"XCreateWindow", EFT_NOOP},
   {"XDrawPoint", EFT_NOOP},
   {"XDrawString", EFT_NOOP},
   {"XDrawText", EFT_NOOP},
   {"XFillRectangle", EFT_NOOP},
   {"XFillRectangles", EFT_NOOP},
   {"XFree", EFT_NOOP},
   {"XFreeColormap", EFT_NOOP},
   {"XFreeColors", EFT_NOOP},
   {"XFreeFont", EFT_NOOP},
   {"XFreeFontNames", EFT_NOOP},
   {"XFreeGC", EFT_NOOP},
   {"XFreePixmap", EFT_NOOP},
   {"XGetGCValues", EFT_NOOP},
   {"XGetGeometry", EFT_NOOP},
   {"XInternAtom", EFT_NOOP},
   {"XMapWindow", EFT_NOOP},
   {"XNextEvent", EFT_NOOP},
   {"XPutImage", EFT_NOOP},
   {"XQueryColor", EFT_NOOP},
   {"XResizeWindow", EFT_NOOP},
   {"XSelectInput", EFT_NOOP},
   {"XSendEvent", EFT_NOOP},
   {"XSetBackground", EFT_NOOP},
   {"XSetClipMask", EFT_NOOP},
   {"XSetClipOrigin", EFT_NOOP},
   {"XSetFillStyle", EFT_NOOP},
   {"XSetFont", EFT_NOOP},
   {"XSetForeground", EFT_NOOP},
   {"XSetFunction", EFT_NOOP},
   {"XSetGraphicsExposures", EFT_NOOP},
   {"XSetLineAttributes", EFT_NOOP},
   {"XSetTile", EFT_NOOP},
   {"XSetWMHints", EFT_NOOP},
   {"XSetWMNormalHints", EFT_NOOP},
   {"XSetWindowBackgroundPixmap", EFT_NOOP},
   {"XStoreName", EFT_NOOP},
   {"XSync", EFT_NOOP},
   {"XVisualIDFromVisual", EFT_NOOP},
   {"XWMGeometry", EFT_NOOP},
   {"XtAppSetFallbackResources", EFT_NOOP},
   {"XtCloseDisplay", EFT_NOOP},
   {"XtDestroyApplicationContext", EFT_NOOP},
   {"XtDestroyWidget", EFT_NOOP},
   {"_IO_getc", EFT_NOOP},
   {"_IO_putc", EFT_NOOP},
   {"__assert_fail", EFT_NOOP},
   {"__dn_expand", EFT_NOOP},
   {"__dn_skipname", EFT_NOOP},
   {"__res_nclose", EFT_NOOP},
   {"__res_ninit", EFT_NOOP},
   {"__res_nmkquery", EFT_NOOP},
   {"__res_nsend", EFT_NOOP},
   {"__res_query", EFT_NOOP},
   {"__res_querydomain", EFT_NOOP},
   {"__res_search", EFT_NOOP},
   {"__sigsetjmp", EFT_NOOP},
   {"_obstack_begin", EFT_NOOP},
   {"_obstack_memory_used", EFT_NOOP},
   {"_obstack_newchunk", EFT_NOOP},
   {"_setjmp", EFT_NOOP},
   {"accept", EFT_NOOP},
   {"access", EFT_NOOP},
   {"asprintf", EFT_NOOP},
   {"atexit", EFT_NOOP},
   {"atof", EFT_NOOP},
   {"atoi", EFT_NOOP},
   {"atol", EFT_NOOP},
   {"bind", EFT_NOOP},
   {"cfgetospeed", EFT_NOOP},
   {"cfree", EFT_NOOP},
   {"cfsetispeed", EFT_NOOP},
   {"cfsetospeed", EFT_NOOP},
   {"chdir", EFT_NOOP},
   {"chmod", EFT_NOOP},
   {"chown", EFT_NOOP},
   {"chroot", EFT_NOOP},
   {"clearerr", EFT_NOOP},
   {"clearok", EFT_NOOP},
   {"closedir", EFT_NOOP},
   {"compress2", EFT_NOOP},
   {"confstr", EFT_NOOP},
   {"connect", EFT_NOOP},
   {"crc32", EFT_NOOP},
   {"creat", EFT_NOOP},
   {"creat64", EFT_NOOP},
   {"deflate", EFT_NOOP},
   {"deflateEnd", EFT_NOOP},
   {"deflateInit2_", EFT_NOOP},
   {"deflateReset", EFT_NOOP},
   {"delwin", EFT_NOOP},
   {"dladdr", EFT_NOOP},
   {"dlclose", EFT_NOOP},
   {"execl", EFT_NOOP},
   {"execle", EFT_NOOP},
   {"execlp", EFT_NOOP},
   {"execv", EFT_NOOP},
   {"execve", EFT_NOOP},
   {"execvp", EFT_NOOP},
   {"fclose", EFT_NOOP},
   {"feof", EFT_NOOP},
   {"ferror", EFT_NOOP},
   {"fflush", EFT_NOOP},
   {"fgetc", EFT_NOOP},
   {"fgetpos", EFT_NOOP},
   {"fileno", EFT_NOOP},
   {"flockfile", EFT_NOOP},
   {"fnmatch", EFT_NOOP},
   {"forkpty", EFT_NOOP},
   {"fprintf", EFT_NOOP},
   {"fputc", EFT_NOOP},
   {"fputs", EFT_NOOP},
   {"fread", EFT_NOOP},
   {"free", EFT_NOOP},
   {"free_all_mem", EFT_NOOP},
   {"freeaddrinfo", EFT_NOOP},
   {"frexp", EFT_NOOP},
   {"fscanf", EFT_NOOP},
   {"fseek", EFT_NOOP},
   {"fseeko", EFT_NOOP},
   {"fseeko64", EFT_NOOP},
   {"fsetpos", EFT_NOOP},
   {"fstat", EFT_NOOP},
   {"fstat64", EFT_NOOP},
   {"fstatfs", EFT_NOOP},
   {"fstatvfs64", EFT_NOOP},
   {"ftell", EFT_NOOP},
   {"ftello", EFT_NOOP},
   {"ftello64", EFT_NOOP},
   {"ftok", EFT_NOOP},
   {"funlockfile", EFT_NOOP},
   {"fwrite", EFT_NOOP},
   {"g_scanner_destroy", EFT_NOOP},
   {"g_scanner_eof", EFT_NOOP},
   {"g_scanner_get_next_token", EFT_NOOP},
   {"g_scanner_input_file", EFT_NOOP},
   {"g_scanner_scope_add_symbol", EFT_NOOP},
   {"gcry_cipher_close", EFT_NOOP},
   {"gcry_cipher_ctl", EFT_NOOP},
   {"gcry_cipher_decrypt", EFT_NOOP},
   {"gcry_cipher_map_name", EFT_NOOP},
   {"gcry_cipher_open", EFT_NOOP},
   {"gcry_md_close", EFT_NOOP},
   {"gcry_md_ctl", EFT_NOOP},
   {"gcry_md_get_algo", EFT_NOOP},
   {"gcry_md_hash_buffer", EFT_NOOP},
   {"gcry_md_map_name", EFT_NOOP},
   {"gcry_md_open", EFT_NOOP},
   {"gcry_md_setkey", EFT_NOOP},
   {"gcry_md_write", EFT_NOOP},
   {"gcry_mpi_add", EFT_NOOP},
   {"gcry_mpi_add_ui", EFT_NOOP},
   {"gcry_mpi_clear_highbit", EFT_NOOP},
   {"gcry_mpi_print", EFT_NOOP},
   {"gcry_mpi_release", EFT_NOOP},
   {"gcry_sexp_release", EFT_NOOP},
   {"getaddrinfo", EFT_NOOP},
   {"getc_unlocked", EFT_NOOP},
   {"getgroups", EFT_NOOP},
   {"gethostname", EFT_NOOP},
   {"getloadavg", EFT_NOOP},
   {"getopt", EFT_NOOP},
   {"getopt_long", EFT_NOOP},
   {"getopt_long_only", EFT_NOOP},
   {"getpeername", EFT_NOOP},
   {"getresgid", EFT_NOOP},
   {"getresuid", EFT_NOOP},
   {"getrlimit", EFT_NOOP},
   {"getrlimit64", EFT_NOOP},
   {"getrusage", EFT_NOOP},
   {"getsockname", EFT_NOOP},
   {"getsockopt", EFT_NOOP},
   {"gettimeofday", EFT_NOOP},
   {"globfree", EFT_NOOP},
   {"gnutls_pkcs12_bag_decrypt", EFT_NOOP},
   {"gnutls_pkcs12_bag_deinit", EFT_NOOP},
   {"gnutls_pkcs12_bag_get_count", EFT_NOOP},
   {"gnutls_pkcs12_bag_get_type", EFT_NOOP},
   {"gnutls_x509_crt_deinit", EFT_NOOP},
   {"gnutls_x509_crt_get_dn_by_oid", EFT_NOOP},
   {"gnutls_x509_crt_get_key_id", EFT_NOOP},
   {"gnutls_x509_privkey_deinit", EFT_NOOP},
   {"gnutls_x509_privkey_get_key_id", EFT_NOOP},
   {"gzclose", EFT_NOOP},
   {"gzeof", EFT_NOOP},
   {"gzgetc", EFT_NOOP},
   {"gzread", EFT_NOOP},
   {"gzseek", EFT_NOOP},
   {"gztell", EFT_NOOP},
   {"gzwrite", EFT_NOOP},
   {"iconv_close", EFT_NOOP},
   {"inet_addr", EFT_NOOP},
   {"inet_aton", EFT_NOOP},
   {"inet_pton", EFT_NOOP},
   {"inflate", EFT_NOOP},
   {"inflateEnd", EFT_NOOP},
   {"inflateInit2_", EFT_NOOP},
   {"inflateInit_", EFT_NOOP},
   {"inflateReset", EFT_NOOP},
   {"initgroups", EFT_NOOP},
   {"jpeg_CreateCompress", EFT_NOOP},
   {"jpeg_CreateDecompress", EFT_NOOP},
   {"jpeg_destroy", EFT_NOOP},
   {"jpeg_finish_compress", EFT_NOOP},
   {"jpeg_finish_decompress", EFT_NOOP},
   {"jpeg_read_header", EFT_NOOP},
   {"jpeg_read_scanlines", EFT_NOOP},
   {"jpeg_resync_to_restart", EFT_NOOP},
   {"jpeg_set_colorspace", EFT_NOOP},
   {"jpeg_set_defaults", EFT_NOOP},
   {"jpeg_set_linear_quality", EFT_NOOP},
   {"jpeg_set_quality", EFT_NOOP},
   {"jpeg_start_compress", EFT_NOOP},
   {"jpeg_start_decompress", EFT_NOOP},
   {"jpeg_write_scanlines", EFT_NOOP},
   {"keypad", EFT_NOOP},
   {"lchown", EFT_NOOP},
   {"link", EFT_NOOP},
   {"llvm.memset.i32", EFT_NOOP},
   {"llvm.memset.i64", EFT_NOOP},
   {"llvm.stackrestore", EFT_NOOP},
   {"llvm.va_copy", EFT_NOOP},
   {"llvm.va_end", EFT_NOOP},
   {"llvm.va_start", EFT_NOOP},
   {"llvm.lifetime.start", EFT_NOOP},
   {"llvm.lifetime.end", EFT_NOOP},
   {"longjmp", EFT_NOOP},
   {"lstat", EFT_NOOP},
   {"lstat64", EFT_NOOP},
   {"mblen", EFT_NOOP},
   {"mbrlen", EFT_NOOP},
   {"mbrtowc", EFT_NOOP},
   {"mbtowc", EFT_NOOP},
   {"memcmp", EFT_NOOP},
   {"mkdir", EFT_NOOP},
   {"mknod", EFT_NOOP},
   {"mkfifo", EFT_NOOP},
   {"mkstemp", EFT_NOOP},
   {"mkstemp64", EFT_NOOP},
   {"mktime", EFT_NOOP},
   {"modf", EFT_NOOP},
   {"mprotect", EFT_NOOP},
   {"munmap", EFT_NOOP},
   {"nanosleep", EFT_NOOP},
   {"nhfree", EFT_NOOP},
   {"nodelay", EFT_NOOP},
   {"obstack_free", EFT_NOOP},
   {"open", EFT_NOOP},
   {"open64", EFT_NOOP},
   {"openlog", EFT_NOOP},
   {"openpty", EFT_NOOP},
   {"pathconf", EFT_NOOP},
   {"pclose", EFT_NOOP},
   {"perror", EFT_NOOP},
   {"pipe", EFT_NOOP},
   {"png_destroy_write_struct", EFT_NOOP},
   {"png_init_io", EFT_NOOP},
   {"png_set_bKGD", EFT_NOOP},
   {"png_set_invert_alpha", EFT_NOOP},
   {"png_set_invert_mono", EFT_NOOP},
   {"png_write_end", EFT_NOOP},
   {"png_write_info", EFT_NOOP},
   {"png_write_rows", EFT_NOOP},
   {"poll", EFT_NOOP},
   {"pread64", EFT_NOOP},
   {"printf", EFT_NOOP},
   {"pthread_attr_destroy", EFT_NOOP},
   {"pthread_attr_init", EFT_NOOP},
   {"pthread_attr_setscope", EFT_NOOP},
   {"pthread_attr_setstacksize", EFT_NOOP},
   {"pthread_create", EFT_NOOP},
   {"pthread_mutex_destroy", EFT_NOOP},
   {"pthread_mutex_init", EFT_NOOP},
   {"pthread_mutex_lock", EFT_NOOP},
   {"pthread_mutex_unlock", EFT_NOOP},
   {"pthread_mutexattr_destroy", EFT_NOOP},
   {"pthread_mutexattr_init", EFT_NOOP},
   {"pthread_mutexattr_settype", EFT_NOOP},
   {"ptsname", EFT_NOOP},
   {"putenv", EFT_NOOP},
   {"puts", EFT_NOOP},
   {"qsort", EFT_NOOP},
   {"re_compile_fastmap", EFT_NOOP},
   {"re_exec", EFT_NOOP},
   {"re_search", EFT_NOOP},
   {"read", EFT_NOOP},
   {"readlink", EFT_NOOP},
   {"recv", EFT_NOOP},
   {"recvfrom", EFT_NOOP},
   {"regcomp", EFT_NOOP},
   {"regerror", EFT_NOOP},
   {"remove", EFT_NOOP},
   {"rename", EFT_NOOP},
   {"rewind", EFT_NOOP},
   {"rewinddir", EFT_NOOP},
   {"rmdir", EFT_NOOP},
   {"rresvport", EFT_NOOP},
   {"safe_cfree", EFT_NOOP},
   {"safe_free", EFT_NOOP},
   {"safefree", EFT_NOOP},
   {"safexfree", EFT_NOOP},
   {"scrollok", EFT_NOOP},
   {"select", EFT_NOOP},
   {"sem_destroy", EFT_NOOP},
   {"sem_init", EFT_NOOP},
   {"sem_post", EFT_NOOP},
   {"sem_trywait", EFT_NOOP},
   {"sem_wait", EFT_NOOP},
   {"send", EFT_NOOP},
   {"sendto", EFT_NOOP},
   {"setbuf", EFT_NOOP},
   {"setenv", EFT_NOOP},
   {"setgroups", EFT_NOOP},
   {"setitimer", EFT_NOOP},
   {"setrlimit", EFT_NOOP},
   {"setsockopt", EFT_NOOP},
   {"setvbuf", EFT_NOOP},
   {"sigaction", EFT_NOOP},
   {"sigaddset", EFT_NOOP},
   {"sigaltstack", EFT_NOOP},
   {"sigdelset", EFT_NOOP},
   {"sigemptyset", EFT_NOOP},
   {"sigfillset", EFT_NOOP},
   {"sigisemptyset", EFT_NOOP},
   {"sigismember", EFT_NOOP},
   {"siglongjmp", EFT_NOOP},
   {"sigprocmask", EFT_NOOP},
   {"sigsuspend", EFT_NOOP},
   {"sm_free", EFT_NOOP},
   {"snprintf", EFT_NOOP},
   {"socketpair", EFT_NOOP},
   {"sprintf", EFT_NOOP},
   {"sscanf", EFT_NOOP},
   {"stat", EFT_NOOP},
   {"stat64", EFT_NOOP},
   {"statfs", EFT_NOOP},
   {"statvfs", EFT_NOOP},
   {"statvfs64", EFT_NOOP},
   {"strcasecmp", EFT_NOOP},
   {"strcmp", EFT_NOOP},
   {"strcoll", EFT_NOOP},
   {"strcspn", EFT_NOOP},
   {"strfmon", EFT_NOOP},
   {"strftime", EFT_NOOP},
   {"strlen", EFT_NOOP},
   {"strncasecmp", EFT_NOOP},
   {"strncmp", EFT_NOOP},
   {"strspn", EFT_NOOP},
   {"symlink", EFT_NOOP},
   {"sysinfo", EFT_NOOP},
   {"syslog", EFT_NOOP},
   {"system", EFT_NOOP},
   {"tcgetattr", EFT_NOOP},
   {"tcsetattr", EFT_NOOP},
   {"tgetent", EFT_NOOP},
   {"tgetflag", EFT_NOOP},
   {"tgetnum", EFT_NOOP},
   {"time", EFT_NOOP},
   {"timegm", EFT_NOOP},
   {"times", EFT_NOOP},
   {"tputs", EFT_NOOP},
   {"truncate", EFT_NOOP},
   {"uname", EFT_NOOP},
   {"uncompress", EFT_NOOP},
   {"ungetc", EFT_NOOP},
   {"unlink", EFT_NOOP},
   {"unsetenv", EFT_NOOP},
   {"utime", EFT_NOOP},
   {"utimes", EFT_NOOP},
   {"vasprintf", EFT_NOOP},
   {"vfprintf", EFT_NOOP},
   {"vim_free", EFT_NOOP},
   {"vprintf", EFT_NOOP},
   {"vsnprintf", EFT_NOOP},
   {"vsprintf", EFT_NOOP},
   {"waddch", EFT_NOOP},
   {"waddnstr", EFT_NOOP},
   {"wait", EFT_NOOP},
   {"wait3", EFT_NOOP},
   {"wait4", EFT_NOOP},
   {"waitpid", EFT_NOOP},
   {"wattr_off", EFT_NOOP},
   {"wattr_on", EFT_NOOP},
   {"wborder", EFT_NOOP},
   {"wclrtobot", EFT_NOOP},
   {"wclrtoeol", EFT_NOOP},
   {"wcrtomb", EFT_NOOP},
   {"wctomb", EFT_NOOP},
   {"wctype", EFT_NOOP},
   {"werase", EFT_NOOP},
   {"wgetch", EFT_NOOP},
   {"wmove", EFT_NOOP},
   {"wrefresh", EFT_NOOP},
   {"write", EFT_NOOP},
   {"wtouchln", EFT_NOOP},
   {"xfree", EFT_NOOP},

   {"\01fopen64", EFT_ALLOC},
   {"\01readdir64", EFT_ALLOC},
   {"\01tmpfile64", EFT_ALLOC},
   {"BIO_new_socket", EFT_ALLOC},
   {"FT_Get_Sfnt_Table", EFT_ALLOC},
   {"FcFontList", EFT_ALLOC},
   {"FcFontMatch", EFT_ALLOC},
   {"FcFontRenderPrepare", EFT_ALLOC},
   {"FcFontSetCreate", EFT_ALLOC},
   {"FcFontSort", EFT_ALLOC},
   {"FcInitLoadConfig", EFT_ALLOC},
   {"FcObjectSetBuild", EFT_ALLOC},
   {"FcObjectSetCreate", EFT_ALLOC},
   {"FcPatternBuild", EFT_ALLOC},
   {"FcPatternCreate", EFT_ALLOC},
   {"FcPatternDuplicate", EFT_ALLOC},
   {"SSL_CTX_new", EFT_ALLOC},
   {"SSL_get_peer_certificate", EFT_ALLOC},
   {"SSL_new", EFT_ALLOC},
   {"SSLv23_client_method", EFT_ALLOC},
   {"SyGetmem", EFT_ALLOC},
   {"TLSv1_client_method", EFT_ALLOC},
   {"Void_ExtendCore", EFT_ALLOC},
   {"XAddExtension", EFT_ALLOC},
   {"XAllocClassHint", EFT_ALLOC},
   {"XAllocSizeHints", EFT_ALLOC},
   {"XAllocStandardColormap", EFT_ALLOC},
   {"XCreateFontSet", EFT_ALLOC},
   {"XCreateImage", EFT_ALLOC},
   {"XCreateGC", EFT_ALLOC},
   //Returns the prev. defined handler.
   {"XESetCloseDisplay", EFT_ALLOC},
   {"XGetImage", EFT_ALLOC},
   {"XGetModifierMapping", EFT_ALLOC},
   {"XGetMotionEvents", EFT_ALLOC},
   {"XGetVisualInfo", EFT_ALLOC},
   {"XLoadQueryFont", EFT_ALLOC},
   {"XListPixmapFormats", EFT_ALLOC},
   {"XRenderFindFormat", EFT_ALLOC},
   {"XRenderFindStandardFormat", EFT_ALLOC},
   {"XRenderFindVisualFormat", EFT_ALLOC},
   {"XOpenDisplay", EFT_ALLOC},
   //These 2 return the previous handler.
   {"XSetErrorHandler", EFT_ALLOC},
   {"XSetIOErrorHandler", EFT_ALLOC},
   {"XShapeGetRectangles", EFT_ALLOC},
   {"XShmCreateImage", EFT_ALLOC},
   //This returns the handler last passed to XSetAfterFunction().
   {"XSynchronize", EFT_ALLOC},
   {"XcursorImageCreate", EFT_ALLOC},
   {"XcursorLibraryLoadImages", EFT_ALLOC},
   {"XcursorShapeLoadImages", EFT_ALLOC},
   {"XineramaQueryScreens", EFT_ALLOC},
   {"XkbGetMap", EFT_ALLOC},
   {"XtAppCreateShell", EFT_ALLOC},
   {"XtCreateApplicationContext", EFT_ALLOC},
   {"XtOpenDisplay", EFT_ALLOC},
   {"alloc", EFT_ALLOC},
   {"alloc_check", EFT_ALLOC},
   {"alloc_clear", EFT_ALLOC},
   {"art_svp_from_vpath", EFT_ALLOC},
   {"art_svp_vpath_stroke", EFT_ALLOC},
   {"art_svp_writer_rewind_new", EFT_ALLOC},
   //FIXME: returns arg0->svp
   {"art_svp_writer_rewind_reap", EFT_ALLOC},
   {"art_vpath_dash", EFT_ALLOC},
   {"cairo_create", EFT_ALLOC},
   {"cairo_image_surface_create_for_data", EFT_ALLOC},
   {"cairo_pattern_create_for_surface", EFT_ALLOC},
   {"cairo_surface_create_similar", EFT_ALLOC},
   {"calloc", EFT_ALLOC},
   {"fopen", EFT_ALLOC},
   {"fopen64", EFT_ALLOC},
   {"fopencookie", EFT_ALLOC},
   {"g_scanner_new", EFT_ALLOC},
   {"gcry_sexp_nth_mpi", EFT_ALLOC},
   {"gzdopen", EFT_ALLOC},
   {"iconv_open", EFT_ALLOC},
   {"jpeg_alloc_huff_table", EFT_ALLOC},
   {"jpeg_alloc_quant_table", EFT_ALLOC},
   {"lalloc", EFT_ALLOC},
   {"lalloc_clear", EFT_ALLOC},
   {"malloc", EFT_ALLOC},
   {"nhalloc", EFT_ALLOC},
   {"oballoc", EFT_ALLOC},
   {"pango_cairo_font_map_create_context", EFT_ALLOC},
   //This may also point *arg2 to a new string.
   {"pcre_compile", EFT_ALLOC},
   {"pcre_study", EFT_ALLOC},
   {"permalloc", EFT_ALLOC},
   {"png_create_info_struct", EFT_ALLOC},
   {"png_create_write_struct", EFT_ALLOC},
   {"popen", EFT_ALLOC},
   {"pthread_getspecific", EFT_ALLOC},
   {"readdir", EFT_ALLOC},
   {"readdir64", EFT_ALLOC},
   {"safe_calloc", EFT_ALLOC},
   {"safe_malloc", EFT_ALLOC},
   {"safecalloc", EFT_ALLOC},
   {"safemalloc", EFT_ALLOC},
   {"safexcalloc", EFT_ALLOC},
   {"safexmalloc", EFT_ALLOC},
   {"savealloc", EFT_ALLOC},
   {"setmntent", EFT_ALLOC},
   {"shmat", EFT_ALLOC},
   //These 2 return the previous handler.
   {"signal", EFT_ALLOC},
   {"sigset", EFT_ALLOC},
   {"tempnam", EFT_ALLOC},
   {"tmpfile", EFT_ALLOC},
   {"tmpfile64", EFT_ALLOC},
   {"xalloc", EFT_ALLOC},
   {"xcalloc", EFT_ALLOC},
   {"xmalloc", EFT_ALLOC},

   {"\01mmap64", EFT_NOSTRUCT_ALLOC},
   //FIXME: this is like realloc but with arg1.
   {"X509_NAME_oneline", EFT_NOSTRUCT_ALLOC},
   {"X509_verify_cert_error_string", EFT_NOSTRUCT_ALLOC},
   {"XBaseFontNameListOfFontSet", EFT_NOSTRUCT_ALLOC},
   {"XGetAtomName", EFT_NOSTRUCT_ALLOC},
   {"XGetDefault", EFT_NOSTRUCT_ALLOC},
   {"XGetKeyboardMapping", EFT_NOSTRUCT_ALLOC},
   {"XListDepths", EFT_NOSTRUCT_ALLOC},
   {"XListFonts", EFT_NOSTRUCT_ALLOC},
   {"XSetLocaleModifiers", EFT_NOSTRUCT_ALLOC},
   {"XcursorGetTheme", EFT_NOSTRUCT_ALLOC},
   {"__strdup", EFT_NOSTRUCT_ALLOC},
   {"crypt", EFT_NOSTRUCT_ALLOC},
   {"ctime", EFT_NOSTRUCT_ALLOC},
   {"dlerror", EFT_NOSTRUCT_ALLOC},
   {"dlopen", EFT_NOSTRUCT_ALLOC},
   {"gai_strerror", EFT_NOSTRUCT_ALLOC},
   {"gcry_cipher_algo_name", EFT_NOSTRUCT_ALLOC},
   {"gcry_md_algo_name", EFT_NOSTRUCT_ALLOC},
   {"gcry_md_read", EFT_NOSTRUCT_ALLOC},
   {"getenv", EFT_NOSTRUCT_ALLOC},
   {"getlogin", EFT_NOSTRUCT_ALLOC},
   {"getpass", EFT_NOSTRUCT_ALLOC},
   {"gnutls_strerror", EFT_NOSTRUCT_ALLOC},
   {"gpg_strerror", EFT_NOSTRUCT_ALLOC},
   {"gzerror", EFT_NOSTRUCT_ALLOC},
   {"inet_ntoa", EFT_NOSTRUCT_ALLOC},
   {"initscr", EFT_NOSTRUCT_ALLOC},
   {"llvm.stacksave", EFT_NOSTRUCT_ALLOC},
   {"mmap", EFT_NOSTRUCT_ALLOC},
   {"mmap64", EFT_NOSTRUCT_ALLOC},
   {"newwin", EFT_NOSTRUCT_ALLOC},
   {"nl_langinfo", EFT_NOSTRUCT_ALLOC},
   {"opendir", EFT_NOSTRUCT_ALLOC},
   {"sbrk", EFT_NOSTRUCT_ALLOC},
   {"strdup", EFT_NOSTRUCT_ALLOC},
   {"strerror", EFT_NOSTRUCT_ALLOC},
   {"strsignal", EFT_NOSTRUCT_ALLOC},
   {"textdomain", EFT_NOSTRUCT_ALLOC},
   {"tgetstr", EFT_NOSTRUCT_ALLOC},
   {"tigetstr", EFT_NOSTRUCT_ALLOC},
   {"tmpnam", EFT_NOSTRUCT_ALLOC},
   {"ttyname", EFT_NOSTRUCT_ALLOC},

   {"__ctype_b_loc", EFT_STAT2},
   {"__ctype_tolower_loc", EFT_STAT2},
   {"__ctype_toupper_loc", EFT_STAT2},

   {"XKeysymToString", EFT_STAT},
   {"__errno_location", EFT_STAT},
   {"__h_errno_location", EFT_STAT},
   {"__res_state", EFT_STAT},
   {"asctime", EFT_STAT},
   {"bindtextdomain", EFT_STAT},
   {"bind_textdomain_codeset", EFT_STAT},
   //This is L_A0 when arg0 is not null.
   {"ctermid", EFT_STAT},
   {"dcgettext", EFT_STAT},
   {"dgettext", EFT_STAT},
   {"dngettext", EFT_STAT},
   {"fdopen", EFT_STAT},
   {"gcry_strerror", EFT_STAT},
   {"gcry_strsource", EFT_STAT},
   {"getgrgid", EFT_STAT},
   {"getgrnam", EFT_STAT},
   {"gethostbyaddr", EFT_STAT},
   {"gethostbyname", EFT_STAT},
   {"gethostbyname2", EFT_STAT},
   {"getmntent", EFT_STAT},
   {"getprotobyname", EFT_STAT},
   {"getprotobynumber", EFT_STAT},
   {"getpwent", EFT_STAT},
   {"getpwnam", EFT_STAT},
   {"getpwuid", EFT_STAT},
   {"getservbyname", EFT_STAT},
   {"getservbyport", EFT_STAT},
   {"getspnam", EFT_STAT},
   {"gettext", EFT_STAT},
   {"gmtime", EFT_STAT},
   {"gnu_get_libc_version", EFT_STAT},
   {"gnutls_check_version", EFT_STAT},
   {"localeconv", EFT_STAT},
   {"localtime", EFT_STAT},
   {"ngettext", EFT_STAT},
   {"pango_cairo_font_map_get_default", EFT_STAT},
   {"re_comp", EFT_STAT},
   {"setlocale", EFT_STAT},
   {"tgoto", EFT_STAT},
   {"tparm", EFT_STAT},
   {"zError", EFT_STAT},

   {"getcwd", EFT_REALLOC},
   {"mem_realloc", EFT_REALLOC},
   {"realloc", EFT_REALLOC},
   {"realloc_obj", EFT_REALLOC},
   {"safe_realloc", EFT_REALLOC},
   {"saferealloc", EFT_REALLOC},
   {"safexrealloc", EFT_REALLOC},
   //FIXME: when arg0 is null, the return points into the string that was
   //  last passed in arg0 (rather than a new string, as for realloc).
   {"strtok", EFT_REALLOC},
   //As above, but also stores the last string into *arg2.
   {"strtok_r", EFT_REALLOC},
   {"xrealloc", EFT_REALLOC},

   {"__rawmemchr", EFT_L_A0},
   {"cairo_surface_reference", EFT_L_A0},
   {"dlsym", EFT_L_A0},
   {"fgets", EFT_L_A0},
   {"jpeg_std_error", EFT_L_A0},
   {"memchr", EFT_L_A0},
   //This will overwrite *arg0 with non-pointer data -
   //  assume that no valid pointer values are created.
   {"memset", EFT_L_A0},
   //This may return a new ptr if the region was moved.
   {"mremap", EFT_L_A0},
   {"stpcpy", EFT_L_A0},
   {"strcat", EFT_L_A0},
   {"strchr", EFT_L_A0},
   {"strcpy", EFT_L_A0},
   {"strerror_r", EFT_L_A0},
   {"strncat", EFT_L_A0},
   {"strncpy", EFT_L_A0},
   {"strpbrk", EFT_L_A0},
   {"strptime", EFT_L_A0},
   {"strrchr", EFT_L_A0},
   {"strstr", EFT_L_A0},
   {"tmpnam_r", EFT_L_A0},
   {"asctime_r", EFT_L_A1},
   {"bsearch", EFT_L_A1},
   {"getmntent_r", EFT_L_A1},
   {"gmtime_r", EFT_L_A1},
   {"gzgets", EFT_L_A1},
   {"localtime_r", EFT_L_A1},
   {"realpath", EFT_L_A1},
   {"\01freopen64", EFT_L_A2},
   //FIXME: may do L_A3 if arg5 > 0.
   {"_XGetAsyncReply", EFT_L_A2},
   {"freopen", EFT_L_A2},
   {"freopen64", EFT_L_A2},
   {"inet_ntop", EFT_L_A2},
   {"XGetSubImage", EFT_L_A8},

   {"llvm.memcpy.i32", EFT_L_A0__A0R_A1R},
   {"llvm.memcpy.i64", EFT_L_A0__A0R_A1R},
   {"llvm.memmove.i32", EFT_L_A0__A0R_A1R},
   {"memccpy", EFT_L_A0__A0R_A1R},
   {"memmove", EFT_L_A0__A0R_A1R},
   {"bcopy", EFT_A1R_A0R},
   {"iconv", EFT_A3R_A1R_NS},

   {"strtod", EFT_A1R_A0},
   {"strtof", EFT_A1R_A0},
   {"strtol", EFT_A1R_A0},
   {"strtold", EFT_A1R_A0},
   {"strtoll", EFT_A1R_A0},
   {"strtoul", EFT_A1R_A0},
   {"readdir_r", EFT_A2R_A1},
   //These also set arg1->pw_name etc. to new strings.
   {"getpwnam_r", EFT_A4R_A1},
   {"getpwuid_r", EFT_A4R_A1},

   {"db_create", EFT_A0R_NEW},
   {"gcry_mpi_scan", EFT_A0R_NEW},
   {"gcry_pk_decrypt", EFT_A0R_NEW},
   {"gcry_sexp_build", EFT_A0R_NEW},
   {"gnutls_pkcs12_bag_init", EFT_A0R_NEW},
   {"gnutls_pkcs12_init", EFT_A0R_NEW},
   {"gnutls_x509_crt_init", EFT_A0R_NEW},
   {"gnutls_x509_privkey_init", EFT_A0R_NEW},
   {"posix_memalign", EFT_A0R_NEW},
   {"scandir", EFT_A1R_NEW},
   {"XGetRGBColormaps", EFT_A2R_NEW},
   {"XmbTextPropertyToTextList", EFT_A2R_NEW},
   {"XQueryTree", EFT_A4R_NEW},
   {"XGetWindowProperty", EFT_A11R_NEW},

   //This must be the last entry.
   {0, EFT_NOOP}
};

/*  FIXME:
 *  SSL_CTX_ctrl, SSL_ctrl - may set the ptr field arg0->x
 *  SSL_CTX_set_verify - sets the function ptr field arg0->x
 *  X509_STORE_CTX_get_current_cert - returns arg0->x
 *  X509_get_subject_name - returns arg0->x->y
 *  XStringListToTextProperty, XGetWindowAttributes - sets arg2->x
 *  XInitImage - sets function ptrs arg0->x->y
 *  XMatchVisualInfo - sets arg4->x
 *  XtGetApplicationResources - ???
 *  glob - sets arg3->gl_pathv
 *  gnutls_pkcs12_bag_get_data - copies arg0->element[arg1].data to *arg2
 *  gnutls_pkcs12_get_bag - finds the arg1'th bag in the ASN1 tree structure
 *    rooted at arg0->pkcs12 and copies it to *arg2
 *  gnutls_pkcs12_import - builds an ASN1 tree rooted at arg0->pkcs12,
 *    based on decrypted data
 *  gnutls_x509_crt_import - builds an ASN1 tree rooted at arg0->cert
 *  gnutls_x509_privkey_export_rsa_raw - points arg1->data thru arg6->data
 *    to new strings
 *  gnutls_x509_privkey_import, gnutls_x509_privkey_import_pkcs8 -
 *    builds an ASN1 tree rooted at arg0->key from decrypted data
 *  cairo_get_target - returns arg0->gstate->original_target
 *  hasmntopt - returns arg0->mnt_opts
 */

void ExtInfo::init()
{
   std::set<extf_t> t_seen;
   extf_t prev_t= EFT_NOOP;
   t_seen.insert(EFT_NOOP);
   for(const ei_pair *p= ei_pairs; p->n; ++p)
   {
      if(p->t != prev_t)
      {
         //This will detect if you move an entry to another block
         //  but forget to change the type.
         if(t_seen.count(p->t))
         {
            llvm::errs() << p->n << "\n";
            assert(!"ei_pairs not grouped by type");
         }
         t_seen.insert(p->t);
         prev_t= p->t;
      }
      if(info.count(p->n))
      {
         llvm::errs() << p->n << "\n";
         assert(!"duplicate name in ei_pairs");
      }
      info[p->n]= p->t;
   }
}


extf_t ExtInfo::get_type(const llvm::Function *F) const
{
   assert(F);
   auto it= info.find(F->getName());
   if(it == info.end())
      return EFT_OTHER;
   else
      return it->second;
}

bool ExtInfo::is_ext(const llvm::Function *F)
{
   assert(F);
   //Check the cache first; everything below is slower.
   auto i_iec= isext_cache.find(F);
   if(i_iec != isext_cache.end())
      return i_iec->second;

   bool res;
   if(F->isDeclaration() || F->isIntrinsic())
   {
      res= 1;
   }
   else
   {
      extf_t t= get_type(F);
      res= t==EFT_ALLOC || t==EFT_REALLOC || t==EFT_NOSTRUCT_ALLOC
           || t==EFT_NOOP;
   }
   isext_cache[F]= res;
   return res;
}




//------------------------------------------------------------------------------
// Node class
//------------------------------------------------------------------------------
class Node
{
   private:
      //The LLVM value represented by this node, or 0 for artificial nodes
      const llvm::Value *val;

   public:

      //How many nodes in the object that starts here (0 if it's not an obj node).
      //  For structs this equals the corresponding struct_sz element.
      u32 obj_sz;
      //The time this node was last visited
      u32 vtime;
      //If rep < node_rank_min, this node is part of a set of equivalent nodes
      //  and (rep) is another node in that set.
      //Else this is the representative node of the set,
      //  and (rep) is its rank in the union-find structure.
      u32 rep;
      //If this node was determined to not point to anything
      bool nonptr;
      //For SFS: true if this is an array or is heap-allocated
      bool weak;

      //The nodes in our points-to set
      bdd points_to;
      //The points_to set at the start of the last visit to this node
      bdd prev_points_to;
      //The simple constraint edges, i.e. the neighbors that
      //  include our points-to set
      bitmap copy_to;
      //Indices into cplx_cons for load, store, and gep cons.
      bitmap load_to, store_from, gep_to;

      //------------------------------------------------------------------------------
      Node(const llvm::Value *v= 0, u32 s= 0, bool w= 0): val(v), obj_sz(s), vtime(0),
         rep(node_rank_min), nonptr(0), weak(w) {}

      bool is_rep() const
      {
         return rep >= node_rank_min;
      }

      const llvm::Value* get_val() const
      {
         return val;
      }
      void set_val(const llvm::Value *v)
      {
         val= v;
      }
};

//There are 5 types of constraints in Andersen's analysis:
//  Address-of (Base): D = &S
//  Copy (Simple): D = S
//  Load (Complex 1): D = *S + off
//  Store (Complex 2): *D + off = S
//  GEP (copy+offset): D = S + off
enum ConsType : unsigned int {addr_of_cons=0, copy_cons, load_cons, store_cons, gep_cons};


//------------------------------------------------------------------------------
class Constraint
{
   public:
      ConsType type;
      u32 dest, src, off;

      Constraint(ConsType t, u32 d, u32 s, u32 o= 0):
         type(t), dest(d), src(s), off(o) {}

      //------------------------------------------------------------------------------
      bool operator == (const Constraint &b) const
      {
         return type == b.type && dest == b.dest && src == b.src && off == b.off;
      }
      bool operator < (const Constraint &b) const
      {
         if(type != b.type)
            return type < b.type;
         if(dest != b.dest)
            return dest < b.dest;
         if(src != b.src)
            return src < b.src;
         return off < b.off;
      }
      bool operator > (const Constraint &b) const
      {
         if(type != b.type)
            return type > b.type;
         if(dest != b.dest)
            return dest > b.dest;
         if(src != b.src)
            return src > b.src;
         return off > b.off;
      }
      bool operator != (const Constraint &b) const
      {
         return !(operator==(b));
      }
      bool operator >= (const Constraint &b) const
      {
         return !(operator<(b));
      }
      bool operator <= (const Constraint &b) const
      {
         return !(operator>(b));
      }
};

namespace llvm
{
   template<> struct DenseMapInfo<Constraint>
   {
         static Constraint getEmptyKey(){
            return Constraint(addr_of_cons, 0, 0, 0);
         }
         static Constraint getTombstoneKey(){
            return Constraint(copy_cons, 0, 0, 0);
         }
         static unsigned getHashValue(const Constraint &X){
            return ((u32)X.type<<29) ^ (X.dest<<12) ^ X.src ^ X.off;
         }
         static unsigned isEqual(const Constraint &X, const Constraint &Y){
            return X == Y;
         }
   };
}


//A binary min-heap structure with keys in the range [0, UINT_MAX]
//  and values in the range [0, MV].
//The memory requirement is always 12*MV B, regardless of the current size.
//No value may occur more than once; inserting a value that already exists
//  will change its key.
class Heap
{
   private:
      static const u32 NONE= 0xffffffff;
      //The keys in the actual heap and the corresponding values.
      //hk[0] is always empty; hk[1] is the min element.
      u32 *hk, *hv;
      //The max value allowed in the heap and the current number of items.
      //(nv) is always equal to the pos. of the last item.
      u32 maxv, nv;
      //The position of each value in (hv), or NONE if it's not there.
      u32 *idxv;

      //Percolate up/down from pos. (i0), to restore the heap property.
      void perc_up(u32 i0)
      {
         for(u32 i= i0; ; )
         {
            u32 ip= i/2;                    //parent pos.
            if(!ip)
               return;
            u32 k= hk[i], kp= hk[ip];
            //if the parent key is not greater, we're done
            if(kp <= k)
               return;
            //Swap key and value with the parent.
            hk[ip]= k, hk[i]= kp;
            u32 v= hv[i], vp= hv[ip];
            hv[ip]= v, hv[i]= vp;
            idxv[v]= ip, idxv[vp]= i;       //swap the values' indices
            i= ip;
         }
      }
      void perc_dn(u32 i0)
      {
         for(u32 i= i0; ; )
         {
            u32 ic= i*2;                    //left child pos.
            if(ic > nv)                     //no left child
               return;
            u32 k= hk[i], kc= hk[ic];
            if(ic < nv){                    //right child exists
               u32 kr= hk[ic+1];
               if(kr < kc)                   //use the right child if it's smaller
                  ++ic, kc= kr;
            }

            if(kc >= k)                     //if the child key is not less, we're done
               return;
            hk[ic]= k, hk[i]= kc;
            u32 v= hv[i], vc= hv[ic];
            hv[ic]= v, hv[i]= vc;
            idxv[v]= ic, idxv[vc]= i;
            i= ic;
         }
      }
   public:
      Heap(u32 mv) : maxv(mv)
      {
         nv= 0;
         //hk, hv have an extra word before the first item,
         //  and there may be up to (mv+1) items.
         hk= (u32*)malloc((mv+2)*4);
         hv= (u32*)malloc((mv+2)*4);
         idxv= (u32*)malloc((mv+1)*4);
         memset(idxv, 0xff, (mv+1)*4);     //set all words to NONE
      }
      ~Heap()
      {
         free(hk); free(hv); free(idxv);
      }
      void swap(Heap &b)
      {
         u32 t, *p;
         p= hk, hk= b.hk, b.hk= p;
         p= hv, hv= b.hv, b.hv= p;
         p= idxv, idxv= b.idxv, b.idxv= p;
         t= maxv, maxv= b.maxv, b.maxv= t;
         t= nv, nv= b.nv, b.nv= t;
      }
      u32 size() const{
         return nv;
      }
      bool empty() const{
         return !nv;
      }
      //Insert the given value and key; returns true if (v) already existed.
      bool push(u32 v, u32 k)
      {
         assert(v <= maxv);
         u32 i= idxv[v];
         if(i == NONE)
         {
            //Append the new item and move up as needed.
            hk[++nv]= k;
            hv[nv]= v;
            idxv[v]= nv;
            perc_up(nv);
            return 0;
         }
         //Change the key in place and move up/down as needed.
         assert(i <= nv);
         u32 pk= hk[i];
         if(k == pk)
            return 1;
         hk[i]= k;
         if(k < pk)
            perc_up(i);
         else
            perc_dn(i);
         return 1;
      }
      //Delete and return the current min value, storing its key in (pk).
      u32 pop(u32 *pk= 0)
      {
         assert(nv);
         u32 k= hk[1], v= hv[1];
         idxv[v]= NONE;
         --nv;
         //Move the last item down from the top, if it exists.
         if(nv){
            hk[1]= hk[nv+1];
            u32 lv= hv[nv+1];
            hv[1]= lv;
            idxv[lv]= 1;
            perc_dn(1);
         }
         if(pk)
            *pk= k;
         return v;
      }
};


//Wrapper for the heap (pops earliest vtime first).
class Worklist
{
   private:
      Heap curr, next;

   public:
      //The constructor requires the node count (nn) to make the heap.
      Worklist(u32 nn) : curr(nn), next(nn) {}
      bool empty() const
      {
         return curr.empty();
      }
      bool swap_if_empty()
      {
         if(curr.empty()){
            curr.swap(next);
            return 1;
         }
         return 0;
      }
      //Insert node (n) with priority (p) into the list.
      void push(u32 n, u32 p){
         next.push(n, p);
      }
      //Return the top-priority node from the list,
      //  storing the priority into (pp) if provided.
      u32 pop(u32 *pp= 0)
      {
         assert(!curr.empty() && "trying to pop empty worklist");
         return curr.pop(pp);
      }
};


Andersen_AA::Andersen_AA(const std::string &_TopFunctionName) : TopFunctionName(_TopFunctionName), last_obj_node(0), gep2pts(0), extinfo(0), WL(0)
{
}

Andersen_AA::~Andersen_AA()
{
   releaseMemory();
}

//------------------------------------------------------------------------------
//Delete all remaining data when our results are no longer needed.
void Andersen_AA::releaseMemory()
{
  run_cleanup();
  for(auto i: boost::irange(0ul,nodes.size()))
  {
    delete nodes[i];
    nodes[i]= 0;
  }
  nodes.clear();
  val_node.clear();
  obj_node.clear();
  ret_node.clear();
  vararg_node.clear();
  tmp_num.clear();
  pts_dom= bddfalse;
  if(gep2pts){
    bdd_freepair(gep2pts);
    gep2pts= 0;
  }
  geps.clear();
  clear_bdd2vec();
  //We should not use bdd_done() here because the clients may still be
  //  using the BDD system.
}

//------------------------------------------------------------------------------
//Initialize all data before starting the run.
void Andersen_AA::run_init()
{
  releaseMemory();
  extinfo= new ExtInfo;
}

//------------------------------------------------------------------------------
//Delete anything not needed to get the analysis results.
void Andersen_AA::run_cleanup()
{
  pre_opt_cleanup();
  constraints.clear();
  ind_calls.clear();
  icall_cons.clear();
  hcd_var.clear();
  off_mask.clear();
  ext_func_nodes= bddfalse;
  ext_func_node_set.clear();
  func_node_set.clear();
  if(extinfo)
  {
    delete extinfo;
    extinfo= 0;
  }
  if(WL)
  {
    delete WL;
    WL= 0;
  }
  lcd_edges.clear();
  lcd_starts.clear();
  lcd_dfs_id.clear();
  while(!lcd_stk.empty())
    lcd_stk.pop();
  lcd_roots.clear();
  ext_seen.clear();
  node_vars.clear();
  ext_failed.clear();
  cplx_cons.clear();
  //Delete the constraint graph and prev_points_to.
  for(auto i: boost::irange(0ul,nodes.size()))
  {
    Node *N= nodes[i];
    N->prev_points_to= bddfalse;
    N->copy_to.clear();
    N->load_to.clear();
    N->store_from.clear();
    N->gep_to.clear();
  }
}
//------------------------------------------------------------------------------
//Delete the points-to sets not needed by the clients.
void Andersen_AA::pts_cleanup()
{
  //BDD id -> first ptr-eq. node.
  llvm::DenseMap<u32, u32> eq;

  for(auto i: boost::irange(0ul,nodes.size()))
  {
    Node *N= nodes[i];
    if(N->obj_sz)
    {
      //Skip Argument objects, which contain top-level pointers
      //  (i.e. the parameters used directly in the function body).
      //Note that an obj. node may have no value if it was merged into
      //  an artificial node.
      if(!N->get_val() || !llvm::isa<llvm::Argument>(N->get_val()))
        N->points_to= bddfalse;
    }

    if (N->points_to != bddfalse)
    {
      u32 idp= N->points_to.id();
      auto j= eq.find(idp);
      if (j == eq.end())
      {
         eq[idp] = i;
      }
      else
      {
         merge_nodes(get_node_rep(i),get_node_rep(j->second));
      }
    }
  }
}


const llvm::Type * Andersen_AA::getmin_struct(llvm::Module &M)
{
   return llvm::Type::getInt8Ty(M.getContext());
}

//------------------------------------------------------------------------------
//add_cons - Add the constraint (t dest src off) to the list, unless:
//  - the constraint is meaningless, like (copy A A 0)
//  - one of the nodes is null
//  - offset is given for addr_of or copy
//Returns true iff it was added.
bool Andersen_AA::add_cons(ConsType t, u32 dest, u32 src, u32 off)
{
   assert(src && dest && "null node in constraint");
   if(t == copy_cons && src == dest)
      return 0;
   if(t == gep_cons && !off)             //replace 0-offset GEP by copy
      t= copy_cons;
   Constraint C(t, dest, src, off);
   switch(t)
   {
      case addr_of_cons:
         assert(dest != i2p);
         assert(!off && "offset not allowed on addr_of_cons");
         break;
      case copy_cons:
         assert(src != i2p && dest != i2p);
         assert(!off && "offset not allowed on copy_cons");
         break;
      case load_cons:
         assert(src != i2p && dest != i2p && src != p_i2p);
         break;
      case store_cons:
         assert(src != i2p && dest != i2p && dest != p_i2p);
         break;
      case gep_cons:
         assert(src != i2p && dest != i2p);
         break;
      default:
         assert(!"unknown constraint type");
   }
   constraints.push_back(C);
   return 1;
}

//------------------------------------------------------------------------------
//Make sure that every node has reasonable info and all values are mapped
//  to the right nodes.
void Andersen_AA::verify_nodes()
{
   llvm::errs() << "***** Checking node info consistency...\n";
   for(auto i: boost::irange(0ul,nodes.size()))
   {
      const Node *N= nodes[i];
      auto V= N->get_val();
      u32 sz= N->obj_sz;
      if(!V)
      {
         assert(!sz || i == i2p && "artificial node has an obj_sz");
         continue;
      }
      u32 vn= get_val_node(V, 1), on= get_obj_node(V, 1), rn= 0, va= 0;
      if(auto F= llvm::dyn_cast<const llvm::Function>(V))
      {
         //Don't use get_ret_node, etc. here -
         //  they will return the obj node for AT func.
         auto it = ret_node.find(F);
         if(it != ret_node.end())
            rn= it->second;
         it= vararg_node.find(F);
         if(it != vararg_node.end())
            va= it->second;
      }
      u32 osz= nodes[on]->obj_sz;
      //This is a value node (including ret/vararg).
      if(i == vn || i == rn || i == va)
         assert((!sz || at_args.count(V)) && "value node has an obj_sz");
      //This node is within the object of its value.
      else if(i < on+osz)
         assert(sz && i+sz <= on+osz && "invalid obj_sz");
      else
         assert(!"node is none of val. obj. or art.");
   }

   for(auto it= val_node.begin(),
       ie= val_node.end(); it != ie; ++it)
   {
      const llvm::Value *V= it->first;
      //The args of addr-taken func. are mapped to the func. obj_nodes instead.
      if(!at_args.count(V))
         assert(V == nodes[it->second]->get_val() && "bad val_node entry");
   }
   for(auto it= obj_node.begin(),
       ie= obj_node.end(); it != ie; ++it)
      assert(it->first == nodes[it->second]->get_val() && "bad obj_node entry");
   for(auto it= ret_node.begin(), ie= ret_node.end(); it != ie; ++it)
      assert(it->first == nodes[it->second]->get_val() && "bad ret_node entry");
   for(auto it= vararg_node.begin(),
       ie= vararg_node.end(); it != ie; ++it)
      assert(it->first == nodes[it->second]->get_val() &&
            "bad vararg_node entry");
}


//Fill in struct_info for T.
//This should only be called by get_struct_info().
void Andersen_AA::analyze_struct(const llvm::StructType *T)
{
   assert(T);
   if(struct_info.count(T))
      return;
   llvm::errs() << "analyze_struct  " << T->getName() << "\n";
   std::vector<u32> sz, off;
   //How many fields have been placed in the expanded struct
   u32 nf= 0;

   for(auto it= T->element_begin(), ie= T->element_end();
       it != ie; ++it)
   {
      const llvm::Type* ET= *it;
      //Treat an array field as a single element of its type.
      while(auto AT = llvm::dyn_cast<const llvm::ArrayType>(ET))
         ET= AT->getElementType();
      //The offset is where this element will be placed in the exp. struct.
      off.push_back(nf);
      //Process a nested struct.
      if(auto ST = llvm::dyn_cast<const llvm::StructType>(ET))
      {
         const std::vector<u32> &szE= get_struct_sz(ST);
         auto nfE= szE.size();
         //Copy ST's info, whose element 0 is the size of ST itself.
         for(auto j: boost::irange(0ul,nfE))
            sz.push_back(szE[j]);
         nf += nfE;
      }
      else
      {                              //simple type
         sz.push_back(1);
         ++nf;
      }
   }
   //Record the size of the complete struct and update max_struct.
   sz[0]= nf;
   if(nf > max_struct_sz)
   {
      max_struct= T;
      max_struct_sz= nf;
   }
   struct_info[T]= std::make_pair(sz, off);
}


//------------------------------------------------------------------------------
//Return the object-node offset corresponding to GEP insn (V).
u32 Andersen_AA::compute_gep_off(const llvm::User *V)
{
   assert(V);
   llvm::errs() << "    (gep  ";
   print_val(V->getOperand(0));
   u32 off= 0;
   for(auto gi= llvm::gep_type_begin(*V), ge= llvm::gep_type_end(*V);
       gi != ge; ++gi)
   {
      //The int-value object of the current index operand
      //  (may not be constant for arrays).
      auto op= llvm::dyn_cast<const llvm::ConstantInt>(gi.getOperand());
      //The actual index
      auto idx= op ? op->getZExtValue() : 0;
      auto ST= llvm::dyn_cast<const llvm::StructType>(gi.getIndexedType());
      //Skip non-struct (i.e. array) offsets
      if(!ST)
         continue;
      assert(op && "non-const struct index in GEP");
      llvm::errs() << "  " << (ST->hasName() ? ST->getName() : "anonStruct") << ":" << idx;
      const std::vector<u32> &so= get_struct_off(ST);
      if(idx >= so.size())
      {
         llvm::errs() << "\n";
         print_struct_info(ST);
         llvm::errs() << "!! Struct index out of bounds: " << idx << "\n";
         assert(0);
      }
      //add the translated offset
      off += so[idx];
   }
   llvm::errs() <<")\n";
   return off;
}

//------------------------------------------------------------------------------
//Find the largest type that this allocation may be cast to.
//This handles both AllocationInst's and allocating calls.
const llvm::Type* Andersen_AA::trace_alloc_type(const llvm::Instruction *I)
{
   assert(I);
   //The largest type seen so far
   const llvm::Type *MT= I->getType()->getContainedType(0);
   auto msz= 0ul;                           //the size of MT (0 for non-struct)
   bool found= 0;                        //if any casts were found

   while(auto AT = llvm::dyn_cast<const llvm::ArrayType>(MT))
      MT= AT->getElementType();
   if(auto ST = llvm::dyn_cast<const llvm::StructType>(MT))
      msz= get_struct_sz(ST).size();

   for(auto it= I->use_begin(), ie= I->use_end(); it != ie; ++it)
   {
      auto CI = llvm::dyn_cast<const llvm::CastInst>(*it);
      //Only check casts to other ptr types.
      if(!CI || !llvm::isa<llvm::PointerType>(CI->getType()))
         continue;
      found= 1;
      //The type we're currently casting to and its size
      const llvm::Type *T= CI->getType()->getContainedType(0);
      auto sz= 0ul;
      while(auto AT= llvm::dyn_cast<const llvm::ArrayType>(T))
         T= AT->getElementType();
      if(auto ST= llvm::dyn_cast<const llvm::StructType>(T))
         sz= get_struct_sz(ST).size();
      if(sz > msz)
      {
         msz= sz;
         MT= T;
      }
   }

   //If the allocation is of non-struct type and we can't find any casts,
   //  assume that it may be cast to the largest struct later on.
   if(!found && !msz)
      return max_struct;
   return MT;
}

//------------------------------------------------------------------------------
//Find the max possible offset for an object pointed to by (V).
size_t Andersen_AA::get_max_offset(const llvm::Value *V)
{
   assert(V);
   llvm::errs() << "        get_max_offset  ";
   print_val(V);
   llvm::errs() << "\n";
   const llvm::Type *T= V->getType();
   assert(llvm::isa<llvm::PointerType>(T) && T->getContainedType(0) == min_struct);
   //If V is a CE or bitcast, the actual pointer type is its operand.
   if(auto E = llvm::dyn_cast<const llvm::ConstantExpr>(V))
      T= E->getOperand(0)->getType();
   else if(auto BI = llvm::dyn_cast<const llvm::BitCastInst>(V))
      T= BI->getOperand(0)->getType();
   //For other values, use the biggest struct type out of all operands.
   else if(auto U = llvm::dyn_cast<const llvm::User>(V))
   {
      llvm::errs() << "          ops:";
      auto msz= 1ul;                         //the max size seen so far
      for(auto it= U->op_begin(), ie= U->op_end(); it != ie; ++it)
      {
         const llvm::Value *V= it->get();
         llvm::errs() << "  ";
         print_val(V);
         T= V->getType();
         if(!llvm::isa<llvm::PointerType>(T))
            continue;
         T= T->getContainedType(0);
         while(auto AT = llvm::dyn_cast<const llvm::ArrayType>(T))
            T= AT->getElementType();
         if(auto ST = llvm::dyn_cast<const llvm::StructType>(T))
         {
            auto sz= get_struct_sz(ST).size();
            if(msz < sz)
               msz= sz;
         }
      }
      llvm::errs() << "\n";
      return msz;
   }
   else                                 //V has no operands
      return 1;

   if(!llvm::isa<llvm::PointerType>(T))
      return 1;
   T= T->getContainedType(0);
   while(auto AT = llvm::dyn_cast<const llvm::ArrayType>(T))
      T= AT->getElementType();
   if(auto ST = llvm::dyn_cast<const llvm::StructType>(T))
      return get_struct_sz(ST).size();
   return 1;
}

//------------------------------------------------------------------------------
//Returns the value node of V, with special handling of const pointers.
u32 Andersen_AA::get_val_node_cptr(const llvm::Value *V)
{
   assert(V);
   u32 vn= get_val_node(V, 1);
   //The value-node map takes priority over the general const.ptr. handling.
   if(vn)
      return vn;

   auto C = llvm::dyn_cast<const llvm::Constant>(V);
   assert(C && llvm::isa<llvm::PointerType>(C->getType()) &&
          "value w/o node is not a const ptr");
   assert(!llvm::isa<llvm::GlobalValue>(C) && "global const.ptr has no node");

   //We don't need constraints for null/undef.
   if(llvm::isa<llvm::ConstantPointerNull>(C) || llvm::isa<llvm::UndefValue>(C))
      return 0;

   auto E = llvm::dyn_cast<const llvm::ConstantExpr>(C);
   assert(E && "unknown const.ptr type");

   switch(E->getOpcode())
   {
      case llvm::Instruction::BitCast:
         return get_val_node_cptr(E->getOperand(0));
      case llvm::Instruction::IntToPtr:
         id_i2p_insn(E);
         return get_val_node(E);
      case llvm::Instruction::GetElementPtr:
         if(llvm::isa<llvm::ConstantPointerNull>(E->getOperand(0)))
         {
            if(E->getNumOperands() > 2)
               return 0;
            id_i2p_insn(E);
            return get_val_node(E);
         }
         else
         {
            llvm::errs() << "CGEP #" <<  next_node << "\n";
            print_val(E);
            llvm::errs() << "\n";
            if(!val_node.count(E))
            {
               u32 vn= next_node++;
               nodes.push_back(new Node(E));
               val_node[E]= vn;
               gep_ce.push_back(vn);
            }
            id_gep_insn(E);
            return get_val_node(E);
         }
      default:
         assert(!"unknown opcode in const.ptr expr");
   }
   return 0;
}


//------------------------------------------------------------------------------
//Check if the address of (V) is ever taken. This can happen if:
//- V is an arg of some function call
//- V is used by an insn. other than compare
//- V is part of a const.expr whose addr. is taken.
bool Andersen_AA::escapes(const llvm::Value *V) const
{
   assert(V);
   for(auto it= V->use_begin(), ie= V->use_end(); it != ie; ++it)
   {
      if(auto I = llvm::dyn_cast<const llvm::CallInst>(*it))
      {
         for(u32 k= 1, ke= I->getNumOperands(); k < ke; ++k)
            if(I->getOperand(k) == V)
               return 1;
      }
      else if(auto I = llvm::dyn_cast<const llvm::InvokeInst>(*it))
      {
         for(u32 k= 3, ke= I->getNumOperands(); k < ke; ++k)
            if(I->getOperand(k) == V)
               return 1;
      }
      else if(auto E = llvm::dyn_cast<const llvm::ConstantExpr>(*it))
      {
         if(escapes(E))
            return 1;
      }
      else if(!llvm::isa<llvm::CmpInst>(*it))
         return 1;
   }
   return 0;
}

//------------------------------------------------------------------------------
//Find the possible pointer sources of the int value (V), storing them in (src).
//Returns true if some path can't be traced to a ptr (and so the i2p cons.
//  should be added).
//(seen) maps the values visited by the current trace to the return value.
//Pass empty sets for both (src) and (seen) when calling from the outside.
//(depth) is the current recursion level; do not set from the outside.
bool Andersen_AA::trace_int(const llvm::Value *V, llvm::DenseSet<const llvm::Value *> &src,
                                llvm::DenseMap<const llvm::Value *, bool> &seen, u32 depth)
{
   auto i_seen= seen.find(V);
   if(i_seen != seen.end())
      return i_seen->second;
   llvm::errs() << "    trace_int[" << depth << "]  ";
   print_val(V);
   llvm::errs() << "\n      ";
   const llvm::Type *TL= V->getType();
   assert(V && llvm::isa<llvm::IntegerType>(TL) &&
          "trying to trace non-int value");
   seen[V]= 0;

   //Opcode/operands of V (which is either insn or const.expr)
   u32 opcode= 0;
   std::vector<const llvm::Value*> ops;

   //Arguments and numbers provide unknown addresses.
   if(llvm::isa<llvm::Argument>(V) || llvm::isa<llvm::ConstantInt>(V))
   {
      llvm::errs() << "<i2p>\n";
      seen[V]= 1;
      return 1;
   }
   else if(auto CE = llvm::dyn_cast<const llvm::ConstantExpr>(V))
   {
      llvm::errs() << "CE";
      opcode= CE->getOpcode();
      for(auto i: boost::irange(0u,CE->getNumOperands()))
         ops.push_back(CE->getOperand(i));
   }
   else if(auto I = llvm::dyn_cast<const llvm::Instruction>(V))
   {
      llvm::errs() << "insn";
      opcode= I->getOpcode();
      for(auto i: boost::irange(0u,I->getNumOperands()))
         ops.push_back(I->getOperand(i));
   }
   else
      assert(!"unknown type of int value");

   assert(opcode);
   llvm::errs() << "  (";
   llvm::errs() << llvm::Instruction::getOpcodeName(opcode);
   for(auto i: boost::irange(0ul,ops.size()))
   {
      llvm::errs() << "  ";
      print_val(ops[i]);
   }
   llvm::errs() << ")\n";

   bool r= 0;
   switch(opcode)
   {
      //These return untraceable int values.
      case llvm::Instruction::Invoke:
      case llvm::Instruction::FPToUI:
      case llvm::Instruction::FPToSI:
      case llvm::Instruction::ICmp:
      case llvm::Instruction::FCmp:
      case llvm::Instruction::Call:
      case llvm::Instruction::VAArg:
      case llvm::Instruction::ExtractElement:
         llvm::errs() << "      <i2p>\n";
         seen[V]= 1;
         return 1;
         //This is the only direct way to get a ptr source.
      case llvm::Instruction::PtrToInt:
         src.insert(ops[0]);
         return 0;
      case llvm::Instruction::Load:
      {
         //Loading from a global constant gives its initializer.
         if(auto G = llvm::dyn_cast<const llvm::GlobalVariable>(ops[0]))
         {
            if(G->hasInitializer() && G->isConstant())
            {
               auto GI= G->getInitializer();
               llvm::errs() << "      global const  ";
               print_val(GI);
               llvm::errs() << "\n";
               r= trace_int(GI, src, seen, depth+1);
               if(r)
                  seen[V]= 1;
               return r;
            }
         }
         //Try to find what was last stored here, within the same basic block.
         llvm::errs() << "      last store  ";
         auto LI0= llvm::cast<const llvm::LoadInst>(V);
         auto addr= ops[0];
         const llvm::Value *S= 0;
         //Whether the BB iterator reached the current insn.
         bool found= 0;
         auto bb= LI0->getParent();
         for(auto it= bb->begin(), ie= bb->end();
             !found && it != ie; ++it)
         {
            if(auto SI = llvm::dyn_cast<const llvm::StoreInst>(it))
            {
               if(SI->getPointerOperand() == addr)
                  S= SI->getOperand(0);
            }
            else if(auto LI= llvm::dyn_cast<const llvm::LoadInst>(it))
               found= LI == LI0;
         }
         assert(found);
         if(S)
         {
            print_val(S);
            llvm::errs() << "\n";
            r= trace_int(S, src, seen, depth+1);
            if(r)
               seen[V]= 1;
            return r;
         }
         llvm::errs() << "<??\?>\n";
         seen[V]= 1;
         return 1;
      }
         //For 1-addr arithmetic or casts, trace the addr operand.
      case llvm::Instruction::Shl:
      case llvm::Instruction::LShr:
      case llvm::Instruction::AShr:
      case llvm::Instruction::Trunc:
      case llvm::Instruction::ZExt:
      case llvm::Instruction::SExt:
      case llvm::Instruction::BitCast:
      {
         const llvm::Type *TR= ops[0]->getType();
         llvm::errs() << "  (cast " << get_type_name(TR) << " -> " <<
                         get_type_name(TL) << "\n";
         if(llvm::isa<llvm::IntegerType>(TR))
         {
            r= trace_int(ops[0], src, seen, depth+1);
            if(r)
               seen[V]= 1;
            return r;
         }
         else
         {
            assert(opcode == llvm::Instruction::BitCast &&
                   "invalid operand for int insn");
            llvm::Type::TypeID t= TR->getTypeID();
            assert(t == llvm::Type::FloatTyID || t == llvm::Type::DoubleTyID &&
                   "invalid cast to int");
            seen[V]= 1;
            return 1;
         }
      }
         //Arithmetic with possibly 2 addr: trace both; add i2p only if both
         //  return i2p (if only 1 has i2p, assume it's an offset for the addr).
      case llvm::Instruction::Add:
      case llvm::Instruction::Sub:
      case llvm::Instruction::Mul:
      case llvm::Instruction::UDiv:
      case llvm::Instruction::SDiv:
      case llvm::Instruction::URem:
      case llvm::Instruction::SRem:
      case llvm::Instruction::And:
      case llvm::Instruction::Or:
      case llvm::Instruction::Xor:
         r= trace_int(ops[0], src, seen, depth+1) &
               trace_int(ops[1], src, seen, depth+1);
         if(r)
            seen[V]= 1;
         return r;
         //Trace all ops into the same set (if any op has i2p,
         //  the result can point to i2p too).
      case llvm::Instruction::PHI:
         r= 0;
         for(auto i: boost::irange(0ul,ops.size()))
         {
            //Sometimes a pointer or other value can come into an int-type phi node.
            const llvm::Type *T= ops[i]->getType();
            if(llvm::isa<llvm::IntegerType>(T))
               r |= trace_int(ops[i], src, seen, depth+1);
            else if(llvm::isa<llvm::PointerType>(T))
               src.insert(ops[i]);
            else
               r= 1;
         }
         if(r)
            seen[V]= 1;
         return r;
      case llvm::Instruction::Select:
         r= trace_int(ops[0], src, seen, depth+1) |
               trace_int(ops[1], src, seen, depth+1);
         if(r)
            seen[V]= 1;
         return r;
      default:
         assert(!"this insn should not produce an int value");
   }
   assert(!"should not get here");
   return 0;
}


//------------------------------------------------------------------------------
// Function processing
//------------------------------------------------------------------------------
//Add the nodes & constraints for the declaration of (F).
void Andersen_AA::id_func(const llvm::Function *F)
{
   assert(F);
   bool AT= escapes(F);        //whether this function's addr. is ever taken
   llvm::errs() << "id_func  ";
   print_val(F);
   llvm::errs() << ":  addr "<< (AT ? "taken" : "not taken");
   u32 vnF= 0, onF= 0;
   //Only make val/obj nodes for addr-taken functions.
   if(AT)
   {
      vnF= next_node++;
      onF= next_node++;
      nodes.push_back(new Node(F));
      //Only 1 obj node for ext.func
      nodes.push_back(new Node(F, 1));
      val_node[F]= vnF;
      obj_node[F]= onF;
      add_cons(addr_of_cons, vnF, onF);
   }
   //Ext. func. should not be analyzed (since they are handled at the call site).
   if(extinfo->is_ext(F))
   {
      llvm::errs() << ", ext\n";
      return;
   }
   llvm::errs() << "\n";
   bool is_va= F->isVarArg();
   bool ptr_ret= llvm::isa<llvm::PointerType>(F->getReturnType());
   //The double-ptr args to main(), argv & envp, are treated as external vars.
   if(F->getName() == TopFunctionName)
   {
      //Assume that the top function is never called indirectly.
      u32 i= 0;
      for(auto& arg : F->args())
      {
         const llvm::Value* argAddr = &arg;
         if(!argAddr->getType()->isPtrOrPtrVectorTy())
            continue;
         //Args 1 (argv) & 2 (envp) need 2 obj nodes, with v -> o0 -> o1.
         u32 vn= next_node++;
         nodes.push_back(new Node(argAddr));
         val_node[argAddr]= vn;
         u32 on= next_node;
         nodes.push_back(new Node(argAddr, 2));
         nodes.push_back(new Node(argAddr, 1));
         next_node += 2;
         obj_node[argAddr]= on;
         add_cons(addr_of_cons, vn, on);
         add_cons(addr_of_cons, on, on+1);
         i++;
      }
   }
   else if(!AT)
   {
      //Make a value node for each ptr arg.
      for(auto& arg : F->args())
      {
         const llvm::Value* argAddr = &arg;
         if(llvm::isa<llvm::PointerType>(argAddr->getType()))
         {
            u32 vn= next_node++;
            nodes.push_back(new Node(argAddr));
            val_node[argAddr]= vn;
         }
      }
      //Make return and vararg nodes, if needed.
      if(ptr_ret)
      {
         u32 rn= next_node++;
         nodes.push_back(new Node(F));
         ret_node[F]= rn;
      }
      if(is_va)
      {
         u32 va= next_node++;
         nodes.push_back(new Node(F));
         vararg_node[F]= va;
      }
   }
   else
   {
      //Map all args to the correct obj nodes
      //  and find where the last ptr arg is.
      //  If there are no ptr args at all, last_ptr will be ~0 rather than 0.
      std::vector<const llvm::Value*> args;
      u32 last_ptr= ~0U, i= 0;
      for(auto& arg : F->args())
      {
         const llvm::Value *V= &arg;
         args.push_back(V);
         val_node[V]= onF + func_node_off_arg0 + i;
         at_args.insert(V);
         if(llvm::isa<llvm::PointerType>(V->getType()))
            last_ptr= i;
         ++i;
      }
      //The return node must go right after the first obj node.
      assert(next_node == onF + func_node_off_ret);
      nodes.push_back(new Node(F, 1));
      ++next_node;
      //If the return is non-ptr, point it to i2p, since indirect calls
      //  can load the retval
      //  from a node pointing to both a ptr-ret func. and a non-ptr-ret one.
      //NOTE: this shouldn't really happen in correct programs
      //if(!ptr_ret)
      //add_cons(addr_of_cons, onF + func_node_off_ret, i2p);
      //Make object nodes for all args up to the last ptr;
      //  their values must be the args themselves (not the function).
      assert(next_node == onF + func_node_off_arg0);
      if(last_ptr != ~0U)
      {
         for(u32 i= 0; i <= last_ptr; ++i)
         {
            nodes.push_back(new Node(args[i], 1));
         }
         next_node += last_ptr+1;
      }
      //Make the vararg node if needed.
      if(is_va)
      {
         nodes.push_back(new Node(F, 1));
         ++next_node;
      }
      //Now we have the complete object of F.
      nodes[onF]->obj_sz= next_node-onF;
      //Note that the args of AT functions are not mapped to value nodes.
   }
}

//------------------------------------------------------------------------------
//Add nodes for any const GEP expressions using the global (G).
void Andersen_AA::id_gep_ce(const llvm::Value *G)
{
   assert(G);
   //Check all GEP and bitcast ConstantExpr's using G.
   for(auto it= G->use_begin(), ie= G->use_end(); it != ie; ++it)
   {
      auto E = llvm::dyn_cast<const llvm::ConstantExpr>(*it);
      if(!E)
         continue;
      //Recursively check the uses of a bitcast.
      if(E->getOpcode() == llvm::Instruction::BitCast)
         id_gep_ce(E);
      else if(E->getOpcode() == llvm::Instruction::GetElementPtr)
      {
         llvm::errs() << "CGEP #" <<  next_node << "\n";
         print_val(E);
         llvm::errs() << "\n";
         //A GEP can only use a pointer as its first op.
         assert(E->getOperand(0) == G && "ptr used as index operand");
         //Make a node for this CGEP and record it to init
         //  after other globals are done.
         assert(!val_node.count(E));
         u32 vn= next_node++;
         nodes.push_back(new Node(E));
         val_node[E]= vn;
         gep_ce.push_back(vn);
      }
   }
}

//Add the nodes & constraints for the declaration of (G).
void Andersen_AA::id_global(const llvm::GlobalVariable *G)
{
   assert(G);
   //Make a node for the global ptr.
   nodes.push_back(new Node(G));
   llvm::errs() << "global #" <<  next_node << "\n";
   print_val(G);
   llvm::errs() << "\n";

   u32 vnG= next_node++;
   val_node[G]= vnG;

   //The type this global points to
   const llvm::Type *T= G->getType()->getContainedType(0);
   bool is_array= 0;
   //An array is considered a single variable of its type.
   while(auto AT = llvm::dyn_cast<const llvm::ArrayType>(T))
   {
      T= AT->getElementType();
      is_array= 1;
   }
   //The first node of the global object (a struct may have more)
   u32 onG= next_node;
   obj_node[G]= onG;

   if(auto ST = llvm::dyn_cast<const llvm::StructType>(T))
   {
      const std::vector<u32> &sz= get_struct_sz(ST);
      auto nf= sz.size();
      //Make nodes for all the fields, with the same obj_sz (array => weak).
      for(auto i: boost::irange(0UL,nf))
         nodes.push_back(new Node(G, sz[i], is_array));
      next_node += nf;
      //A struct may be used in constant GEP expr.
      id_gep_ce(G);
   }
   else
   {
      //Make 1 obj node, with obj size 1.
      nodes.push_back(new Node(G, 1, is_array));
      ++next_node;
      //An array may be used in constant GEP expr.
      if(is_array)
         id_gep_ce(G);
   }

   add_cons(addr_of_cons, vnG, onG);
}


u32 Andersen_AA::get_node_rep(u32 n)
{
   u32 &r0= nodes[n]->rep;
   //If (n) has a rank, it is the rep.
   if(r0 >= node_rank_min)
      return n;
   //Recurse on the parent to get the real rep.
   u32 r= get_node_rep(r0);
   r0= r;                              //set n's parent to the rep
   return r;
}

//const version of the above, w/o path compression.
u32 Andersen_AA::cget_node_rep(u32 n) const
{
   u32 r;
   while((r= nodes[n]->rep) < node_rank_min)
      n= r;
   return n;
}

//------------------------------------------------------------------------------
//Find the node for the return value of (F).
//Returns 0 if F doesn't return a ptr.
u32 Andersen_AA::get_ret_node(const llvm::Function *F) const
{
   assert(F);
   if(!llvm::isa<llvm::PointerType>(F->getReturnType()))
      return 0;
   auto it= ret_node.find(F);
   if(it == ret_node.end())
   {
      //Addr-taken func. have obj nodes in place of the ret/vararg nodes.
      u32 on= get_obj_node(F, 1);
      assert(on && "missing ret_node entry");
      return on + func_node_off_ret;
   }
   u32 rn= it->second;
   assert(rn && "ret_node map has a 0 entry");
   return rn;
}

//Find the node for the vararg part of (F).
//Returns 0 if F is not vararg.
u32 Andersen_AA::get_vararg_node(const llvm::Function *F) const
{
   assert(F);
   if(!F->getFunctionType()->isVarArg())
      return 0;
   auto it= vararg_node.find(F);
   if(it == vararg_node.end())
   {
      u32 on= get_obj_node(F, 1);
      assert(on && "missing vararg_node entry");
      return on + nodes[on]->obj_sz - 1;
   }
   u32 va= it->second;
   assert(va && "vararg_node map has a 0 entry");
   return va;
}

//------------------------------------------------------------------------------
//Return the name of type (T).
std::string Andersen_AA::get_type_name(const llvm::Type *T) const
{
   assert(T);
   if(llvm::isa<llvm::StructType>(T))
   {
      auto ST = llvm::dyn_cast<const llvm::StructType>(T);
      if(ST && ST->hasName())
         return ST->getName();
      return "<anon.struct>";
   }
   if(T->isSingleValueType())
   {
      if(T->isVoidTy())
         return "void";
      else if(T->isHalfTy())
         return "_Float16";
      else if(T->isFloatTy())
         return "float";
      else if(T->isDoubleTy())
         return "double";
      else if(T->isX86_FP80Ty())
         return "long double";
      else if(T->isFP128Ty())
         return "long double";
      else if(T->isIntegerTy())
      {
         if(T->getScalarSizeInBits()==1)
            return "_Bool";
         else if(T->getScalarSizeInBits()>1 && T->getScalarSizeInBits()<=8)
            return "unsigned char";
         else if(T->getScalarSizeInBits()>8 && T->getScalarSizeInBits()<=16)
            return "unsigned short int";
         else if(T->getScalarSizeInBits()>16 && T->getScalarSizeInBits()<=32)
            return "unsigned int";
         else if(T->getScalarSizeInBits()>32 && T->getScalarSizeInBits()<=64)
            return "unsigned long long int";
         else
            llvm_unreachable("not expected integer bitwidth size");
      }
   }
   return "<??\?>";
}

//------------------------------------------------------------------------------
void Andersen_AA::print_struct_info(const llvm::Type *T) const
{
   assert(T);
   auto ST= llvm::dyn_cast<const llvm::StructType>(T);
   if(!ST)
   {
      llvm::errs() <<"--- (not a struct) ---\n";
      return;
   }
   llvm::errs() <<"--- ";
   llvm::errs() << get_type_name(ST) <<" ---\nsz=";
   auto it= struct_info.find(ST);
   assert(it != struct_info.end());
   const std::pair<std::vector<u32>, std::vector<u32> > &info= it->second;
   const std::vector<u32> &sz= info.first, &off= info.second;
   for(auto i: boost::irange(0ul,sz.size()))
      llvm::errs() << " " << sz[i];
   llvm::errs()<<"\noff=";
   for(auto i: boost::irange(0ul,sz.size()))
      llvm::errs() << " "<< off[i];
   llvm::errs()<<'\n';
}

//------------------------------------------------------------------------------
void Andersen_AA::print_all_structs() const
{
   llvm::errs() << "==========  Struct info  ===================================\n";
   for(auto it= struct_info.begin(),
       ie= struct_info.end(); it != ie; ++it)
      print_struct_info(it->first);
}


//Max. length of the printed name of a single object, max length for a complete
//  value or node printout, and the width of the left column for some formats.
const u32 max_name_len= 1<<8, max_val_len= 1<<12, max_node_len= 1<<13,
lhs_width= 40;

//------------------------------------------------------------------------------
//(n) - the node ID that will be printed for some types (unless 0).
//(const_with_val) - print the values of constants (default yes).
//(first) is used for recursion; do not set when calling from outside.
void Andersen_AA::print_val(const llvm::Value *V, u32 n, bool const_with_val, bool first) const
{
   assert(V);
   //With a single value, the line may only reach this size.

   //Print the parent function for an insn or arg.
   auto I = llvm::dyn_cast<const llvm::Instruction>(V);
   if(I)
   {
      llvm::errs() << I->getParent()->getParent()->getName();
      llvm::errs() <<':';
   }
   else if(auto A= llvm::dyn_cast<const llvm::Argument>(V))
   {
      llvm::errs() << A->getParent()->getName();
      llvm::errs() <<':';
   }

   if(V->hasName())
   {
      llvm::errs() << V->getName();
   }
   else if(I)
   {
      if(n && first)
         llvm::errs() << "<insn#"<< n <<".";
      else
         llvm::errs() << "<insn.";
      llvm::errs() << I->getOpcodeName();
      if(I->getType()->getTypeID() != llvm::Type::VoidTyID)
      {
         u32 instID;
         if(!tmp_num.count(I))
         {
            auto F = I->getFunction();
            llvm::ModuleSlotTracker MST(F->getParent());
            MST.incorporateFunction(*F);
            instID= static_cast<u32>(MST.getLocalSlot(I));
         }
         else
            instID = tmp_num.find(I)->second;
         llvm::errs() << "|'%"<<instID;
      }
      else
         llvm::errs() << "??\?";
      llvm::errs() << '>';
   }
   else if(auto C= llvm::dyn_cast<const llvm::Constant>(V))
   {
      print_const(C, n, const_with_val, first);
   }
   else if(I)
   {
      if(n && first)
         llvm::errs() << "<insn#"<< n <<".";
      else
         llvm::errs() << "<insn.";
      llvm::errs() << I->getOpcodeName();
      llvm::errs() << '>';
   }
   else
   {
      if(n && first)
         llvm::errs() << "<??\?#" << n << ">";
      else
         llvm::errs() << "<??\?>";
   }
}

//------------------------------------------------------------------------------
//For use by print_val only
void Andersen_AA::print_const(const llvm::Constant *C, u32 n, bool const_with_val, bool first) const
{
   assert(C);
   if(n && first)
      llvm::errs() << "<const#"<< n << ".";
   else
      llvm::errs() << "<const.";
   if(auto K = llvm::dyn_cast<const llvm::ConstantInt>(C))
   {
      if(const_with_val)
         llvm::errs() << "int>(" << K->getSExtValue() << ")";
      else
         llvm::errs() <<"int>";
   }
   else if(llvm::isa<llvm::ConstantFP>(C))
      llvm::errs() <<"FP>";
   else if(llvm::isa<llvm::ConstantAggregateZero>(C))
      llvm::errs() <<"aggregate(0)>";
   else if(llvm::isa<llvm::ConstantVector>(C))
      llvm::errs() <<"vector>";
   else if(llvm::isa<llvm::ConstantPointerNull>(C))
      llvm::errs() <<"ptr(0)>";
   else if(llvm::isa<llvm::UndefValue>(C))
      llvm::errs() <<"undef>";
   else if(auto K = llvm::dyn_cast<const llvm::ConstantArray>(C))
   {
      if(const_with_val)
      {
         std::string tn= get_type_name(K->getType()->getElementType());
         auto ne= K->getType()->getNumElements();
         llvm::errs() <<"array>(";
         llvm::errs() << tn;
         llvm::errs() << "[" << ne << "])";
      }
      else
         llvm::errs() <<"array>";
   }
   else if(auto K = llvm::dyn_cast<const llvm::ConstantStruct>(C))
   {
      if(const_with_val)
      {
         std::string tn= get_type_name(K->getType());
         llvm::errs() <<"struct>(";
         llvm::errs() << tn;
         llvm::errs() <<')';
      }
      else
         llvm::errs() <<"struct>";
   }
   else if(auto K = llvm::dyn_cast<const llvm::ConstantDataSequential>(C))
   {
      if(const_with_val)
      {
         std::string tn= get_type_name(K->getElementType());
         llvm::errs() <<"Sequential>(";
         llvm::errs() << tn;
         llvm::errs() <<')';
      }
      else
         llvm::errs() <<"Sequential>";
   }
   else if(auto E = llvm::dyn_cast<const llvm::ConstantExpr>(C))
   {
      llvm::errs() <<"expr>";
      if(const_with_val)
         print_const_expr(E);
   }
   else
      llvm::errs() <<"??\?>";
}

//------------------------------------------------------------------------------
//For use by print_val only
void Andersen_AA::print_const_expr(const llvm::ConstantExpr *E) const
{
   assert(E);
   llvm::errs() <<'(';
   llvm::errs()<<E->getOpcodeName();
   for(auto it= E->op_begin(); it != E->op_end(); ++it)
   {
      llvm::errs() <<' ';
      //We can only get here if const_with_val is 1.
      //Also this is a recursive call (first=0),
      //  and we no longer need the node IDs.
      print_val(*it, 0, 1, 0);
   }
   llvm::errs() <<')';
}

//------------------------------------------------------------------------------
//Print the node with # (n) (either its value or a description with its number).
//  The printout is extended with dots on the right up to (width) (default 0).
void Andersen_AA::print_node(u32 n) const
{

   switch(n)
   {
      case 0:
         llvm::errs()<<"<none>";
         break;
      case i2p:
         llvm::errs()<<"<i2p>";
         break;
      case p_i2p:
         llvm::errs()<<"<p_i2p>";
         break;
      default:
         const llvm::Value *V= nodes[n]->get_val();
         //Obj node for current value and its obj size (both 0 if V is null)
         u32 on= V ? get_obj_node(V, 1) : 0;
         u32 sz= nodes[on]->obj_sz;
         if(!V)
            llvm::errs() << "<artificial#" << n << ">";
         else if(auto F= llvm::dyn_cast<const llvm::Function>(V))
         {
            llvm::errs() << F->getName();
            if(n == get_ret_node(F))
               llvm::errs() <<"<retval>";
            else if(n == get_vararg_node(F))
               llvm::errs() <<"<vararg>";
            //A function's value node is its original pointer.
            else if(n == get_val_node(F))
               llvm::errs() <<"<fptr>";
            //If it's an object node, mark it with the offset from the obj. start.
            else if(n-on < sz)
               llvm::errs() << "/" << n-on;
            //Unknown function node
            else
               llvm::errs() <<"<??\?>";
         }
         else
         {
            print_val(V, n, 0);            //0 to omit the values of constant nodes
            if(n-on < sz)
               llvm::errs()<< "/" << n-on;
         }
   }
}

//------------------------------------------------------------------------------
void Andersen_AA::print_all_nodes() const
{
   llvm::errs() << "==========  Node list  =====================================\n";
   std::vector<std::string> lines;
   for(auto i: boost::irange(0ul,nodes.size()))
   {
      const Node *N= nodes[i];
      llvm::errs() << " #"<< i << ", sz= " << N->obj_sz << "\n";
      print_node(i);
   }
}


//------------------------------------------------------------------------------
void Andersen_AA::print_constraint(const Constraint &C) const
{
   if(C.type == store_cons)
      llvm::errs() <<"*";
   llvm::errs()<<'(';
   print_node(C.dest);
   llvm::errs()<<')';
   if(C.type == store_cons && C.off)
      llvm::errs() <<" + " << C.off;
   llvm::errs()<<" = ";
   if(C.type == addr_of_cons)
      llvm::errs()<<'&';
   else if(C.type == load_cons)
      llvm::errs()<<"*";
   llvm::errs()<<'(';
   print_node(C.src);
   llvm::errs()<<')';
   if(C.type != store_cons && C.off)
      llvm::errs() << " + " << C.off;
}

void Andersen_AA::print_all_constraints() const
{
   llvm::errs() << "==========  Constraint list  ===============================\n";
   std::vector<std::string> lines;
   for(auto i: boost::irange(0ul,constraints.size()))
   {
      const Constraint &C= constraints[i];
      print_constraint(C);
      llvm::errs() <<"\n";
   }
}

//------------------------------------------------------------------------------
//Print all nodes with their points-to sets. If (points_to_only) is false,
//  the copy edges and complex constraints are also printed.
//If (O) is not null, the output goes there instead of stderr.
void Andersen_AA::print_cons_graph(bool points_to_only) const
{
   const char *header=
         "==========  Constraint graph ==============================\n";
   if(points_to_only)
      header= "==========  Points-to graph  ===============================\n";
   llvm::errs()<<header;
   std::vector<std::string> lines;
   for(auto i: boost::irange(0ul,nodes.size()))
   {
      if(!points_to_only && !nodes[i]->is_rep())
         continue;
      print_node(i);
      llvm::errs()<<"  ->";
      //If node #i was merged, print the edges and constraints of the rep node.
      const Node *N= nodes[cget_node_rep(i)];
      const std::vector<u32> *pts= bdd2vec(N->points_to);
      for(auto i: boost::irange(0ul,pts->size()))
      {
         llvm::errs()<<"  ";
         print_node((*pts)[i]);
      }
      if(!points_to_only)
      {
         if(!N->copy_to.empty())
         {
            llvm::errs()<<"\n    COPY:";
            for(auto it= N->copy_to.begin(), ie= N->copy_to.end();
                it != ie; ++it)
            {
               llvm::errs()<<"  ";
               print_node(*it);
            }
         }
         if(!N->load_to.empty())
         {
            llvm::errs()<<"\n    LOAD:";
            print_node_cons(N->load_to);
         }
         if(!N->store_from.empty())
         {
            llvm::errs()<<"\n    STORE:";
            print_node_cons(N->store_from);
         }
         if(!N->gep_to.empty())
         {
            llvm::errs()<<"\n    GEP:";
            print_node_cons(N->gep_to);
         }
      }
      llvm::errs()<<'\n';
   }
}

//------------------------------------------------------------------------------
void Andersen_AA::print_node_cons(const bitmap &L) const
{
   for(auto it= L.begin(), ie= L.end(); it != ie; ++it)
   {
      llvm::errs()<<"  ";
      const Constraint &C= cplx_cons[*it];
      switch(C.type)
      {
         case load_cons:
            print_node(C.dest);
            break;
         case store_cons:
            print_node(C.src);
            break;
         case gep_cons:
            print_node(C.dest);
            break;
         default:
            assert(!"not a complex constraint");
      }
      if(C.off)
         llvm::errs() << " +"<< C.off;
   }
}

//------------------------------------------------------------------------------
void Andersen_AA::print_metrics() const
{
  auto nn= nodes.size();
  //_uniq doesn't count the same points_to set more than once;
  //  the difference is a measure of remaining pointer equivalence.
  u32 n_pts= 0, n_pts_uniq= 0;
  unsigned long long sum_pts= 0, sum_pts_uniq= 0;
  std::set<u32> pts_seen;
  for(auto i: boost::irange(0ul,nn))
  {
    const Node *N= nodes[i];
    u32 sz= (u32)bdd_satcountset(N->points_to, pts_dom);
    if(!sz)
      continue;
    assert(N->is_rep() && "non-rep node has a points_to");
    assert(!N->nonptr && "non-pointer has a points_to");
    ++n_pts;
    sum_pts += sz;
    if(!pts_seen.count(N->points_to.id()))
    {
      pts_seen.insert(N->points_to.id());
      ++n_pts_uniq;
      sum_pts_uniq += sz;
    }
  }
  pts_seen.clear();
  double avg_pts= n_pts ? sum_pts/(double)n_pts : 0,
      avg_pts_uniq= n_pts_uniq ? sum_pts_uniq/(double)n_pts_uniq : 0;
  llvm::errs() << "Points-to edges: "<<sum_pts<<" in "<<n_pts<<" sets, avg "<<avg_pts<<"\n";
  llvm::errs() << "- unique points-to: "<<sum_pts_uniq<<" in "<<n_pts_uniq<<", avg "<<avg_pts_uniq<<"\n";
}


//------------------------------------------------------------------------------
//Print a list of pointer-relevant external functions
//  that are not listed in extinfo.cpp.
void Andersen_AA::list_ext_unknown(const llvm::Module &M) const
{
   assert(extinfo);
   std::vector<std::string> names;
   for(auto it= M.begin(), ie= M.end(); it != ie; ++it)
   {
      if(it->isDeclaration() || it->isIntrinsic())
      {
         bool rel= 0;
         if(llvm::isa<llvm::PointerType>(it->getReturnType()))
         {
            rel= 1;
         }
         else for(auto j= it->arg_begin(), je= it->arg_end(); j != je; ++j)
         {
            if(llvm::isa<llvm::PointerType>(j->getType()))
            {
               rel= 1;
               break;
            }
         }
         if(rel && extinfo->get_type(&*it) == EFT_OTHER)
         {
            names.push_back(it->getName());
         }
      }
   }

   sort(names.begin(), names.end());
   if(names.size())
      llvm::errs() << "!! Unknown ext. calls:\n";
   for(auto i: boost::irange(0ul,names.size()))
   {
      llvm::errs() << names[i] << "\n";
   }
}


//------------------------------------------------------------------------------
//Add constraints for the GEP CE at node vnE.
void Andersen_AA::proc_gep_ce(u32 vnE)
{
   const llvm::ConstantExpr *E= llvm::dyn_cast_or_null<const llvm::ConstantExpr>(nodes[vnE]->get_val());
   assert(E);
   assert(E->getOpcode() == llvm::Instruction::GetElementPtr);
   if(global_init_done.count(vnE))
      return;
   global_init_done[vnE]= 1;

   const llvm::Value *R= E->getOperand(0);
   //Strip bitcasts from the RHS, until we get to GEP, i2p, or non-CE value.
   bool nested= 0;
   for(auto ER= llvm::dyn_cast<const llvm::ConstantExpr>(R); ER && !nested;
       ER= llvm::dyn_cast_or_null<const llvm::ConstantExpr>(R))
   {
      switch(ER->getOpcode())
      {
         case llvm::Instruction::BitCast:
            R= ER->getOperand(0);
            break;
         case llvm::Instruction::IntToPtr:
            add_cons(addr_of_cons, vnE, i2p);
            return;
         case llvm::Instruction::GetElementPtr:
            //We can have (gep (bitcast (gep X 1)) 1); the inner gep
            //  must be handled recursively.
            nested= 1;
            break;
         default:
            assert(!"unexpected constant expr type");
      }
   }
   assert(!llvm::isa<llvm::ConstantPointerNull>(R) &&
          "GEP of null not supported for global init");
   llvm::errs() << "  CGEP  \n";
   print_val(E);
   llvm::errs() << "\n";

   //This may be the first time we reach R.
   u32 vnR= get_val_node(R, 1);
   if(!vnR)
   {
      llvm::errs() << "CE #" <<  next_node;
      print_val(R);
      llvm::errs() << "\n";
      vnR= next_node++;
      nodes.push_back(new Node(R));
      val_node[R]= vnR;
   }
   u32 off= compute_gep_off(E);

   //For E = (gep R off), the constraint is (gep E R off).
   add_cons(gep_cons, vnE, vnR, off);
   if(nested)
   {
      proc_gep_ce(vnR);
   }
   else if(auto RG = llvm::dyn_cast<const llvm::GlobalVariable>(R))
   {
      //R itself may need global_init if it's a global var.
      if(RG->hasInitializer())
      {
         proc_global_init(get_obj_node(R), RG->getInitializer());
      }
      else
      {
         llvm::errs() << "!! uninitialized global in CGEP\n";
      }
   }
}

//------------------------------------------------------------------------------
//Add constraints for the global with obj. node (onG) and initializer (C).
//Returns the # of fields processed.
//(first) is used for recursion.
u32 Andersen_AA::proc_global_init(u32 onG, const llvm::Constant *C, bool first)
{
   assert(onG && C);
   auto i_gid= global_init_done.find(onG);
   if(i_gid != global_init_done.end())
      return i_gid->second;

   if(first)
   {
      llvm::errs() << "global_init  ";
      print_node(onG);
      llvm::errs() << " <= ";
      print_val(C);
      llvm::errs() << "\n";
   }

   //Strip bitcast expr from C, until we get to non-expr value or GEP;
   //  set C=0 in case of int->ptr (which we don't trace for globals)
   //  and exit on a ptr->int (a non-ptr single value).
   bool done= 0;
   for(auto E= llvm::dyn_cast<const llvm::ConstantExpr>(C); E && !done;
       E= llvm::dyn_cast_or_null<const llvm::ConstantExpr>(C))
   {
      switch(E->getOpcode())
      {
         case llvm::Instruction::BitCast:
            C= E->getOperand(0);
            break;
         case llvm::Instruction::GetElementPtr:
            done= 1;
            break;
         case llvm::Instruction::IntToPtr:
            C= 0;
            break;
         case llvm::Instruction::PtrToInt:
            if(first)
               global_init_done[onG]= 1;
            return 1;
         default:
            assert(!"unexpected constant expr type");
      }
   }

   if(!C)
   {
      add_cons(addr_of_cons, onG, i2p);
      //Don't mark it as done until the top-level call exits.
      if(first)
      {
         llvm::errs() <<"  <i2p>\n";
         global_init_done[onG]= 1;
      }
      return 1;
   }
   //No constraint for null/undef init
   if(C->isNullValue() || llvm::isa<llvm::UndefValue>(C))
   {
      if(first)
         global_init_done[onG]= 1;
      return 1;
   }

   //single-value init
   if(C->getType()->isSingleValueType())
   {
      if(llvm::isa<llvm::PointerType>(C->getType()))
      {
         if(auto E= llvm::dyn_cast<const llvm::ConstantExpr>(C))
         {
            //The expr. itself may need initialization;
            //  then it can be copied to the global.
            //This may be the first time we reach E.
            u32 vnE= get_val_node(E, 1);
            if(!vnE)
            {
               llvm::errs() << "CE #%u:  " << next_node << "\n";
               print_val(E);
               llvm::errs() << "\n";
               vnE= next_node++;
               nodes.push_back(new Node(E));
               val_node[E]= vnE;
            }
            proc_gep_ce(vnE);
            add_cons(copy_cons, onG, vnE);
         }
         else
         {
            //This must be a global ptr initialized above the current one.
            u32 onC= get_obj_node(C);
            add_cons(addr_of_cons, onG, onC);
         }
      }
      if(first)
         global_init_done[onG]= 1;
      return 1;
   }

   u32 off= 0;
   if(auto CS= llvm::dyn_cast<const llvm::ConstantStruct>(C))
   {
      //Recursively copy each field of the original struct into the next available
      //  field of the expanded struct. Note that the fields of a constant struct
      //  are accessed by getOperand().
      for(auto i: boost::irange(0u,CS->getNumOperands()))
         off += proc_global_init(onG+off, CS->getOperand(i), 0);
   }
   else if(auto CA= llvm::dyn_cast<const llvm::ConstantArray>(C))
   {
      //Copy each array element into the same node.
      //The offset returned (the field count of 1 el.)
      //  will be the same every time.
      for(auto i: boost::irange(0u,CA->getNumOperands()))
         off= proc_global_init(onG, CA->getOperand(i), 0);
   }
   else if(auto CA= llvm::dyn_cast<const llvm::ConstantDataSequential>(C))
   {
      for(auto i: boost::irange(0u,CA->getNumElements()))
         off= proc_global_init(onG, CA->getElementAsConstant(i), 0);
   }
   else
      assert(!"unexpected non-1st-class constant");

   if(first)
      global_init_done[onG]= off;
   return off;
}



//------------------------------------------------------------------------------
// Instruction handlers
//------------------------------------------------------------------------------

void Andersen_AA::id_ret_insn(const llvm::Instruction *I)
{
   assert(I);
   auto RI= llvm::cast<const llvm::ReturnInst>(I);
   //no val_node for RI

   //ignore void and non-ptr return statements
   if(!RI->getNumOperands())
      return;
   const llvm::Value *src= RI->getOperand(0);
   if(!llvm::isa<llvm::PointerType>(src->getType()))
      return;

   auto F= RI->getParent()->getParent();
   llvm::errs() << "  id_ret_insn  ";
   print_val(F);
   llvm::errs() << " <= ";
   print_val(src);
   llvm::errs() << "\n";

   u32 rnF= get_ret_node(F), vnS= get_val_node_cptr(src);
   assert(rnF);
   //vnS may be null if src is a null ptr
   if(vnS)
      add_cons(copy_cons, rnF, vnS);
}

//------------------------------------------------------------------------------
//Note: this handles both call and invoke
void Andersen_AA::id_call_insn(const llvm::Instruction *I)
{
   assert(I);
   llvm::ImmutableCallSite CS(I);                       //this will assert the correct opcode
   u32 vnI= get_val_node(I, 1);
   //val_node may be 0 if the call returns non-ptr.

   llvm::errs() << "  id_call_insn  ";
   print_val(I);
   llvm::errs() << "\n";

   const llvm::Value *callee= CS.getCalledValue();
   auto F= llvm::dyn_cast<const llvm::Function>(callee);
   if(!F)
   {
      //Try to recover the original function pointer from a bitcast.
      auto E= llvm::dyn_cast<const llvm::ConstantExpr>(callee);
      if(E && E->getOpcode() == llvm::Instruction::BitCast)
         F= llvm::dyn_cast<const llvm::Function>(E->getOperand(0));
   }

   if(vnI)
      id_call_obj(vnI, F);

   if(F)
   {
      if(extinfo->is_ext(F))
         id_ext_call(CS, F);
      else
         id_dir_call(CS, F);
   }
   else
      //If the callee was not identified as a function (null F), this is indirect.
      id_ind_call(CS);
}

//------------------------------------------------------------------------------

void Andersen_AA::id_malloc_insn(const llvm::Instruction *I)
{
   assert(I);
   auto AI= llvm::cast<const llvm::AllocaInst>(I);
   u32 vnI= get_val_node(AI);

   llvm::errs() << "  id_malloc_insn  ";
   print_val(AI);
   llvm::errs() << "\n";

   const llvm::Type *T= 0;
   //heap-allocated or array => weak
   bool weak= 1;
   //Find out which type of data was allocated.

   T= trace_alloc_type(I);


   u32 on= next_node;
   obj_node[AI]= on;
   if(auto ST= llvm::dyn_cast<const llvm::StructType>(T))
   {
      const std::vector<u32> &sz= get_struct_sz(ST);
      auto nf= sz.size();
      //Make nodes for all the fields, with the same obj_sz.
      for(auto i: boost::irange(0ul,nf))
      {
         nodes.push_back(new Node(AI, sz[i], weak));
      }
      next_node += nf;
   }
   else
   {
      //Non-struct is 1 field.
      nodes.push_back(new Node(AI, 1, weak));
      ++next_node;
   }
   add_cons(addr_of_cons, vnI, on);
}

void Andersen_AA::id_alloc_insn(const llvm::Instruction *I)
{
   assert(I);
   auto AI= llvm::cast<const llvm::AllocaInst>(I);
   u32 vnI= get_val_node(AI);

   llvm::errs() << "  id_alloc_insn  ";
   print_val(AI);
   llvm::errs() << "\n";

   const llvm::Type *T= 0;
   //heap-allocated or array => weak
   bool weak= 0;
   //Find out which type of data was allocated.

   T= AI->getAllocatedType();
   //An array is considered the same as 1 element.
   while(auto AT= llvm::dyn_cast<const llvm::ArrayType>(T))
   {
      weak= 1;
      T= AT->getElementType();
   }

   u32 on= next_node;
   obj_node[AI]= on;
   if(auto ST= llvm::dyn_cast<const llvm::StructType>(T))
   {
      const std::vector<u32> &sz= get_struct_sz(ST);
      auto nf= sz.size();
      //Make nodes for all the fields, with the same obj_sz.
      for(auto i: boost::irange(0ul,nf))
      {
         nodes.push_back(new Node(AI, sz[i], weak));
      }
      next_node += nf;
   }
   else
   {
      //Non-struct is 1 field.
      nodes.push_back(new Node(AI, 1, weak));
      ++next_node;
   }
   add_cons(addr_of_cons, vnI, on);
}

//------------------------------------------------------------------------------
void Andersen_AA::id_load_insn(const llvm::Instruction *I)
{
   assert(I);
   auto LI= llvm::cast<const llvm::LoadInst>(I);
   u32 vnI= get_val_node(LI);

   llvm::errs() << "  id_load_insn  ";
   print_val(LI);
   llvm::errs() << "\n";

   u32 vnS= get_val_node_cptr(LI->getOperand(0));
   if(!vnS)
   {
      llvm::errs() << "!! load from null:  ";
      print_val(LI);
      llvm::errs() << "  <=  ";
      print_val(LI->getOperand(0));
      return;
   }
   add_cons(load_cons, vnI, vnS);
}

//------------------------------------------------------------------------------
void Andersen_AA::id_store_insn(const llvm::Instruction *I)
{
   assert(I);
   auto SI= llvm::cast<const llvm::StoreInst>(I);
   //no val_node for SI

   llvm::Value *dest= SI->getOperand(1), *src= SI->getOperand(0);
   //ignore stores of non-ptr values
   if(!llvm::isa<llvm::PointerType>(src->getType()))
      return;

   llvm::errs() << "  id_store_insn  ";
   print_val(dest);
   llvm::errs() << "  <=  ";
   print_val(src);
   llvm::errs() << "\n";

   u32 vnD= get_val_node_cptr(dest), vnS= get_val_node_cptr(src);
   if(vnS && vnD)                        //either may be a null ptr
      add_cons(store_cons, vnD, vnS);
}

//------------------------------------------------------------------------------
void Andersen_AA::id_gep_insn(const llvm::User *gep)
{
   auto GO = llvm::dyn_cast<const llvm::GEPOperator>(gep);
   assert(GO);
   u32 vnI= get_val_node(GO);

   llvm::errs() << "  id_gep_insn  ";
   print_val(GO);
   llvm::errs() << "\n";

   auto S= GO->getOperand(0);
   if(llvm::isa<llvm::ConstantPointerNull>(S))
   {
      if(GO->getNumOperands() == 2)
         id_i2p_insn(GO);
      //A multi-index GEP of null is not a replacement for i2p, and so
      //  the result may be considered null.
      return;
   }
   u32 vnS= get_val_node_cptr(S);
   assert(vnS && "non-null GEP operand has no node");
   u32 off= compute_gep_off(GO);
   add_cons(gep_cons, vnI, vnS, off);
}

//------------------------------------------------------------------------------
//Handle IntToPtr insn. and const.expr, as well as insn/expr
//  in the form (GEP null X), which are equivalent.
void Andersen_AA::id_i2p_insn(const llvm::Value *V)
{
   assert(V);
   llvm::errs() << "  id_i2p_insn  ";
   print_val(V);
   llvm::errs() << "\n";
   u32 vnD= 0;
   llvm::Value *op= 0;
   if(auto II= llvm::dyn_cast<const llvm::IntToPtrInst>(V))
   {
      vnD= get_val_node(II);
      op= II->getOperand(0);
   }
   else if(auto GI= llvm::dyn_cast<const llvm::GetElementPtrInst>(V))
   {
      assert(llvm::isa<llvm::ConstantPointerNull>(GI->getOperand(0)) &&
             GI->getNumOperands() == 2 &&
             "only single-index GEP of null is used for i2p");
      vnD= get_val_node(GI);
      op= GI->getOperand(1);
   }
   else if(auto E= llvm::dyn_cast<const llvm::ConstantExpr>(V))
   {
      //A const.expr should not have a node yet.
      assert(!val_node.count(E));
      vnD= next_node++;
      nodes.push_back(new Node(E));
      val_node[E]= vnD;
      if(E->getOpcode() == llvm::Instruction::IntToPtr)
      {
         op= E->getOperand(0);
      }
      else if(E->getOpcode() == llvm::Instruction::GetElementPtr)
      {
         assert(llvm::isa<llvm::ConstantPointerNull>(E->getOperand(0)) &&
                E->getNumOperands() == 2 &&
                "only single-index GEP of null is used for i2p");
         op= E->getOperand(1);
      }
      else
         assert(!"const.expr is not i2p or gep");
   }
   else
      assert(!"value is not i2p, gep, or const.expr");

   llvm::DenseSet<const llvm::Value*> src;
   llvm::DenseMap<const llvm::Value*, bool> seen;
   bool has_i2p= trace_int(op, src, seen);
   llvm::errs() << "    src:";
   for(auto it= src.begin(), ie= src.end(); it != ie; ++it)
   {
      const llvm::Value *S= *it;
      llvm::errs() << "  ";
      print_val(S);
      u32 vnS= get_val_node_cptr(S);
      if(vnS)
         add_cons(copy_cons, vnD, vnS);
   }
   if(has_i2p)
   {
      llvm::errs() << "  <i2p>";
      add_cons(addr_of_cons, vnD, i2p);
   }
   llvm::errs() << "\n";
}

//------------------------------------------------------------------------------
void Andersen_AA::id_bitcast_insn(const llvm::Instruction *I)
{
   assert(I);
   auto BI= llvm::cast<const llvm::BitCastInst>(I);
   u32 vnI= get_val_node(BI);

   llvm::errs() << "  id_bitcast_insn  ";
   print_val(BI);
   llvm::errs() << "\n";

   llvm::Value *op= BI->getOperand(0);
   //Bitcast can only convert ptr->ptr or num->num.
   assert(llvm::isa<llvm::PointerType>(op->getType()));
   u32 vnS= get_val_node_cptr(op);
   if(vnS)                               //src may be a null ptr
      add_cons(copy_cons, vnI, vnS);
}

//------------------------------------------------------------------------------
void Andersen_AA::id_phi_insn(const llvm::Instruction *I)
{
   assert(I);
   auto PN= llvm::cast<const llvm::PHINode>(I);
   u32 vnI= get_val_node(PN);

   llvm::errs() << "  id_phi_insn  ";
   print_val(PN);
   llvm::errs() << "\n";

   for(auto i: boost::irange(0u,PN->getNumIncomingValues()))
   {
      auto incoming = PN->getIncomingValue(i);
      u32 vnS = get_val_node_cptr(incoming);
      if(vnS)                             //src may be a null ptr
         add_cons(copy_cons, vnI, vnS);
   }
}

//------------------------------------------------------------------------------
void Andersen_AA::id_select_insn(const llvm::Instruction *I)
{
   assert(I);
   auto SI= llvm::cast<const llvm::SelectInst>(I);
   u32 vnI= get_val_node(SI);

   llvm::errs() << "  id_select_insn  ";
   print_val(SI);
   llvm::errs() << "\n";

   //(select a0 a1 a2) = (a0 ? a1 : a2).
   u32 vnS1= get_val_node_cptr(SI->getOperand(1)), vnS2=
         get_val_node_cptr(SI->getOperand(2));
   //S1/S2 may be null ptr.
   if(vnS1)
      add_cons(copy_cons, vnI, vnS1);
   if(vnS2)
      add_cons(copy_cons, vnI, vnS2);
}

//------------------------------------------------------------------------------
void Andersen_AA::id_vaarg_insn(const llvm::Instruction *I)
{
   assert(I);
   auto VI= llvm::cast<const llvm::VAArgInst>(I);
   u32 vnI= get_val_node(VI);

   llvm::errs() << "  id_vaarg_insn  ";
   print_val(VI);
   llvm::errs() << "\n";

   auto F= I->getParent()->getParent();
   u32 vaF= get_vararg_node(F);
   //Anything accessed by va_arg is either one of the varargs
   //  or a normal arg of type va_list.
   assert(vaF && "va_list args not handled yet");
   add_cons(copy_cons, vnI, vaF);
}

//------------------------------------------------------------------------------
void Andersen_AA::id_extract_insn(const llvm::Instruction *I)
{
   assert(I);
   auto EI= llvm::cast<const llvm::ExtractValueInst>(I);
   //u32 vnI= get_val_node(EI);

   llvm::errs() << "  id_extract_insn  ";
   print_val(EI);
   llvm::errs() << "\n";

   assert(!"ExtractValue not handled yet");
}


//------------------------------------------------------------------------------
// Call handlers
//------------------------------------------------------------------------------

//Make the object nodes for the ptr-return call at node vnI;
//  (F) is the called function (may be null).
void Andersen_AA::id_call_obj(u32 vnI, const llvm::Function *F)
{
   auto I= llvm::dyn_cast_or_null<const llvm::Instruction>(nodes[vnI]->get_val());
   assert(I);
   llvm::ImmutableCallSite CS(I);
   llvm::errs() << "    id_call_obj:  ";

   if(F && extinfo->no_struct_alloc(F))
   {
      llvm::errs() << "ext/no_struct_alloc\n";
      //1 obj for non-struct-alloc externals (heap alloc => weak).
      u32 on= next_node++;
      obj_node[I]= on;
      nodes.push_back(new Node(I, 1, 1));
      add_cons(addr_of_cons, vnI, on);
      //An indirect call may refer to an is_alloc external.
      //Also, realloc does a normal alloc if arg0 is null.
   }
   else if(!F || extinfo->is_alloc(F) || (extinfo->get_type(F) == EFT_REALLOC
                                          && llvm::isa<llvm::ConstantPointerNull>(CS.getArgument(0))))
   {
      llvm::errs() << (F ? "ext/alloc\n" : "indirect\n");
      //An ext. alloc call is equivalent to a malloc.
      const llvm::Type *T= trace_alloc_type(I);
      u32 on= next_node;
      obj_node[I]= on;

      if(auto ST= llvm::dyn_cast<const llvm::StructType>(T))
      {
         const std::vector<u32> &sz= get_struct_sz(ST);
         auto nf= sz.size();
         for(auto i: boost::irange(0ul,nf))
         {
            nodes.push_back(new Node(I, sz[i], 1));
         }
         next_node += nf;
      }
      else
      {
         nodes.push_back(new Node(I, 1, 1));
         ++next_node;
      }
      if(F)
         add_cons(addr_of_cons, vnI, on);
      //For indirect calls, the solver will determine
      //  if the above should be added.
   }
   else if(extinfo->has_static(F))
   {
      bool stat2= extinfo->has_static2(F);
      llvm::errs() << "ext/static";
      if(stat2)
         llvm::errs() << "2";
      llvm::errs() << "\n";
      std::string fn= F->getName();
      u32 on= 0;
      auto i_srn= stat_ret_node.find(fn);
      if(i_srn != stat_ret_node.end())
         on= i_srn->second;
      else
      {
         obj_node[I]= on= next_node;
         stat_ret_node[fn]= on;
         //Unknown ext. static vars X,Y where ret -> X -> Y
         if(stat2)
         {
            //stat2 funcs have only non-struct static data.
            nodes.push_back(new Node(I, 2, true));
            nodes.push_back(new Node(I, 1, true));
            next_node += 2;
            add_cons(addr_of_cons, on, on+1);
         }
         else
         {                            //ret -> X; X may be a struct
            const llvm::Type *T= I->getType()->getContainedType(0);
            if(auto ST= llvm::dyn_cast<const llvm::StructType>(T))
            {
               const std::vector<u32> &sz= get_struct_sz(ST);
               auto nf= sz.size();
               for(auto i: boost::irange(0ul,nf))
               {
                  nodes.push_back(new Node(I, sz[i], true));
               }
               next_node += nf;
            }
            else
            {
               nodes.push_back(new Node(I, 1, true));
               ++next_node;
            }
         }
      }
      add_cons(addr_of_cons, vnI, on);
      //Only alloc/static externals need a call-site object.
   }
   else
      llvm::errs() << "\n";
}

//------------------------------------------------------------------------------
//Add the constraints for a direct, non-external call.
void Andersen_AA::id_dir_call(llvm::ImmutableCallSite CS, const llvm::Function *F)
{
   assert(F);
   llvm::errs() << "    id_dir_call:  ";
   print_val(F);
   llvm::errs() << "  (ret.val ";

   //Only handle the ret.val. if it's used as a ptr.
   if(llvm::isa<llvm::PointerType>(CS.getType()))
   {
      u32 vnI= get_val_node(CS.getInstruction());
      //Does it actually return a ptr?
      if(llvm::isa<llvm::PointerType>(F->getReturnType()))
      {
         u32 rnF= get_ret_node(F);
         assert(rnF);
         add_cons(copy_cons, vnI, rnF);
         llvm::errs() << "normal)\n";
      }
      else
      {
         //If not, this is an int->ptr cast that we can't trace.
         add_cons(addr_of_cons, vnI, i2p);
         llvm::errs() << "i2p)\n";
      }
   }
   else
      llvm::errs() << "ignored)\n";

   //Iterators for the actual and formal parameters
   auto itA= CS.arg_begin(), ieA= CS.arg_end();
   auto itF= F->arg_begin(), ieF= F->arg_end();
   //Go through the fixed parameters.
   llvm::errs() << "      args:";
   for(; itF != ieF; ++itA, ++itF)
   {
      //Some programs (e.g. Linux kernel) leave unneeded parameters empty.
      if(itA == ieA)
      {
         llvm::errs() << " !! not enough args\n";
         break;
      }
      const llvm::Argument& arg= *itF;
      const llvm::Value *AA= *itA, *FA= &arg;          //current actual/formal arg
      //Non-ptr formal args don't need constraints.
      if(!llvm::isa<llvm::PointerType>(FA->getType()))
         continue;
      llvm::errs() << "    ";
      print_val(FA);
      llvm::errs() << " <= ";
      print_val(AA);
      u32 vnFA= get_val_node(FA);
      if(llvm::isa<llvm::PointerType>(AA->getType()))
      {
         u32 vnAA= get_val_node_cptr(AA);
         if(vnAA)
            add_cons(copy_cons, vnFA, vnAA);
      }
      else
         add_cons(addr_of_cons, vnFA, i2p);
   }
   //Any remaining actual args must be varargs.
   if(F->isVarArg())
   {
      u32 vaF= get_vararg_node(F);
      assert(vaF);
      llvm::errs() << "\n      varargs:";
      for(; itA != ieA; ++itA)
      {
         llvm::Value *AA= *itA;
         llvm::errs() << "  ";
         print_val(AA);
         if(llvm::isa<llvm::PointerType>(AA->getType()))
         {
            u32 vnAA= get_val_node_cptr(AA);
            if(vnAA)
               add_cons(copy_cons, vaF, vnAA);
         }
         else
            add_cons(addr_of_cons, vaF, i2p);
      }
   }
   else
      assert(itA == ieA && "too many args to non-vararg func.");
   llvm::errs() << "\n";
}

//------------------------------------------------------------------------------
//Add the constraints for an indirect call.
void Andersen_AA::id_ind_call(llvm::ImmutableCallSite CS)
{
   llvm::errs() << "    id_ind_call:  ";
   auto C= CS.getCalledValue();
   assert(C);
   if(llvm::isa<llvm::InlineAsm>(C))
   {
      llvm::errs() << "ignoring inline ASM\n";
      return;
   }

   auto I= CS.getInstruction();
   //The callee may be an i2p const.expr.
   u32 vnC= get_val_node_cptr(C);
   assert(vnC && "null callee");
   ind_calls.insert(vnC);

   llvm::errs() << "retval ";
   if(llvm::isa<llvm::PointerType>(CS.getType()))
   {
      u32 vnI= get_val_node(CS.getInstruction());
      add_cons(load_cons, vnI, vnC, func_node_off_ret);
      //Map the constraint to the insn. that created it.
      icall_cons[Constraint(load_cons, vnI, vnC, func_node_off_ret)].insert(I);
      llvm::errs() << "normal";
   }
   else
      llvm::errs() << "ignored";

   //Go through the fixed parameters.
   llvm::errs() << "\n      args:";
   //The node offset of the next ptr arg
   u32 arg_off= func_node_off_arg0;
   auto itA= CS.arg_begin(), ieA= CS.arg_end();
   for(; itA != ieA; ++itA, ++arg_off)
   {
      llvm::Value *AA= *itA;
      llvm::errs() << "  ";
      print_val(AA);
      //FIXME: don't add these cons. if the current formal arg in the
      //  function ptr type is of non-ptr type
      if(llvm::isa<llvm::PointerType>(AA->getType()))
      {
         u32 vnAA= get_val_node_cptr(AA);
         if(vnAA)
         {
            add_cons(store_cons, vnC, vnAA, arg_off);
            icall_cons[Constraint(store_cons, vnC, vnAA, arg_off)].insert(I);
         }
      }
      else
      {
         add_cons(store_cons, vnC, p_i2p, arg_off);
         icall_cons[Constraint(store_cons, vnC, p_i2p, arg_off)].insert(I);
      }
   }
   //TODO: handle varargs (whenever the offset on a store cons. is exceeded,
   //  the solver will need to reset it to the vararg node offset if the current
   //  member of the dest. points_to is a vararg func).
   llvm::errs() << "\n";
}

//------------------------------------------------------------------------------
//Add the constraints for the direct call of ext. function F, based on its name.
//If F is unknown, assume it does nothing to pointers.
void Andersen_AA::id_ext_call(const llvm::ImmutableCallSite &CS, const llvm::Function *F)
{
   assert(F && extinfo->is_ext(F));
   auto I = CS.getInstruction();
   llvm::errs() <<"    id_ext_call:  ";
   print_val(F);
   llvm::errs() << "\n";

   //The constraints for static/alloc were added by id_call_obj.
   if(extinfo->is_alloc(F) || extinfo->has_static(F))
      return;

   extf_t tF= extinfo->get_type(F);
   switch(tF)
   {
      case EFT_REALLOC:
         //When realloc is passed a non-null pointer, it copies the data
         //  to a new block, so the return will point to a copy of the object
         //  that arg0 pointed to. We can consider it to be the same object,
         //  so the return can just copy arg0's points-to.
         if(llvm::isa<llvm::ConstantPointerNull>(CS.getArgument(0)))
            break;
      case EFT_L_A0:
      case EFT_L_A1:
      case EFT_L_A2:
      case EFT_L_A8:
      {
         if(!llvm::isa<llvm::PointerType>(I->getType()))
            break;
         u32 vnD= get_val_node(I);
         u32 i_arg;
         switch(tF)
         {
            case EFT_L_A1: i_arg= 1; break;
            case EFT_L_A2: i_arg= 2; break;
            case EFT_L_A8: i_arg= 8; break;
            default: i_arg= 0;
         }
         llvm::errs() << "      L_A" << i_arg << "\n";
         auto src= CS.getArgument(i_arg);
         if(llvm::isa<llvm::PointerType>(src->getType()))
         {
            u32 vnS= get_val_node_cptr(src);
            if(vnS)
               add_cons(copy_cons, vnD, vnS);
         }
         else
            add_cons(addr_of_cons, vnD, i2p);
         break;
      }
      case EFT_L_A0__A0R_A1R:
      {
         llvm::errs() << "      L_A0__A0R_A1R\n";
         add_store2_cons(CS.getArgument(0), CS.getArgument(1));
         //memcpy returns the dest.
         if(llvm::isa<llvm::PointerType>(I->getType()))
         {
            add_cons(copy_cons, get_val_node(I), get_val_node(CS.getArgument(0)));
         }
         break;
      }
      case EFT_A1R_A0R:
         llvm::errs() << "      A1R_A0R\n";
         add_store2_cons(CS.getArgument(1), CS.getArgument(0));
         break;
      case EFT_A3R_A1R_NS:
         llvm::errs() << "      A3R_A1R_NS\n";
         //These func. are never used to copy structs, so the size is 1.
         add_store2_cons(CS.getArgument(3), CS.getArgument(1), 1);
         break;
      case EFT_A1R_A0:
      {
         llvm::errs() << "      A1R_A0\n";
         u32 vnD= get_val_node_cptr(CS.getArgument(1));
         u32 vnS= get_val_node_cptr(CS.getArgument(0));
         if(vnD && vnS)
            add_cons(store_cons, vnD, vnS);
         break;
      }
      case EFT_A2R_A1:
      {
         llvm::errs() << "      A2R_A1\n";
         u32 vnD= get_val_node_cptr(CS.getArgument(2));
         u32 vnS= get_val_node_cptr(CS.getArgument(1));
         if(vnD && vnS)
            add_cons(store_cons, vnD, vnS);
         break;
      }
      case EFT_A4R_A1:
      {
         llvm::errs() << "      A4R_A1\n";
         u32 vnD= get_val_node_cptr(CS.getArgument(4));
         u32 vnS= get_val_node_cptr(CS.getArgument(1));
         if(vnD && vnS)
            add_cons(store_cons, vnD, vnS);
         break;
      }
      case EFT_L_A0__A2R_A0:
      {
         llvm::errs() << "      L_A0__A2R_A0\n";
         if(llvm::isa<llvm::PointerType>(I->getType()))
         {
            //Do the L_A0 part if the retval is used.
            u32 vnD= get_val_node(I);
            auto src= CS.getArgument(0);
            if(llvm::isa<llvm::PointerType>(src->getType()))
            {
               u32 vnS= get_val_node_cptr(src);
               if(vnS)
                  add_cons(copy_cons, vnD, vnS);
            }
            else
               add_cons(addr_of_cons, vnD, i2p);
         }
         //Do the A2R_A0 part.
         u32 vnD= get_val_node_cptr(CS.getArgument(2));
         u32 vnS= get_val_node_cptr(CS.getArgument(0));
         if(vnD && vnS)
            add_cons(store_cons, vnD, vnS);
         break;
      }
      case EFT_A0R_NEW:
      case EFT_A1R_NEW:
      case EFT_A2R_NEW:
      case EFT_A4R_NEW:
      case EFT_A11R_NEW:
      {
         u32 i_arg;
         switch(tF)
         {
            case EFT_A1R_NEW: i_arg= 1; break;
            case EFT_A2R_NEW: i_arg= 2; break;
            case EFT_A4R_NEW: i_arg= 4; break;
            case EFT_A11R_NEW: i_arg= 11; break;
            default: i_arg= 0;
         }
         llvm::errs() << "      A" << i_arg << "R_NEW\n";
         //X -> X/0; *argI = X
         auto dest = CS.getArgument(i_arg);
         u32 vnD = get_val_node_cptr(dest);
         if(!vnD)
            break;
         auto T = dest->getType()->getContainedType(0);
         assert(llvm::isa<llvm::PointerType>(T) && "arg is not a double pointer");
         T= T->getContainedType(0);
         while(auto AT= llvm::dyn_cast<const llvm::ArrayType>(T))
            T= AT->getElementType();

         //make X/0 etc.
         u32 on = next_node;
         obj_node[dest]= on;
         if(auto ST= llvm::dyn_cast<const llvm::StructType>(T))
         {
            const std::vector<u32> &sz= get_struct_sz(ST);
            auto nf= sz.size();
            for(auto i: boost::irange(0ul,nf))
            {
               //FIXME: X/0 shouldn't really have a value because it's not
               //  pointed to by any program variable, but for now we require
               //  all obj_nodes to have one.
               nodes.push_back(new Node(dest, sz[i], 1));
            }
            next_node += nf;
         }
         else
         {
            nodes.push_back(new Node(dest, 1, 1));
            ++next_node;
         }
         u32 vn= next_node++;
         nodes.push_back(new Node);        //X
         add_cons(addr_of_cons, vn, on);   //X -> X/0
         add_cons(store_cons, vnD, vn);    //*argI = X
         break;
      }
      case EFT_ALLOC:
      case EFT_NOSTRUCT_ALLOC:
      case EFT_STAT:
      case EFT_STAT2:
         assert(!"alloc type func. are not handled here");
      case EFT_NOOP:
      case EFT_OTHER:
         break;
      default:
         assert(!"unknown ext.func type");
   }
}

//Add the load/store constraints and temp. nodes for the complex constraint
//  *D = *S (where D/S may point to structs).
void Andersen_AA::add_store2_cons(const llvm::Value *D, const llvm::Value *S, size_t sz)
{
   assert(D && S);
   u32 vnD= get_val_node_cptr(D), vnS= get_val_node_cptr(S);
   if(!vnD || !vnS)
      return;
   //Get the max possible size of the copy, unless it was provided.
   if(!sz)
      sz= std::min(get_max_offset(D), get_max_offset(S));
   //For each field (i), add (Ti = *S + i) and (*D + i = Ti).
   for(auto i: boost::irange(0ul,sz))
   {
      u32 tn= next_node++;
      nodes.push_back(new Node(0));
      add_cons(load_cons, tn, vnS, i);
      add_cons(store_cons, vnD, tn, i);
   }
}

void Andersen_AA::processBlock(const llvm::BasicBlock *BB, const llvm::TargetLibraryInfo *TLI)
{
   if (bb_seen.count(BB)) { return; }
   bb_seen.insert(BB);

   //Handle each insn based on opcode.
   for(auto it= BB->begin(), ie= BB->end(); it != ie; ++it)
   {
      const llvm::Instruction *I= &*it;
      bool is_ptr= llvm::isa<llvm::PointerType>(I->getType());
      switch(I->getOpcode())
      {
         case llvm::Instruction::Ret:
            assert(!is_ptr);
            id_ret_insn(I);
            break;
         case llvm::Instruction::Invoke:
         case llvm::Instruction::Call:
            if(llvm::isMallocLikeFn(I, TLI))
               id_alloc_insn(I);
            else
               id_call_insn(I);
            break;
         case llvm::Instruction::Alloca:
            assert(is_ptr);
            id_alloc_insn(I);
            break;
         case llvm::Instruction::Load:
            if(is_ptr)
               id_load_insn(I);
            break;
         case llvm::Instruction::Store:
            assert(!is_ptr);
            id_store_insn(I);
            break;
         case llvm::Instruction::GetElementPtr:
            assert(is_ptr);
            id_gep_insn(I);
            break;
         case llvm::Instruction::IntToPtr:
            assert(is_ptr);
            id_i2p_insn(I);
            break;
         case llvm::Instruction::BitCast:
            if(is_ptr)
               id_bitcast_insn(I);
            break;
         case llvm::Instruction::PHI:
            if(is_ptr)
               id_phi_insn(I);
            break;
         case llvm::Instruction::Select:
            if(is_ptr)
               id_select_insn(I);
            break;
         case llvm::Instruction::VAArg:
            if(is_ptr)
               id_vaarg_insn(I);
            break;
         case llvm::Instruction::ExtractValue:
            if(is_ptr)
               id_extract_insn(I);
            break;
            //No other ops should affect pointer values.
         default:
            assert(!is_ptr && "unknown insn has a pointer return type");
      }
   }

   // now process each of this BasicBlock's successors
   //
   for (auto i = llvm::succ_begin(BB), e = llvm::succ_end(BB); i != e; ++i)
   {
      processBlock(*i, TLI);
   }
}

//Process all instructions in (F).
void Andersen_AA::visit_func(const llvm::Function *F, const llvm::TargetLibraryInfo *TLI)
{
   assert(F);
   llvm::errs() << "visit_func  ";
   print_val(F);
   llvm::errs() << "\n";

   //First make nodes for all ptr-return insn
   //  (since trace_int may sometimes return values below the current insn).
   //Also number all unnamed instructions that have a result.
   for(auto& BB : F->getBasicBlockList())
      for(auto& Inst : BB.getInstList())
      {
         const llvm::Instruction *I= &Inst;
         if(llvm::isa<llvm::PointerType>(I->getType()))
         {
            nodes.push_back(new Node(I));
            val_node[I]= next_node++;
         }
         if(!I->hasName() && I->getType()->getTypeID() != llvm::Type::VoidTyID)
         {
            llvm::ModuleSlotTracker MST(F->getParent());
            MST.incorporateFunction(*F);
            tmp_num[I]= static_cast<u32>(MST.getLocalSlot(I));
         }
      }

   // process basic blocks in CFG depth-first order (this is not a
   // requirement for this analysis, but it is required by the sparse
   // flow-sensitive pointer analysis using this analysis because (1)
   // it must enforce the same mapping from Value* to value and object
   // nodes as this analysis, and (2) it must process the basic blocks
   // in CFG depth-first order)
   //
   processBlock(&F->getEntryBlock(), TLI);
   bb_seen.clear();
}


void Andersen_AA::obj_cons_id(const llvm::Module &M, const llvm::TargetLibraryInfo *TLI, const llvm::Type * MS)
{
   llvm::errs() << "***** obj_cons_id starting\n";
   //Insert special nodes w/o values.
   next_node= first_var_node;
   for(auto i: boost::irange(0u,first_var_node))
      nodes.push_back(new Node);
   //i2p is actually an object, since its addr. is taken;
   //  p_i2p is its initial pointer
   nodes[i2p]->obj_sz= 1;
   add_cons(addr_of_cons, p_i2p, i2p);

   //Find and analyze all struct types in the program.
   max_struct= min_struct = MS;
   max_struct_sz= 0;

   const auto &tst= M.getIdentifiedStructTypes();
   for(auto ST : tst)
      analyze_struct(ST);

   //IDENTIFY all functions & globals.
   for(auto& f : M.functions())
      id_func(&f);
   for(auto& g: M.globals())
      id_global(&g);

   //Init globals separately
   //  (since an initializer may refer to a global below it).
   for(auto& g: M.globals())
   {
      const llvm::GlobalVariable *G= &g;
      if(G->hasInitializer())
      {
         proc_global_init(get_obj_node(G), G->getInitializer());
      }
      else
      {
         llvm::errs() << "!! uninitialized global:\n";
         print_val(G);
      }
   }
   //Now handle all remaining GEP CEs, since some of them are local.
   for(auto i: boost::irange(0ul,gep_ce.size()))
      proc_gep_ce(gep_ce[i]);

   //Visit all instructions (which may refer to any of the above).
   for(auto& f : M.functions())
   {
      if(!extinfo->is_ext(&f))
         visit_func(&f, TLI);
   }
   //Count the node types (except the special nodes).
   assert(next_node == nodes.size() && "wrong node count");

   print_all_structs();

   //The nodes are now verified and printed at the end of clump_addr_taken().
   //The original constraints should appear in the order they were identified.
   print_all_constraints();

#if CHECK_CONS_UNDEF
   //Look for nodes that are read by constraints but never written.
   std::set<u32> def;
   for(auto i: boost::irange(0ul,constraints.size()))
   {
      if(constraints[i].type != store_cons)
         def.insert(constraints[i].dest);
   }
   //Assume addr-taken function args are always defined (as they will be
   //  written by the store_cons of indirect calls).
   for(auto it= at_args.begin(), ie= at_args.end();
       it != ie; ++it)
   {
      def.insert(get_val_node(*it));
   }
   //The header should not appear if there is no problem.
   bool hdr_done= 0;
   for(auto i: boost::irange(0ul,constraints.size()))
   {
      const Constraint &C= constraints[i];
      assert(C.src);
      if(C.type == addr_of_cons || def.count(C.src))
         continue;
      const llvm::Value /**D= nodes[C.dest]->get_val(),*/ *S= nodes[C.src]->get_val();
      if(S)
      {
         if(llvm::isa<llvm::Argument>(S))
         {
            //A non-addr-taken function may be never used, or one of its args
            //  may be always set to null.
            llvm::errs() << "[arg] ";
            continue;
         }
         else if(S->getName() == "UnifiedRetVal")
         {
            //A function may always return null; with opt -mergereturn,
            //  the UnifiedRetVal is undef, else the <retval> itself is.
            llvm::errs() << "[uret] ";
            continue;
         }
         else if(auto F = llvm::dyn_cast<const llvm::Function>(S))
         {
            if(C.src == get_ret_node(F))
            {
               llvm::errs() << "[ret] ";
               continue;
               //The vararg part may also be unused.
            }
            else if(C.src == get_vararg_node(F))
            {
               llvm::errs() << "[vararg] ";
               continue;
            }
         }
      }
      if(!hdr_done)
      {
         hdr_done= 1;
         llvm::errs() << "!! Constraints with undefined src:\n";
      }
      print_constraint(C);
      llvm::errs() << '\n';
   }
#endif

}

//------------------------------------------------------------------------------
//Move all address-taken nodes to the front, to make the points-to sets denser.
void Andersen_AA::clump_addr_taken()
{
   llvm::errs() << "***** Moving addr-taken nodes to the front...\n";
   std::vector<Node*> old_nodes;
   old_nodes.swap(nodes);
   auto onsz= old_nodes.size();
   nodes.resize(onsz);
   //The node that was originally at (i) will be at move_to[i].
   u32 *move_to= (u32*)malloc(onsz*4);

   //The special nodes must stay at the front.
   for(auto i: boost::irange(0u,first_var_node))
   {
      nodes[i]= old_nodes[i];
      move_to[i]= i;
   }

   u32 nn= first_var_node;               //size of nodes
   //First copy all addr_taken, then all others.
   for(u32 i= first_var_node; i < onsz; ++i){
      if(old_nodes[i]->obj_sz){
         nodes[nn]= old_nodes[i];
         move_to[i]= nn++;
      }
   }
   last_obj_node= nn-1;
   for(u32 i= first_var_node; i < onsz; ++i){
      if(!old_nodes[i]->obj_sz){
         nodes[nn]= old_nodes[i];
         move_to[i]= nn++;
      }
   }

   //Renumber the nodes in all constraints and value-node maps.
   for(auto i: boost::irange(0ul,constraints.size()))
   {
      Constraint &C= constraints[i];
      C.dest= move_to[C.dest];
      C.src= move_to[C.src];
      //Make sure all address-taken nodes are in the object-node segment
      //  (i.e. that no value node has its address taken).
      assert(C.type != addr_of_cons || C.src <= last_obj_node);
   }
   for(auto it= val_node.begin(), ie= val_node.end();
       it != ie; ++it)
      it->second= move_to[it->second];
   for(auto it= obj_node.begin(), ie= obj_node.end();
       it != ie; ++it)
      it->second= move_to[it->second];
   for(auto it= ret_node.begin(), ie= ret_node.end(); it != ie; ++it)
      it->second= move_to[it->second];
   for(auto it= vararg_node.begin(), ie= vararg_node.end(); it != ie; ++it)
      it->second= move_to[it->second];
   //Also update ind_calls and icall_cons.
   std::set<u32> old_ind_calls;
   old_ind_calls.swap(ind_calls);
   for(auto it= old_ind_calls.begin(), ie= old_ind_calls.end(); it != ie; ++it)
      ind_calls.insert(move_to[*it]);
   std::vector<std::pair<Constraint, std::set<const llvm::Instruction*> > > old_icall_cons;
   for(auto it = icall_cons.begin(), ie= icall_cons.end(); it != ie; ++it)
      old_icall_cons.push_back(*it);
   icall_cons.clear();
   for(auto i: boost::irange(0ul,old_icall_cons.size()))
   {
      Constraint &C= old_icall_cons[i].first;
      C.dest= move_to[C.dest];
      C.src= move_to[C.src];
      icall_cons[C]= old_icall_cons[i].second;
   }

   free(move_to);
   //Recheck what we just modified and print the nodes in the new sequence.
   verify_nodes();
   print_all_nodes();
}

//------------------------------------------------------------------------------
//Unify nodes #n1 and #n2, returning the # of the parent (the one of higher
//  rank). The node of lower rank has the rep field set to the parent,
//  and most of its data is deleted.
u32 Andersen_AA::merge_nodes(u32 n1, u32 n2)
{
   assert(n1 && n1 < nodes.size() && "invalid node 1");
   assert(n2 && n2 < nodes.size() && "invalid node 2");
   assert(n1 != n2 && "trying to merge a node with itself");
   assert(n1 != i2p && n2 != i2p && "trying to merge i2p");
   Node *N1= nodes[n1], *N2= nodes[n2];
   u32 rank1= N1->rep, rank2= N2->rep;
   assert(rank1 >= node_rank_min && rank2 >= node_rank_min &&
          "only rep nodes may be merged");
   //Make n1 the parent.
   if(rank1 < rank2)
   {
      std::swap(n1, n2);
      std::swap(N1, N2);
   }
   else if(rank1 == rank2)
      ++N1->rep;
   N2->rep= n1;
   llvm::errs() << "merge_nodes  ";
   print_node(n1);
   llvm::errs() <<"  <=  ";
   print_node(n2);
   llvm::errs() << "\n";

   //If n2 was not visited in a long time,
   //  the combined node should be visited sooner.
   if(N1->vtime > N2->vtime)
      N1->vtime= N2->vtime;
   //Move n2's edges and constraints into n1.
   //This may cause n1 to have some edges to itself; copy edges are removed
   //  here (and also by the solver), while other types should remain.
   N1->points_to |= N2->points_to;
   N1->copy_to |= N2->copy_to;
   if(N1->copy_to.test(n1))
   {
      N1->copy_to.reset(n1);
   }
   if(N1->copy_to.test(n2))
   {
      N1->copy_to.reset(n2);
   }
   N1->load_to |= N2->load_to;
   N1->store_from |= N2->store_from;
   N1->gep_to |= N2->gep_to;
   //The entire points_to must be propagated across the new copy edges.
   N1->prev_points_to= bddfalse;
   //Delete n2's edges and constraints (the lists were cleared by splice()).
   N2->points_to= bddfalse;
   N2->prev_points_to= bddfalse;
   N2->copy_to.clear();
   N2->load_to.clear();
   N2->store_from.clear();
   N2->gep_to.clear();
   //If n2 was used as an indirect call target, n1 will now be used as such.
   if(ind_calls.count(n2))
   {
      ind_calls.erase(n2);
      ind_calls.insert(n1);
   }
   //If n2 is not a non-pointer, neither is n1.
   N1->nonptr &= N2->nonptr;
   //The constraint list is not updated here; instead use get_node_rep
   //  when processing a constraint.
   //icall_cons is not updated either (see solve.cpp and merge_ptr_eq()).
   //obj_sz and addr_taken should not be merged,
   //  since n2 will be pointed to as before;
   //  the GEP-related BDDs should also be ignored here.

   //We don't want zero entries in the map, so use find() rather than [].
   auto i_hv= hcd_var.find(n1);
   u32 hv1= i_hv == hcd_var.end() ? 0 : i_hv->second;
   i_hv= hcd_var.find(n2);
   u32 hv2= i_hv == hcd_var.end() ? 0 : i_hv->second;
   //Was *N2 in an offline cycle?
   if(hv2)
   {
      if(!hv1)
      {
         //*N1 is the same as *N2 (because N1/N2 have to be pointer-equivalent
         //  to get merged), so *N1 would be in a cycle with HV2.
         hcd_var[n1]= hv2;
      }
      else
      {
         u32 rhv1= get_node_rep(hv1), rhv2= get_node_rep(hv2);
         if(rhv1 != rhv2)
         {
            //If we had offline cycles (*N1, RHV1) and (*N2, RHV2), and we know
            //  that *N1 and *N2 are the same, it means RHV1 and RHV2
            //  will be in the same SCC.
            merge_nodes(rhv1, rhv2);
            if(n1 == rhv2)
            {
               //This is the only way for N1 to get merged again.
               n1= get_node_rep(n1);
            }
            else
            {
               assert(nodes[n1]->is_rep());
            }
         }
      }
      //Delete N2 from HCD, as it no longer points to anything.
      hcd_var.erase(n2);
   }
   return n1;
}


//------------------------------------------------------------------------------
//Delete what the optimizations and solver won't need.
void Andersen_AA::pre_opt_cleanup()
{
   struct_info.clear();
   gep_ce.clear();
   global_init_done.clear();
   stat_ret_node.clear();
   at_args.clear();
}

//------------------------------------------------------------------------------
//Hash-based Value Numbering: offline analysis to delete redundant constraints
//  by finding some of the pointer-equivalent variables.
//If (do_union) is 1, use the HU algorithm: give a node a set of labels
//  and union all incoming labels.
void Andersen_AA::hvn(bool do_union)
{
   llvm::errs() << "***** Starting HVN\n";
   make_off_nodes();
   //The LHS of GEP's will be pre-labeled, so start the counter here.
   //  The labels must be disjoint from node IDs because addr_of_cons
   //  use the source ID as a label.
   next_ptr_eq= nodes.size();
   gep2pe.clear();
   add_off_edges();

   //Run the DFS starting from every node, unless it's already been processed.
   curr_dfs= 1;
   lbl2pe.clear();
   for(u32 i= firstAFP; i < firstREF + nREF; ++i)
   {
      if(!off_nodes[i].dfs_id)
      {
         hvn_dfs(i, do_union);
      }
   }
   assert(dfs_stk.empty());

   merge_ptr_eq();
   off_nodes.clear();
   main2off.clear();
   lbl2pe.clear();
   gep2pe.clear();
}


//------------------------------------------------------------------------------
//HVN with Reference processing: by iterating HVN, we can use the fact
//  that (X ptr_eq Y) => (*X ptr_eq *Y).
//The easiest way to do HR is to run the complete HVN as long as it removes
//  at least (min_del) constraints (default 100).
//If (do_union) is 1 (default 0), run HU rather than HVN.
void Andersen_AA::hr(bool do_union, u32 min_del)
{
   //Note: we can optimize this by modifying the constraint graph at the end
   //  of each iteration (merging *X and *Y whenever X ptr_eq Y), then
   //  running the DFS on the same graph.
   u32 curr_n_cons= constraints.size(), prev_n_cons= 0;
   llvm::errs() << "  running HR" << (do_union ? "U" : "") << ", constraint count:  " << curr_n_cons;
   do
   {
      hvn(do_union);
      prev_n_cons= curr_n_cons;
      curr_n_cons= constraints.size();
      llvm::errs() << "  " << curr_n_cons;
   } while(prev_n_cons - curr_n_cons >= min_del);
   llvm::errs() << "\n";
}

//------------------------------------------------------------------------------
//Make the offline nodes corresponding to the rep nodes of the main graph.
void Andersen_AA::make_off_nodes()
{
   llvm::errs() << "***** Creating offline graph nodes\n";
   u32 nn= nodes.size();
   assert(last_obj_node && "clump_addr_taken is required");
   main2off.assign(nn, 0);
   //Start the graph with only the null node (onn - size of off_nodes).
   u32 onn= 1;
   //Add the AFP nodes first (assuming clump_addr_taken has already moved
   //  them in front of all the value nodes).
   firstAFP= 1;
   //Look for function object nodes.
   for(u32 i= first_var_node; i <= last_obj_node; )
   {
      const Node *N= nodes[i];
      u32 sz= N->obj_sz;
      assert(sz && "object nodes are not clumped");
      assert(N->get_val() && "obj node has no value");
      auto F= llvm::dyn_cast<const llvm::Function>(N->get_val());
      //Skip this object if it's not a function.
      if(!F)
      {
         i += sz;
         continue;
      }
      //Go through the retval and all the parameters of F.
      for(u32 j= func_node_off_ret; j < sz; ++j)
      {
         //A rep parameter node gets a new offline node,
         //  but non-rep parm. are not included in any constraints.
         if(nodes[i+j]->is_rep())
         {
            main2off[i+j]= onn++;
         }
      }
      //Set (i) to the node after the current object.
      i += sz;
   }
   firstVAL= onn;
   nAFP= firstVAL - firstAFP;

   //Now add the value nodes, including p_i2p and temporary (no-value) nodes.
   main2off[p_i2p]= onn++;
   for(u32 i= last_obj_node+1; i < nn; ++i)
   {
      const Node *N= nodes[i];
      assert(!N->obj_sz && "unexpected obj node");
      if(N->is_rep())
      {
         main2off[i]= onn++;
      }
   }
   firstREF= onn;
   nVAL= firstREF-firstVAL;
   nREF= nVAL+nAFP;
   //Create all the offline nodes, including REF.
   //AFP & VAR start out direct; then indirect REFs are added.
   off_nodes.assign(onn, OffNode());
   off_nodes.insert(off_nodes.end(), nREF, OffNode(1));
   //Mark AFPs indirect.
   for(u32 i= firstAFP; i < firstVAL; ++i)
   {
      off_nodes[i].indirect= 1;
   }
   llvm::errs() << "  "<< nAFP << " AFP, "<< nVAL << " VAL, "<< nREF << " REF\n";
}

//------------------------------------------------------------------------------
//Add the offline constraint edges based on the current constraint list.
//If hcd is off (default), also add the implicit edges and mark some nodes
//  as indirect.
void Andersen_AA::add_off_edges(bool hcd)
{
   llvm::errs() << "***** Adding offline constraint edges\n";
   u32 n_copy= 0, n_load= 0, n_store= 0, n_impl_addr= 0, n_impl_copy= 0;
   for(auto i: boost::irange(0ul,constraints.size()))
   {
      const Constraint &C= constraints[i];
      //This may fail if the source of an addr_of is a non-rep (which is
      //  allowed but hasn't happened yet).
      assert(nodes[C.dest]->is_rep() && nodes[C.src]->is_rep());
      u32 od= main2off[C.dest], os= main2off[C.src];
      if(!od)
      {
         //A few constraints will have an obj_node for the dest.
         if(nodes[C.dest]->obj_sz)
         {
            continue;
         }
         assert(!"no offline node for dest");
      }
      assert(os || nodes[C.src]->obj_sz && "no offline node for non-obj src");
      switch(C.type)
      {
         case addr_of_cons:
            if(!hcd)
            {
               //D = &S: impl. *D -> S.
               //Also add the actual points-to edge to the label set.
               //  Because of SSA form, there is only one addr_of_cons per dest.
               //  node, so all initial label sets will be singletons or empty,
               //  and HVN mode (do_union = 0) will work correctly.
               off_nodes[od].ptr_eq.set(C.src);
               //Note that S is often a non-AFP obj_node, which we ignore.
               if(os)
               {
                  off_nodes[REF(od)].impl_edges.set(os);
                  ++n_impl_addr;
               }
            }
            break;
         case copy_cons:
            //D = S: edge D -> S, impl. *D -> *S.
            if(os)
            {
               off_nodes[od].edges.set(os);
               ++n_copy;
               if(!hcd)
               {
                  off_nodes[REF(od)].impl_edges.set(REF(os));
                  ++n_impl_copy;
               }
            }
            else
            {
               //Copying from an obj_node not in the graph makes dest. indirect.
               if(!hcd)
               {
                  off_nodes[od].indirect= 1;
               }
            }
            break;
         case load_cons:
            //Note: we don't handle load/store with offset as part of the HVN.
            //  These are only used for indirect calls, so handling them
            //  would not help much.
            if(C.off)
            {
               //D = *S + k: D indirect
               if(!hcd)
               {
                  off_nodes[od].indirect= 1;
               }
            }
            else
            {
               //D = *S: edge D -> *S
               assert(os && "no offline node for src");
               off_nodes[od].edges.set(REF(os));
               ++n_load;
            }
            break;
         case store_cons:
            //*D + k = *S: ignored
            if(!C.off)
            {
               //*D = S: edge *D -> S
               assert(os && "no offline node for src");
               off_nodes[REF(od)].edges.set(os);
               ++n_store;
            }
            break;
         case gep_cons:
            //D = gep S k: D is pre-labeled with the ID of its
            //  (S, k) pair. This works because in SSA form, the LHS of a GEP
            //  cannot be assigned any other value.
            if(!hcd)
            {
               u32 pe;
               std::pair<u32, u32> R(C.src, C.off);
               auto i_g2p = gep2pe.find(R);
               if(i_g2p == gep2pe.end())
               {
                  gep2pe[R]= pe= next_ptr_eq++;
               }
               else
               {
                  pe= i_g2p->second;
               }
               off_nodes[od].ptr_eq.set(pe);
            }
            break;
         default:
            assert(!"unknown constraint type");
      }
   }
   llvm::errs() << "  "<< n_copy << " copy, " << n_load << " load, "<< n_store << " store, " << n_impl_addr << " impl. addr_of, " << n_impl_copy << " impl. copy\n";
}


//------------------------------------------------------------------------------
//Return the rep node of the SCC containing (n), with path compression.
static u32 get_off_rep(u32 n)
{
   u32 &r0= off_nodes[n].rep;
   //If (n) has a rank, it is the rep.
   if(r0 >= node_rank_min)
   {
      return n;
   }
   //Recurse on the parent to get the real rep.
   u32 r= get_off_rep(r0);
   r0= r;                                //set n's parent to the rep
   return r;
}

//------------------------------------------------------------------------------
//Unify offline nodes #n1 and #n2, returning the # of the parent (the one of
//  higher rank). The node of lower rank has the rep field set to the parent.
static u32 merge_off_nodes(u32 n1, u32 n2)
{
   assert(n1 && n1 < off_nodes.size() && "invalid node 1");
   assert(n2 && n2 < off_nodes.size() && "invalid node 2");
   assert(n1 != n2 && "trying to merge a node with itself");
   OffNode *N1= &off_nodes[n1], *N2= &off_nodes[n2];
   assert(N1->dfs_id && N2->dfs_id && "trying to merge unvisited nodes");
   u32 rank1= N1->rep, rank2= N2->rep;
   assert(rank1 >= node_rank_min && rank2 >= node_rank_min &&
          "only rep nodes may be merged");
   //Make n1 the parent.
   if(rank1 < rank2)
   {
      std::swap(n1, n2);
      std::swap(N1, N2);
   }
   else if(rank1 == rank2)
   {
      ++N1->rep;
   }
   N2->rep= n1;
   llvm::errs() << "    merge "<< n1 << " <= "<< n2 << "\n";

   //Move n2's edges and labels into n1. In HVN mode, if both nodes were
   //  pre-labeled, n1 may have >1 label, but hvn_label() will be called
   //  on n1 as soon as the SCC is collapsed, so it will have only 1 label
   //  after hvn_dfs() returns.
   N1->edges |= N2->edges;
   N1->impl_edges |= N2->impl_edges;
   N1->ptr_eq |= N2->ptr_eq;
   N2->edges.clear();
   N2->impl_edges.clear();
   N2->ptr_eq.clear();
   //The entire SCC should be indirect if any node in it is.
   N1->indirect |= N2->indirect;
   //If either node was pre-labeled, the merged node should get the same label.
   N1->ptr_eq |= N2->ptr_eq;
   return n1;
}

//------------------------------------------------------------------------------
//Part of HVN: using Tarjan's algorithm, collapse cycles in the offline graph
//  and assign pointer-equivalence labels to the offline nodes.
//The DFS starts from (n); (do_union) is passed on to hvn_label().
void Andersen_AA::hvn_dfs(u32 n, bool do_union)
{
   assert(n);
   OffNode *N= &off_nodes[n];
   assert(!N->scc_root && N->is_rep());
   u32 our_dfs= curr_dfs++;
   N->dfs_id= our_dfs;

   //Look for SCCs using all edges.
   for(auto it= N->edges.begin(), ie= N->edges.end(); it != ie; ++it)
   {
      hvn_check_edge(n, *it, do_union);
   }
   for(auto it= N->impl_edges.begin(), ie= N->impl_edges.end(); it != ie; ++it)
   {
      hvn_check_edge(n, *it, do_union);
   }
   assert(N->is_rep());

   //Is N the root of an SCC?
   if(N->dfs_id == our_dfs)
   {
      while(!dfs_stk.empty())
      {
         u32 n2= dfs_stk.top();
         //Anything visited before us should remain on the stack.
         if(off_nodes[n2].dfs_id < our_dfs)
         {
            break;
         }
         dfs_stk.pop();
         n= merge_off_nodes(n, n2);
      }
      //Note: the SCC root may be different from the node we started from
      //  if one of the stack nodes had a higher rank.
      assert(n);
      N= &off_nodes[n];
      N->scc_root= 1;
      assert(N->is_rep());
      //Now label the root; a pre-labeled node still needs to get the data
      //  from its neighbors.
      if(do_union)
      {
         hu_label(n);
      }
      else
      {
         hvn_label(n);
      }
   }
   else
   {
      dfs_stk.push(n);
   }
}

//------------------------------------------------------------------------------
//Look at the offline constraint edge from (n) to (dest) and possibly
//  continue the DFS along it; (do_union) is passed on to hvn_label().
void Andersen_AA::hvn_check_edge(u32 n, u32 dest, bool do_union)
{
   OffNode *N= &off_nodes[n];
   assert(N->is_rep());
   //dest comes from a bitmap entry, so it may be out of date.
   u32 n2= get_off_rep(dest);
   const OffNode *N2= &off_nodes[n2];
   //Skip this neighbor if it was merged into N or is a collapsed SCC.
   if(n2 == n || N2->scc_root)
   {
      return;
   }
   //If it's unvisited, continue the DFS.
   if(!N2->dfs_id){
      hvn_dfs(n2, do_union);
   }
   //Set our dfs_id to the minimum reachable ID.
   if(N2->dfs_id < N->dfs_id)
   {
      N->dfs_id= N2->dfs_id;
   }
   assert(N->is_rep());
}

//------------------------------------------------------------------------------
//Label a node based on its neighbors' labels (for HVN).
void Andersen_AA::hvn_label(u32 n)
{
   OffNode *N= &off_nodes[n];
   bitmap &pe= N->ptr_eq;
   assert(N->is_rep() && N->scc_root);
   //All indirect nodes get new labels.
   if(N->indirect)
   {
      //Remove pre-labeling, in case a direct pre-labeled node was
      //  merged with an indirect one.
      pe.clear();
      pe.set(next_ptr_eq++);
      return;
   }
   //Collect all incoming labels into the current node.
   for(auto it= N->edges.begin(), ie= N->edges.end(); it != ie; ++it)
   {
      u32 n2= get_off_rep(*it);
      if(n2 == n){
         continue;
      }
      bitmap &pe2= off_nodes[n2].ptr_eq;
      assert(!pe2.empty() && "unlabeled neighbor");
      //Ignore non-ptr neighbors.
      if(!pe2.test(0)){
         pe |= pe2;
      }
   }
   //If a direct node has no incoming labels, it's a non-pointer.
   if(pe.empty())
   {
      pe.set(0);
      //If there was >1 incoming label, replace the set by its ID.
   }
   else if(pe.count() != 1)
   {
      auto i_l2p= lbl2pe.find(pe);
      if(i_l2p == lbl2pe.end())
      {
         lbl2pe[pe]= next_ptr_eq;
         pe.clear();
         pe.set(next_ptr_eq++);
      }
      else
      {
         pe.clear();
         pe.set(i_l2p->second);
      }
   }
   //If there was only 1, keep it.
   assert(N->ptr_eq.count()==1);
}

//------------------------------------------------------------------------------
//Label a node with the union of incoming labels (for HU).
void Andersen_AA::hu_label(u32 n)
{
   OffNode *N= &off_nodes[n];
   bitmap &pe= N->ptr_eq;
   assert(N->is_rep() && N->scc_root);
   //An indirect node starts with a unique label.
   if(N->indirect)
   {
      pe.set(next_ptr_eq++);
   }
   //Collect all incoming labels into the current node.
   for(auto it= N->edges.begin(), ie= N->edges.end(); it != ie; ++it)
   {
      u32 n2= get_off_rep(*it);
      if(n2 == n)
      {
         continue;
      }
      bitmap &pe2= off_nodes[n2].ptr_eq;
      assert(!pe2.empty() && "unlabeled neighbor");
      //Ignore non-ptr neighbors.
      if(!pe2.test(0))
      {
         pe |= pe2;
      }
   }
   //If a direct node has no incoming labels, it's a non-pointer.
   if(pe.empty())
   {
      pe.set(0);
   }
}

//------------------------------------------------------------------------------
//Merge all pointer-equivalent nodes and update the constraint list.
void Andersen_AA::merge_ptr_eq()
{
   llvm::errs() << "***** Merging pointer-equivalent nodes\n";
   u32 nn= nodes.size();
   //The first node (of the main graph) with the given ptr_eq.
   std::unordered_map<bitmap, u32> pe2node;
   for(auto i: boost::irange(0u,nn))
   {
      u32 on= main2off[i];
      //If this node has no offline version, it's not pointer-equivalent.
      if(!on){
         continue;
      }
      bitmap &pe= off_nodes[get_off_rep(on)].ptr_eq;
      assert(!pe.empty());
      //Non-ptr nodes should be deleted from the constraints.
      if(pe.test(0)){
         assert(pe.count()==1);
         nodes[i]->nonptr= 1;
         continue;
      }
      //Anything previously marked as non-ptr cannot have another label.
      assert(!nodes[i]->nonptr);
      auto i_p2n= pe2node.find(pe);
      if(i_p2n == pe2node.end())
      {
         //This PE was not seen yet, so (i) is its first node.
         assert(nodes[i]->is_rep());
         pe2node[pe]= i;
      }
      else
      {
         u32 en= i_p2n->second;
         //Merge (i) into the node representing (pe).
         i_p2n->second= merge_nodes(en, i);
      }
   }

   std::vector<Constraint> old_cons;
   old_cons.swap(constraints);
   llvm::DenseSet<Constraint> cons_seen;
   for(auto i: boost::irange(0ul,old_cons.size()))
   {
      Constraint &C= old_cons[i];
      //Ignore this constraint if either side is a non-ptr.
      if(nodes[C.dest]->nonptr || nodes[C.src]->nonptr)
         continue;
      C.dest= get_node_rep(C.dest);
      //Don't replace the source of addr_of by the rep: the merging
      //  done in HVN/HCD is based only on pointer equivalence,
      //  so non-reps may still be pointed to.
      if(C.type != addr_of_cons)
         C.src= get_node_rep(C.src);
      //Ignore (copy X X) and duplicates.
      if((C.type != copy_cons || C.dest != C.src) && !cons_seen.count(C)){
         cons_seen.insert(C);
         constraints.push_back(C);
      }
   }

   //Also rewrite icall_cons to refer to the rep nodes.
   std::vector<std::pair<Constraint, std::set<const llvm::Instruction*> > > old_icall_cons;
   for(auto it= icall_cons.begin(), ie= icall_cons.end(); it != ie; ++it)
   {
      old_icall_cons.push_back(*it);
   }
   icall_cons.clear();
   for(auto i: boost::irange(0ul,old_icall_cons.size()))
   {
      Constraint &C= old_icall_cons[i].first;
      if(nodes[C.dest]->nonptr || nodes[C.src]->nonptr)
         continue;
      C.dest= get_node_rep(C.dest);
      C.src= get_node_rep(C.src);
      const std::set<const llvm::Instruction*> &I= old_icall_cons[i].second;
      icall_cons[C].insert(I.begin(), I.end());
   }
}

//------------------------------------------------------------------------------
//Hybrid Cycle Detection, offline part: map any var X to var A if there is
//  an SCC containing both *X and A. Note: after HVN, no SCC will have >1
//  non-REF node.
void Andersen_AA::hcd()
{
   llvm::errs() << "***** Starting HCD\n";
   make_off_nodes();
   //(1) means don't make implicit edges or set the indirect flag.
   add_off_edges(1);
   //Map the offline nodes to the main graph.
   for(auto i: boost::irange(0ul,main2off.size()))
   {
      u32 n= main2off[i];
      if(n)
      {
         off_nodes[n].main_node= i;
         off_nodes[REF(n)].main_node= i;
      }
   }

   //Run the DFS starting from every node, unless it's already been processed.
   curr_dfs= 1;
   for(u32 i= firstAFP; i < firstREF + nREF; ++i)
   {
      if(!off_nodes[i].dfs_id)
      {
         hcd_dfs(i);
      }
   }
   assert(dfs_stk.empty());

   off_nodes.clear();
   main2off.clear();

   //Update constraints and icall_cons to refer to the reps,
   //  because we merged some of the VARs.
   std::vector<Constraint> old_cons;
   old_cons.swap(constraints);
   llvm::DenseSet<Constraint> cons_seen;
   for(auto i: boost::irange(0ul,old_cons.size()))
   {
      const Constraint &C0= old_cons[i];
      u32 dest= get_node_rep(C0.dest), src= C0.src;
      if(C0.type != addr_of_cons)
         src= get_node_rep(src);
      Constraint C(C0.type, dest, src, C0.off);
      //Ignore (copy X X) and duplicates.
      if((C.type != copy_cons || C.dest != C.src) && !cons_seen.count(C))
      {
         cons_seen.insert(C);
         constraints.push_back(C);
      }
   }
   std::vector<std::pair<Constraint, std::set<const llvm::Instruction*> > > old_icall_cons;
   for(auto it= icall_cons.begin(), ie= icall_cons.end(); it != ie; ++it)
   {
      old_icall_cons.push_back(*it);
   }
   icall_cons.clear();
   for(auto i: boost::irange(0ul,old_icall_cons.size()))
   {
      Constraint &C= old_icall_cons[i].first;
      C.dest= get_node_rep(C.dest);
      C.src= get_node_rep(C.src);
      const std::set<const llvm::Instruction*> &I= old_icall_cons[i].second;
      icall_cons[C].insert(I.begin(), I.end());
   }
}

//Part of HCD: using Tarjan's algorithm, find SCCs containing both VAR and REF
//  nodes, then map all REFs to the VAR. We don't collapse SCCs here.
void Andersen_AA::hcd_dfs(u32 n)
{
   assert(n);
   llvm::errs() << "  hcd_dfs "<< n << "\n";
   OffNode *N= &off_nodes[n];
   assert(!N->scc_root && N->is_rep());
   u32 our_dfs= curr_dfs++;
   N->dfs_id= our_dfs;

   //Look for SCCs using normal edges only.
   for(auto it= N->edges.begin(), ie= N->edges.end(); it != ie; ++it)
   {
      const OffNode *N2= &off_nodes[*it];
      assert(N2->is_rep());
      //Skip this neighbor if it's in an already processed SCC.
      if(N2->scc_root)
      {
         continue;
      }
      //If it's unvisited, continue the DFS.
      if(!N2->dfs_id)
      {
         hcd_dfs(*it);
      }
      //Set our dfs_id to the minimum reachable ID.
      if(N2->dfs_id < N->dfs_id)
      {
         N->dfs_id= N2->dfs_id;
      }
   }
   assert(N->is_rep());

   //Is N the root of an SCC?
   if(N->dfs_id == our_dfs)
   {
      //Record all nodes in our SCC (the root is not on the stack).
      std::vector<u32> scc(1, n);
      llvm::errs() << "    HCD SCC: " << n;
      //The VAR (non-REF) nodes in this SCC (may be several since we don't run
      //  HR to convergence).
      std::vector<u32> var;
      if(n < firstREF)
         var.push_back(n);
      while(!dfs_stk.empty())
      {
         u32 n2= dfs_stk.top();
         assert(n2 != n);
         //Anything visited before us should remain on the stack.
         if(off_nodes[n2].dfs_id < our_dfs)
         {
            break;
         }
         dfs_stk.pop();
         scc.push_back(n2);
         llvm::errs() <<  " " << n2;
         if(n2 < firstREF)
         {
            llvm::errs() << '*';
            var.push_back(n2);
         }
      }
      llvm::errs() << "\n";
      //Singleton SCCs are ignored.
      if(scc.size() == 1)
      {
         N->scc_root= 1;
         return;
      }
      assert(var.size() && "no VAR node in SCC");
      //Replace the offline VARs by the corresponding main nodes,
      //  then merge all of those.
      //Note that this will collapse any remaining VAR-only SCCs.
      u32 var_rep= off_nodes[var[0]].main_node;
      for(u32 i= 1, ie= var.size(); i != ie; ++i)
      {
         var_rep= merge_nodes(var_rep, off_nodes[var[i]].main_node);
      }
      //Now process the entire SCC.
      for(auto i: boost::irange(0ul,scc.size()))
      {
         u32 n= scc[i];
         assert(n);
         OffNode *N= &off_nodes[n];
         //Label N as a "root" (since it should be considered deleted but
         //  is not collapsed into the root).
         N->scc_root= 1;
         if(n >= firstREF)
         {
            //Map the main node of N to the vars' rep.
            hcd_var[N->main_node]= var_rep;
         }
      }
   }
   else
   {
      dfs_stk.push(n);
   }
}

//Don't factor any constraint sets smaller than this (must be >1).
static const u32 factor_min_sz= 2;
//Map each factored constraint to the temp node of the result.
static llvm::DenseMap<Constraint, u32> factored_cons;

//------------------------------------------------------------------------------
//Factor load/store constraints.
//If there are (n) constraints of the form (A = *V + k), where (V) and (k)
//  are the same but (A) is any variable, the solver will create (n*p) copy
//  edges, where (p) is V's points-to size.
//We can replace them by a single load_cons (T = *V + k) and (n) copy_cons
//  (A = T), for a total of (n+p) copy edges and 1 extra node.
//Similarly, (*V + k = B) becomes (*V + k = T), plus (T = B) for all B.
void Andersen_AA::factor_ls()
{
   //Map the (src,off) pair of each load_cons to the set of dest nodes,
   //  and the (dest,off) of each store_cons to the set of src.
   //Also delete all load/store cons. from the main list.
   llvm::DenseMap<std::pair<u32, u32>, std::set<u32> > loads, stores;
   std::vector<Constraint> old_cons;
   old_cons.swap(constraints);
   for(auto i: boost::irange(0ul,old_cons.size()))
   {
      const Constraint &C= old_cons[i];
      if(C.type == load_cons)
      {
         loads[std::make_pair(C.src, C.off)].insert(C.dest);
      }
      else if(C.type == store_cons)
      {
         stores[std::make_pair(C.dest, C.off)].insert(C.src);
      }
      else
      {
         constraints.push_back(C);
      }
   }
   old_cons.clear();

   for(auto it= loads.begin(), ie= loads.end(); it != ie; ++it)
   {
      factor_ls(it->second, it->first.first, it->first.second, 1);
   }
   for(auto it= stores.begin(), ie= stores.end(); it != ie; ++it)
   {
      factor_ls(it->second, it->first.first, it->first.second, 0);
   }

   //Update icall_cons to the new constraints.
   std::vector<std::pair<Constraint, std::set<const llvm::Instruction*> > > old_icall_cons;
   for(auto it= icall_cons.begin(), ie= icall_cons.end(); it != ie; ++it)
   {
      old_icall_cons.push_back(*it);
   }
   icall_cons.clear();

   for(auto i: boost::irange(0ul,old_icall_cons.size()))
   {
      Constraint &C= old_icall_cons[i].first;
      auto i_fc= factored_cons.find(C);
      if(i_fc != factored_cons.end())
      {
         if(C.type == load_cons)
            C.dest= i_fc->second;
         else
            C.src= i_fc->second;
      }
      const std::set<const llvm::Instruction*> &I= old_icall_cons[i].second;
      icall_cons[C].insert(I.begin(), I.end());
   }
   factored_cons.clear();
}

//------------------------------------------------------------------------------
//Factor a cons. of the form (other = *ref + off) or (*ref + off = other).
void Andersen_AA::factor_ls(const std::set<u32> &other, u32 ref, u32 off, bool load)
{
   u32 szo= other.size();
   assert(szo);
   //dest (for load) or src (for store) will be filled in below.
   Constraint C(load ? load_cons : store_cons, ref, ref, off);
   if(szo < factor_min_sz)
   {
      //Return unfactored cons. to the list.
      for(auto j= other.begin(), je= other.end(); j != je; ++j)
      {
         if(load)
            C.dest= *j;
         else
            C.src= *j;
         constraints.push_back(C);
      }
   }
   else
   {
      //Add (T = *V + k) or (*V + k = T).
      u32 t= nodes.size();
      nodes.push_back(new Node);
      if(load)
         C.dest= t;
      else
         C.src= t;
      constraints.push_back(C);
      //All the original cons. will be mapped to T.
      Constraint C0= C;
      //Add (A = T) or (T = B).
      C.type= copy_cons;
      C.off= 0;
      if(load)
         C.src= t;
      else
         C.dest= t;
      for(auto j= other.begin(), je= other.end(); j != je; ++j)
      {
         if(load)
            C0.dest= C.dest= *j;
         else
            C0.src= C.src= *j;
         constraints.push_back(C);
         factored_cons[C0]= t;
      }
   }
}

void Andersen_AA::cons_opt()
{
   //Do 1 pass of regular HVN, to reduce HU's memory usage.
   hvn(0);
   //Run HRU until it can no longer remove 100 constraints.
   hr(1, 100);
   //Do HCD after HVN, so that it has fewer nodes to put in the map.
   //This also avoids updating that map after HVN.
   hcd();
   factor_ls();
}

//This vector holds the result of sat2vec().
static std::vector<u32> bddexp;
//A map of BDD IDs to their last-used time and vector expansion.
static std::map<u32, std::pair<u32, std::vector<u32>*> > bv_cache;

//The cache size limit and LRU tracking may be disabled because it already
//  uses relatively little memory.
//The sequence number of the current bdd2vec call (for LRU).
static u32 bv_time= 0;
//priority_queue puts the greatest element at the top, but we need the
//  lowest time at the top, so use the reverse comparator.
static std::priority_queue<std::pair<u32, u32>, std::vector<std::pair<u32, u32> >,
std::greater<std::pair<u32, u32> > > bv_lru;

//------------------------------------------------------------------------------
//This bdd_allsat callback puts all assignments of bit vector (v)
//  (in which -1 means don't care) into the vector (bddexp);
//  the contents of (v) after the end of FDD domain 0 are ignored,
//  and odd positions in (v) are skipped because domains 0 and 1 have
//  their variables interleaved.
//It assumes that domain 0 is the first set of variables to be allocated.
static void sat2vec(char *v, int szv)
{
   //The number of bits in (v) used for domain 0
   //  and any other variables interleaved into it.
   static u32 fdd0_bits= 0;
   if(!fdd0_bits)
      fdd0_bits= (u32)fdd_varnum(0) * 2 - 1;
   //The result with all dont-cares at 0.
   u32 base= 0;
   //The list of dont-care masks and its size.
   static u32 dc[32];
   u32 ndc= 0;
   //v[0] represents bit 0 of the result, so the 1 bit in (m) moves left.
   //Odd values of (i) are skipped because they refer to domain 1.
   for(u32 i= 0, m= 1; i < fdd0_bits; i += 2, m <<= 1)
   {
      switch(v[i])
      {
         case -1:
            dc[ndc++]= m;
            break;
         case 1:
            base |= m;
            break;
         default:
            ;
      }
   }
   //Speed up the handling of small cases (up to 2 dont-cares).
   switch(ndc)
   {
      case 2:
      {
         u32 x= base|dc[1];
         bddexp.push_back(x);
         bddexp.push_back(x|dc[0]);
      }
      case 1:
         bddexp.push_back(base|dc[0]);
      case 0:
         bddexp.push_back(base);
         break;
      default:
         assert(ndc <= 25 && "too many assignments for BDD");
         //Use all subsets of dont-cares.
         for(u32 i= 0, ie= 1<<ndc; i < ie; ++i)
         {
            u32 x= base;
            for(u32 j= 0, m= 1; j < ndc; ++j, m <<= 1)
            {
               if(i&m)
               {
                  x |= dc[j];
               }
            }
            bddexp.push_back(x);
         }
   }
}
//------------------------------------------------------------------------------
//Return a pointer to a vector containing the data of BDD (x).
//Make sure you're finished with this pointer before calling bdd2vec again:
//  it can be deleted when the cache gets full.
static std::vector<u32>* bdd2vec(bdd x)
{
   assert(x != bddtrue);
   if(x == bddfalse)
   {
      static std::vector<u32> v_false;
      return &v_false;
   }
   u32 id= x.id();
   auto i_bvc= bv_cache.find(id);
   if(i_bvc != bv_cache.end())
   {
      //If this BDD is cached, update its time and return its bitmap.
      i_bvc->second.first= bv_time;
      bv_lru.push(std::make_pair(bv_time++, id));
      return i_bvc->second.second;
   }

   //If the cache has reached its capacity, remove the oldest items.
   if(bv_cache.size() >= bvc_sz)
   {
      for(auto i: boost::irange(0u,bvc_remove))
      {
         //Some LRU entries may have an older time than the cache entry.
         while(1)
         {
            std::pair<u32, u32> x= bv_lru.top();
            bv_lru.pop();
            u32 t= x.first, id= x.second;
            i_bvc= bv_cache.find(id);
            assert(i_bvc != bv_cache.end());
            if(t == i_bvc->second.first)
            {
               delete i_bvc->second.second;
               bv_cache.erase(i_bvc);
               break;
            }
         }
      }
   }

   //Fill in bddexp and copy it to the cache.
   //This is faster than passing a new blank vector to sat2vec()
   //  because bddexp doesn't need to grow every time.
   bddexp.clear();
   bdd_allsat(x, sat2vec);
   //The points-to results should be printed in the same order as with bitmaps,
   //  and the solver is a little faster when it processes in increasing order.
   sort(bddexp.begin(), bddexp.end());
   auto v= new std::vector<u32>(bddexp);
   bv_cache[id]= make_pair(bv_time, v);
   bv_lru.push(std::make_pair(bv_time++, id));
   return v;
}

//------------------------------------------------------------------------------
//Delete all of the above data.
static void clear_bdd2vec()
{
  bddexp.clear();
  for(auto it= bv_cache.begin(),
      ie= bv_cache.end(); it != ie; ++it)
    delete it->second.second;
  bv_cache.clear();
  while(!bv_lru.empty())
    bv_lru.pop();
  bv_time= 0;
}



//Initialize the BDDs for points-to sets and GEPs.
void Andersen_AA::pts_init()
{
   u32 npts= last_obj_node+1;
   //The offsets that occur in GEP/load/store constraints.
   std::set<u32> valid_off;
   for(auto i: boost::irange(0ul,constraints.size()))
   {
      const Constraint &C= constraints[i];
      if(C.off)
      {
         valid_off.insert(C.off);
         print_constraint(C);
         llvm::errs() << "\n";
      }
   }
   //fprintf(stderr, "valid offsets: %u of %u\n", valid_off.count(), max_sz);
   //For each offset (i), off_nodes[i] holds the nodes with obj_sz == (i+1),
   //  i.e. those for which (i) is the max allowed offset.
   std::vector<std::set<u32> > off_nodes;
   u32 max_sz= 0;
   for(auto i: boost::irange(0u,npts))
   {
      u32 sz= nodes[i]->obj_sz;
      if(sz < 2)
         continue;
      if(max_sz < sz)
      {
         max_sz= sz;
         off_nodes.resize(sz);
      }
      //Record the node in the largest valid offset.
      u32 off= sz-1;
      for(; off && !valid_off.count(off); --off);
      if(!off)
         continue;
      llvm::errs() << "off_nodes["<< off << "] <- ";
      print_node(i);
      llvm::errs() << "\n";
      off_nodes[off].insert(i);
   }

   //Use at least 8M nodes, with 1 cache entry per 8 nodes.
   //The second parameter doesn't matter because cacheratio overrides it.
   bdd_init(8000000, 1000);
   bdd_setcacheratio(8);
   //Grow the node table by 1M at a time, when <40% is free.
   bdd_setmaxincrease(1000000);
   bdd_setminfreenodes(40);
   //Allow unlimited growth of the node table
   //  (since we already keep track of memory usage).
   bdd_setmaxnodenum(0);
   //Disable default pre/post-garbage-collection hook.
   bdd_gbc_hook(NULL);
   //We require a particular variable ordering.
   bdd_disable_reorder();

   //Make a BDD domain for the points_to members and another for the result
   //  of GEPs. This should be done with a single call to fdd_extdomain,
   //  so that the variables are interleaved, else making the GEP functions
   //  will be very slow. Also, the points-to domain must be the first
   //  set of BDD variables allocated.
   int domains[]= {static_cast<int>(npts), static_cast<int>(npts)};
   fdd_extdomain(domains, 2);
   node_vars.assign(npts, bddfalse);
   pts_dom= fdd_ithset(0);
   if(npts >= 2)
   {
      //This will fail if sat2vec assumes the wrong variable order.
      bdd b= get_node_var(npts-1) | get_node_var(npts/3);
      assert(bdd_satcountset(b, pts_dom) == 2 && "test set has wrong size");
      std::vector<u32> v= *bdd2vec(b);
      assert(v.size() == 2 && "bdd2vec doesn't work");
      assert(v[0] == npts/3 && v[1] == npts-1 && "bdd2vec doesn't work");
   }
   //Set up the domain map.
   gep2pts= bdd_newpair();
   fdd_setpair(gep2pts, 1, 0);
   //Make a bit vector for each domain.
   bvec vpts= bvec_varfdd(0), vgep= bvec_varfdd(1);
   u32 pts_bits= (u32)vpts.bitnum();
   //For each offset, make the GEP function.
   geps.assign(max_sz, bddfalse);
   //The set of nodes with more fields than the current offset.
   bdd om= bddfalse;
   off_mask.assign(max_sz, bddfalse);
   for(auto it0= valid_off.rbegin(), ie= valid_off.rend(); it0 != ie; ++it0)
   {
      u32 off= *it0;
      assert(off < off_nodes.size());
      for(auto it1= off_nodes[off].begin(), ie= off_nodes[off].end(); it1 != ie; ++it1)
      {
         om |= get_node_var(*it1);
      }
      //Vector for (pts+off).
      bvec add= bvec_add(vpts, bvec_con(pts_bits, off));
      //The function starts as the adder mapped into the GEP domain,
      //  so that the GEP vars will hold the set of (x+off) for all x in PTS.
      bdd f= bvec_equ(vgep, add);
      //Restrict the final function's argument to the set of acceptable nodes.
      geps[off]= f & om;
      //Save the current offset mask.
      off_mask[off]= om;
   }
   for(auto i: boost::irange(0u,npts))
   {
      const Node *N= nodes[i];
      if(auto F= llvm::dyn_cast_or_null<const llvm::Function>(N->get_val()))
      {
         if(extinfo->is_ext(F))
         {
            assert(N->obj_sz == 1);
            ext_func_nodes |= get_node_var(i);
            ext_func_node_set.insert(i);
            func_node_set.insert(i);
         }
         else if(N->obj_sz > 1)
         {
            func_node_set.insert(i);
         }
      }
   }
}


//------------------------------------------------------------------------------
void Andersen_AA::solve_init()
{
   assert(pts_dom != bddfalse && "solve_init called before pts_init");
   u32 nn= nodes.size(), npts= last_obj_node+1;
   //Build the constraint graph and cplx_cons.
   //Note that prev_points_to remains empty for all nodes.
   u32 ncplx= 0;
   cplx_cons.clear();
   for(auto i: boost::irange(0ul,constraints.size()))
   {
      const Constraint &C= constraints[i];
      u32 dest= get_node_rep(C.dest), src= get_node_rep(C.src);
      switch(C.type)
      {
         case addr_of_cons:
            assert(src < npts);
            nodes[dest]->points_to |= get_node_var(src);
            break;
         case copy_cons:
            assert(src != dest);
            nodes[src]->copy_to.set(dest);
            break;
         case load_cons:
            nodes[src]->load_to.set(ncplx++);
            cplx_cons.push_back(C);
            break;
         case store_cons:
            nodes[dest]->store_from.set(ncplx++);
            cplx_cons.push_back(C);
            break;
         case gep_cons:
            nodes[src]->gep_to.set(ncplx++);
            cplx_cons.push_back(C);
            break;
         default:
            assert(!"unknown constraint type");
      }
   }
   constraints.clear();
   print_cons_graph(0);           //print the complete constraint graph

   //Start the worklist with all nodes that point to something
   //  and have outgoing constraint edges.
   assert(!WL);
   WL= new Worklist(nn);
   llvm::errs() << "***** Initial worklist:";
   for(auto i: boost::irange(0u,nn))
   {
      Node *N= nodes[i];
      if(!N->is_rep())
         continue;
      N->vtime= 0;
      if(N->points_to == bddfalse)
         continue;
      if(N->copy_to.empty() && N->load_to.empty() && N->store_from.empty() &&
            N->gep_to.empty())
      {
         //If N has no outgoing constraints, we can't do anything with it now.
         continue;
      }
      WL->push(i, 0);
      llvm::errs() << "  ";
      print_node(i);
   }
   llvm::errs() << "\n";
   WL->swap_if_empty();
   ext_seen.clear();
   ext_failed.clear();
}

//------------------------------------------------------------------------------
//Returns 0 on completion, 1 on abort.
bool Andersen_AA::solve()
{
   assert(WL && "solve called without a worklist");
   llvm::errs() << "***** Starting pass 0\n";

   n_node_runs= 0;
   last_lcd= 0;
   vtime= 1;
   bool fail= 0;
   while(1)
   {
      if(WL->swap_if_empty())
      {            //current pass done
         //If nothing is queued for this pass, the graph has converged.
         if(WL->empty())
            break;
         llvm::errs() << "***** Starting pass\n";
      }
      u32 p;
      u32 n= WL->pop(&p);
      //If a node was merged after the push, it will stay on the list
      //  but should not be processed.
      if(!nodes[n]->is_rep())
         continue;

      //the ID worklist sets vtime to 0
      //This entry may have an earlier vtime if it was pushed onto next and then
      //  popped from curr.
      if(p < nodes[n]->vtime)
         continue;
      //If something was merged into n after it was pushed, its vtime may have
      //  been reduced below p.

      solve_node(n);

      //Is it time to do cycle detection?
      if(lcd_starts.size() >= lcd_size || n_node_runs - last_lcd >= lcd_period)
         run_lcd();
   }

   delete WL;
   WL= 0;
   return fail;
}

//------------------------------------------------------------------------------
//Lazy Cycle Detection: detect cycles starting from every node that had a
//  neighbor (across an outgoing copy edge) with an equal points-to set.
void Andersen_AA::run_lcd()
{
   last_lcd= n_node_runs;
   curr_lcd_dfs= 1;
   lcd_dfs_id.clear();
   lcd_roots.clear();
   //Run DFS starting from every rep node on the list, unless it was already seen
   //  in the current LCD pass.
   llvm::errs() << "LCD starting\n";
   for(auto it= lcd_starts.begin(), ie= lcd_starts.end();
       it != ie; ++it)
   {
      u32 n= *it;
      if(nodes[n]->is_rep() && !lcd_dfs_id.count(n))
         lcd_dfs(n);
   }
   llvm::errs() << "LCD done\n";
   assert(lcd_stk.empty());
   lcd_starts.clear();
}

//------------------------------------------------------------------------------
//Handle the complex constraints of node (n),
//  then propagate its points_to along its copy_to edges.
void Andersen_AA::solve_node(u32 n)
{
   ++n_node_runs;
   Node *N= nodes[n];
   llvm::errs() << "solve_node  ";
   print_node(n);
   llvm::errs() << "  vtime: "<< N->vtime << " -> " << vtime << "\n";
   N->vtime= vtime++;

   //Points-to bits added to N since the last visit.
   bdd d_points_to= N->points_to - N->prev_points_to;
   //If this node was on the prev. worklist, it may have been processed
   //  (updating the points_to) after getting added to the curr. list.
   if(d_points_to == bddfalse)
      return;
   const std::vector<u32> *d_points_to_v= bdd2vec(d_points_to);
   const u32 *pdpts= &(d_points_to_v->at(0));
   const u32 *edpts= pdpts + d_points_to_v->size();

   //This takes constant time with BDDs.
   N->prev_points_to= N->points_to;

   //If we collapse our points_to via HCD, this will be its rep.
   u32 hcd_rep= 0;
   //Is there an offline cycle (*N, HV)?
   auto i_hv= hcd_var.find(n);
   if(i_hv != hcd_var.end())
   {
      llvm::errs() << "HCD starting\n";
      //Then merge everything in our points_to with HV.
      hcd_rep= get_node_rep(i_hv->second);
      bool chg= 0;
      for(const u32 *ip= pdpts; ip != edpts; ++ip)
      {
         u32 x= get_node_rep(*ip);
         if(x != hcd_rep && x != i2p)
         {
            hcd_rep= merge_nodes(hcd_rep, x);
            chg= 1;
         }
      }
      llvm::errs() << "HCD done\n";

   }
   //The final rep of the SCC goes on the worklist.
   WL->push(hcd_rep, nodes[hcd_rep]->vtime);
   //N can be non-rep if it either pointed to itself or was == HV.
   assert(nodes[n]->is_rep() || get_node_rep(n) == hcd_rep);

   //Because points_to changed, we need to go over the complex constraints again
   //  and propagate d_points_to along the copy edges.
   //All of these steps were factored out for profiling.
   //Each complex constraint is updated to refer to the rep node. This may
   //  introduce duplicates into the list, which will be deleted.
   std::set<Constraint> cons_seen;

   for(auto it= N->load_to.begin(), ie= N->load_to.end(); it != ie; )
   {
      u32 cid= *it;
      //Move past the current element because we may erase it.
      ++it;
      if(solve_ls_cons(n, hcd_rep, d_points_to, cons_seen, cplx_cons[cid]))
      {
         N->load_to.reset(cid);
      }
   }

   cons_seen.clear();
   for(auto it= N->store_from.begin(), ie= N->store_from.end(); it != ie; )
   {
      u32 cid= *it;
      ++it;
      if(solve_ls_cons(n, hcd_rep, d_points_to, cons_seen, cplx_cons[cid]))
      {
         N->store_from.reset(cid);
      }
   }

   cons_seen.clear();
   for(auto it= N->gep_to.begin(), ie= N->gep_to.end(); it != ie; )
   {
      u32 cid= *it;
      ++it;
      if(solve_gep_cons(n, d_points_to, cons_seen, cplx_cons[cid]))
      {
         N->gep_to.reset(cid);
      }
   }
   solve_prop(n, d_points_to);
}

//------------------------------------------------------------------------------
//Add the copy edges for the load or store constraint (C);
//  all the other args are local vars in solve_node().
//Note that (C) will be updated in place with the node reps.
//Returns true if (C) became redundant.
bool Andersen_AA::solve_ls_cons(u32 n, u32 hcd_rep, bdd d_points_to,
                                    std::set<Constraint> &cons_seen, Constraint &C)
{
   bool load= C.type == load_cons;
   assert(load || C.type == store_cons);
   //This has to be done on every call because solve_ls_off() may invalidate
   //  the cache entry for d_points_to.
   const std::vector<u32> *d_points_to_v= bdd2vec(d_points_to);
   const u32 *pdpts= &(d_points_to_v->at(0));
   const u32 *edpts= pdpts + d_points_to_v->size();
   Constraint C0= C;
   u32 dest0= C.dest, src0= C.src;
   u32 dest= get_node_rep(dest0), src= get_node_rep(src0), off= C.off;
   //Is N a func.ptr used for indirect calls?
   bool ind_call= ind_calls.count(n);
   //If C is used for an actual ind. call, this will point to its insn.
   std::set<const llvm::Instruction*> *I= 0;

   if(load)
   {
      //If n2 was merged into n, C.src may still be n2.
      assert(src == n);
      //Update C with the rep of src/dest.
      C.src= n;
      C.dest= dest;
   }
   else
   {
      assert(dest == n);
      C.dest= n;
      C.src= src;
   }
   if(ind_call)
   {
      //Use the original (pre-merge) cons. since icall_cons
      //  is not updated when merging.
      auto  i_icc= icall_cons.find(C0);
      if(i_icc != icall_cons.end())
      {
         I= &(i_icc->second);
         if(dest != dest0 || src != src0)
         {
            //If dest/src was merged, update icall_cons, so that we handle this
            //  call site again whenever a new ext.func enters our points_to.
            //Copy the current insn. set, delete the old entry (including the
            //  original copy of the set), point I to the new cons. entry
            //  and merge the current set into it.
            std::set<const llvm::Instruction*> II(*I);
            icall_cons.erase(i_icc);
            I= &(icall_cons[C]);
            I->insert(II.begin(), II.end());
         }
      }
   }

   //If (C) was seen before, it's obviously redundant,
   //  but we should still handle ind. calls for (C0).
   if(cons_seen.count(C))
   {
      if(I)
      {
         assert((load && off == 1) || (!load && off >= 2) &&
                "wrong offset for icall retval or arg");
         for(const u32 *ip= pdpts; ip != edpts; ++ip)
         {
            u32 rn= *ip;
            const Node *R= nodes[rn];
            auto F= llvm::dyn_cast_or_null<const llvm::Function>(R->get_val());
            if(F && extinfo->is_ext(F))
            {
               for(auto it= I->begin(), ie= I->end(); it != ie; ++it)
               {
                  handle_ext(F, *it);
               }
            }
         }
      }
      return 1;
   }
   cons_seen.insert(C);
   llvm::errs() << (load ? "  load_cons  " : "  store_cons  ");
   print_node(load ? dest : src);
   llvm::errs() <<  "  +"<<off<<"  ";
   if(I)
      llvm::errs() << (load ? "(ind_call retval)  " : "(ind_call arg)  ");

   //If our points_to was collapsed via HCD, we only need to add the edge
   //  from its rep. Note that loads with offset are still handled using
   //  the full poins_to, since HCD says nothing about the nodes
   //  at some offset from these members.
   if(hcd_rep && !off)
   {
      assert(!I && "ind. call cons. has no offset");
      assert(nodes[hcd_rep]->is_rep());
      if(load)
      {
         if(add_copy_edge(hcd_rep, dest))
         {
            WL->push(dest, nodes[dest]->vtime);
         }
      }
      else
      {
         if(add_copy_edge(src, hcd_rep))
         {
            WL->push(hcd_rep, nodes[hcd_rep]->vtime);
         }
      }
      llvm::errs() << "<HCD>\n";
      //This cons. is now redundant because all future members of our points_to
      //  will be pointer-equivalent to hcd_rep, which already has the edge.
      return 1;
   }

   if(off)
   {
      solve_ls_off(d_points_to, load, dest, src, off, I);
   }
   else
   {
      assert(!I);
      solve_ls_n(pdpts, edpts, load, dest, src);
   }
   llvm::errs() << "\n";
   return 0;
}

//------------------------------------------------------------------------------
//The main loop of solve_ls_cons(), separated for profiling.
//The first version is for load/store with offset, and the second
//  is for normal load/store (offset 0).
void Andersen_AA::solve_ls_off(bdd d_points_to, bool load,
                                   u32 dest, u32 src, u32 off, const std::set<const llvm::Instruction*> *I)
{
   //Remove points-to members too small for the offset.
   //However, if this is an ind. call, we must keep all ext. function nodes
   //  because they all have obj_sz 1.
   bdd mask= I ? off_mask[off] | ext_func_nodes : off_mask[off];
   bdd m_points_to= d_points_to & mask;
   if(m_points_to == bddfalse)
      return;
   const std::vector<u32> *d_points_to_v= bdd2vec(m_points_to);
   const u32 *pdpts= &(d_points_to_v->at(0));
   const u32 *edpts= pdpts + d_points_to_v->size();
   //Did D.points_to change? (for load only).
   bool chg= 0;
   for(const u32 *ip= pdpts; ip != edpts; ++ip)
   {
      //Use the original points-to member (rather than the rep)
      //  to check for ext.func. and to compare offset/obj_sz.
      u32 rn= *ip;
      llvm::errs() << '{';
      print_node(rn);
      llvm::errs() << '}';
      //In case of ind. call, check if rsrc is an ext. function.
      if(I)
      {
         assert((load && off == 1) || (!load && off >= 2) &&
                "wrong offset for icall retval or arg");
         //When handling an ind.call cons, skip non-func. members.
         if(!func_node_set.count(rn))
            continue;
         const Node *R= nodes[rn];
         if(ext_func_node_set.count(rn))
         {
            auto F= llvm::dyn_cast_or_null<const llvm::Function>(R->get_val());
            for(auto it= I->begin(), ie= I->end(); it != ie; ++it)
            {
               handle_ext(F, *it);
            }
            continue;
         }
         //For non-ext func, check if the offset is in range.
         //  The return and vararg nodes of non-ext func. have Function*
         //  values but obj_sz == 1, so only the node of the function itself
         //  will be processed.
         //      assert(R->obj_sz > off);
         llvm::errs() << "(non-ext)";
      }
      else if(func_node_set.count(rn))
      {
         //Skip load/store with a function if this cons.
         //  is not an actual ind. call.
         continue;
         //This is now an assert because we use off_mask (but it's disabled
         //  because checking the obj_sz is slow).
         //    }
         //    else
         //    {
         //      assert(nodes[rn]->obj_sz > off);
      }
      rn= get_node_rep(rn+off);
      if(load)
      {
         if(add_copy_edge(rn, dest))
         {
            chg= 1;
         }
      }
      else
      {
         //Don't store into non-ptr args.
         if(I)
         {
            const llvm::Value *V= nodes[rn]->get_val();
            //Note: V may be null due to constraint factoring.
            if(V && !llvm::isa<llvm::PointerType>(V->getType()))
               continue;
         }
         if(add_copy_edge(src, rn))
         {
            WL->push(rn, nodes[rn]->vtime);
         }
      }
   }
   if(chg)
   {
      WL->push(dest, nodes[dest]->vtime);
   }
}

//------------------------------------------------------------------------------
void Andersen_AA::solve_ls_n(const u32 *pdpts, const u32 *edpts, bool load,
                                 u32 dest, u32 src)
{
   bool chg= 0;
   for(const u32 *ip= pdpts; ip != edpts; ++ip)
   {
      u32 rn= get_node_rep(*ip);
      llvm::errs() << '{';
      print_node(rn);
      llvm::errs() << '}';
      if(load)
      {
         if(add_copy_edge(rn, dest))
         {
            chg= 1;
         }
      }
      else
      {
         if(add_copy_edge(src, rn))
         {
            WL->push(rn, nodes[rn]->vtime);
         }
      }
   }
   if(chg)
   {
      WL->push(dest, nodes[dest]->vtime);
   }
}

//------------------------------------------------------------------------------
//Handle the GEP constraint (C).
bool Andersen_AA::solve_gep_cons(u32 n, bdd d_points_to,
                                     std::set<Constraint> &cons_seen, Constraint &C)
{
   assert(C.type == gep_cons);
   //If n2 was merged into n, C.src may still be n2.
   assert(get_node_rep(C.src) == n);
   C.src= n;
   u32 dest= get_node_rep(C.dest), off= C.off;
   C.dest= dest;
   if(cons_seen.count(C))
   {
      return 1;
   }
   cons_seen.insert(C);
   llvm::errs() << "  gep_cons  ";
   print_node(dest);
   llvm::errs() << "  ";
   Node *D= nodes[dest];
   bdd prev_pts= D->points_to;
   assert(off < geps.size() && geps[off] != bddfalse);
   //Apply the GEP function with the given offset (removing variables
   //  in the domain from the result) and map it back to the domain.
   D->points_to |= bdd_replace(bdd_relprod(d_points_to,
                                           geps[off], pts_dom), gep2pts);
   if(D->points_to != prev_pts)
   {
      llvm::errs() << '*';
      WL->push(dest, D->vtime);
   }
   llvm::errs() << "\n";
   return 0;
}

//------------------------------------------------------------------------------
//Add the copy edge from (src) to (dest) and copy the entire points_to.
//Returns whether the points_to of (dest) changed.
//This doesn't push (dest) on the worklist, so that we can add several
//  edges toward (dest) and do 1 push at the end.
bool Andersen_AA::add_copy_edge(u32 src, u32 dest)
{
   //We don't want i2p to point to anything, so don't add edges to it.
   //Neither do we add edges from i2p, as there's nothing to propagate.
   //Also avoid edges from a node to itself.
   if(src == i2p || dest == i2p || src == dest)
      return 0;
   Node *S= nodes[src];
   if(S->copy_to.test_and_set(dest))
   {
      llvm::errs() << 'c';
      Node *D= nodes[dest];
      bdd prev_pts= D->points_to;
      D->points_to |= S->points_to;
      if(D->points_to != prev_pts)
      {
         llvm::errs() << '*';
         return 1;
      }
   }
   return 0;
}

//------------------------------------------------------------------------------
//Propagate (d_points_to) along all copy edges from node (n).
void Andersen_AA::solve_prop(u32 n, bdd d_points_to)
{
   Node *N= nodes[n];
   std::set<u32> copy_seen;
   for(auto it= N->copy_to.begin(), ie= N->copy_to.end(); it != ie; ++it)
   {
      u32 dest0= *it;
      assert(dest0 != n && "copy self-loop not removed");
      u32 dest= get_node_rep(dest0);
      llvm::errs() << "  copy edge  ";
      print_node(dest);
      //If the rep of this copy dest. is n itself, or if we already propagated
      //  to it, it can be skipped.
      if(dest == n || copy_seen.count(dest))
         continue;
      copy_seen.insert(dest);
      Node *D= nodes[dest];
      std::pair<u32, u32> E(n, dest);
      //If this is the first time N and D had equal (nonempty) points_to,
      //  do a cycle check starting from N.
      if(!lcd_edges.count(E) && N->points_to != bddfalse)
      {
         if(N->points_to == D->points_to)
         {
            lcd_edges.insert(E);
            lcd_starts.insert(n);
         }
      }
      bdd prev_pts= D->points_to;
      D->points_to |= d_points_to;
      if(D->points_to != prev_pts)
      {
         llvm::errs() << "  *";
         WL->push(dest, D->vtime);
      }
      llvm::errs() << "\n";
   }
}

//------------------------------------------------------------------------------
//Add the edges for the indirect call (I) of ext.func (F).
void Andersen_AA::handle_ext(const llvm::Function *F, const llvm::Instruction *I)
{
   assert(extinfo->is_ext(F));
   std::pair<const llvm::Function*, const llvm::Instruction*> arg(F, I);
   if(ext_seen.count(arg))
      return;
   ext_seen.insert(arg);
   llvm::ImmutableCallSite CS(I);
   extf_t tF= extinfo->get_type(F);
   switch(tF)
   {
      case EFT_ALLOC:
      case EFT_NOSTRUCT_ALLOC:
      {
         //For is_alloc, point the caller's copy of the retval to its object
         //  (making sure the retval is actually used).
         if(!llvm::isa<llvm::PointerType>(I->getType()))
            break;
         u32 vnD= get_node_rep(get_val_node(I));
         u32 onD= get_obj_node(I);
         Node *D= nodes[vnD];
         llvm::errs() << "(alloc: "<< F->getName() << ": ";
         print_node(vnD);
         llvm::errs() <<" -> ";
         print_node(onD);
         llvm::errs() <<')';
         bdd prev_pts= D->points_to;
         D->points_to |= get_node_var(onD);
         if(D->points_to != prev_pts)
         {
            llvm::errs() << '*';
            WL->push(vnD, D->vtime);
         }
         break;
      }
      case EFT_REALLOC:
         //The function pointer may point to realloc at one time
         //  and to a function with fewer args at another time;
         //  we should skip the realloc if the current call has fewer args.
         if(CS.arg_size() < 1)
            break;
         if(llvm::isa<llvm::ConstantPointerNull>(CS.getArgument(0)))
         {
            if(!llvm::isa<llvm::PointerType>(I->getType()))
               break;
            llvm::errs() << "realloc:(alloc)";
            u32 vnD= get_node_rep(get_val_node(I));
            u32 onD= get_obj_node(I);
            Node *D= nodes[vnD];
            bdd prev_pts= D->points_to;
            D->points_to |= get_node_var(onD);
            if(D->points_to != prev_pts)
            {
               llvm::errs() <<'*';
               WL->push(vnD, D->vtime);
            }
            break;
         }
         llvm::errs() << "realloc:";
      case EFT_L_A0:
      case EFT_L_A1:
      case EFT_L_A2:
      case EFT_L_A8:
      {
         if(!llvm::isa<llvm::PointerType>(I->getType()))
            break;
         u32 vnD= get_node_rep(get_val_node(I));
         u32 i_arg;
         switch(tF)
         {
            case EFT_L_A1: i_arg= 1; break;
            case EFT_L_A2: i_arg= 2; break;
            case EFT_L_A8: i_arg= 8; break;
            default: i_arg= 0;
         }
         if(CS.arg_size() <= i_arg)
            break;
         llvm::errs() << "(L_A"<< i_arg << ")";
         const llvm::Value *src= CS.getArgument(i_arg);
         if(llvm::isa<llvm::PointerType>(src->getType()))
         {
            u32 vnS= get_node_rep(get_val_node(src, 1));
            if(vnS)
            {
               if(add_copy_edge(vnS, vnD))
               {
                  WL->push(vnD, nodes[vnD]->vtime);
               }
            }
         }
         else
         {
            Node *D= nodes[vnD];
            bdd prev_pts= D->points_to;
            D->points_to |= get_node_var(i2p);
            if(D->points_to != prev_pts)
            {
               llvm::errs() << '*';
               WL->push(vnD, D->vtime);
            }
         }
         break;
      }
      case EFT_NOOP:
      case EFT_OTHER:
         //No-op and unknown func. have no effect.
         llvm::errs() << "(no-op)";
         break;
      default:
         //FIXME: support other types
         ext_failed.insert(F->getName());
   }
}

//------------------------------------------------------------------------------
//Detect cycles starting from node #n,
//  using Nuutila's version of Tarjan's algorithm.
//When a strongly-connected component is found, all nodes in it are unified.
void Andersen_AA::lcd_dfs(u32 n)
{
   assert(n != i2p && !lcd_roots.count(n));
   Node *N= nodes[n];
   assert(N->is_rep());
   u32 our_dfs= curr_lcd_dfs++;
   lcd_dfs_id[n]= our_dfs;
   //If any of n's edges point to non-rep nodes, they will be updated.
   bitmap del_copy, add_copy;

   for(auto it= N->copy_to.begin(), ie= N->copy_to.end(); it != ie; ++it)
   {
      u32 dest0= *it;
      assert(dest0 != n && "copy self-loop not removed");
      u32 dest= get_node_rep(dest0);
      //Delete and skip any edge whose replacement is already there
      //  or would copy (n) to itself.
      if(add_copy.test(dest) || dest == n)
      {
         del_copy.set(dest0);
         continue;
      }

      //Note that dest may be an already collapsed SCC.
      if(!lcd_roots.count(dest))
      {
         //Recurse on dest if it hasn't been visited by this LCD pass;
         //  this may merge dest.
         if(!lcd_dfs_id.count(dest))
         {
            lcd_dfs(dest);
            dest= get_node_rep(dest);
         }
         //If dest (or any successor) was visited in this pass before us, it must
         //  be the root, so set our dfs_id to the root's id.
         if(lcd_dfs_id[dest] < lcd_dfs_id[n])
            lcd_dfs_id[n]= lcd_dfs_id[dest];
      }

      //If the dest. was merged (such as by the recursion above),
      //  make the edge point to the rep.
      if(dest != dest0)
      {
         del_copy.set(dest0);
         //Don't add the replacement if it's already there.
         if(add_copy.test(dest))
         {
            continue;
         }
         assert(dest != n && "copy self-loop not removed");
         add_copy.set(dest);
      }
   }
   assert(N->is_rep());
   N->copy_to.intersectWithComplement(del_copy);
   N->copy_to |= add_copy;

   //If our dfs_id is unchanged, N is the root of an SCC.
   if(lcd_dfs_id[n] == our_dfs)
   {
      bool chg= 0;                        //was (n) merged?
      while(!lcd_stk.empty())
      {
         u32 n2= lcd_stk.top();
         //Anything visited before us should remain on the stack.
         if(lcd_dfs_id[n2] < our_dfs)
            break;
         lcd_stk.pop();
         //Note: n2 may have been merged using HCD.
         u32 rn2= get_node_rep(n2);
         if(rn2 != n)
         {
            n= merge_nodes(n, rn2);
         }
         chg= 1;
      }
      //Once this SCC is collapsed, the root should not be processed again.
      lcd_roots.insert(n);
      if(chg)
      {
         //N also counts as part of its SCC.
         WL->push(n, nodes[n]->vtime);
      }
   }
   else
      //Save N until we get back to the root.
      lcd_stk.push(n);
}


void Andersen_AA::computePointToSet(llvm::Module &M, const llvm::TargetLibraryInfo *TLI)
{
   run_init();
   list_ext_unknown(M);
   auto MS = getmin_struct(M);
   obj_cons_id(M, TLI, MS);
   clump_addr_taken();
   pre_opt_cleanup();
   cons_opt();
   pts_init();
   solve_init();
   solve();
   print_cons_graph(1);
   print_metrics();
   run_cleanup();
   pts_cleanup();

}

//Return the points-to set of node n, with offset off,
//  as a pointer to a vector in the cache.
const std::vector<u32>* Andersen_AA::pointsToSet(u32 n, u32 off)
{
  assert(n && n < nodes.size() && "node ID out of range");
  if(!off)
    return bdd2vec(nodes[get_node_rep(n)]->points_to);
  assert(off < geps.size() && geps[off] != bddfalse);
  bdd gep= bdd_replace(bdd_relprod(nodes[get_node_rep(n)]->points_to,
      geps[off], pts_dom), gep2pts);
  return bdd2vec(gep);
}

//Return the points-to set of V's node.
const std::vector<u32>* Andersen_AA::pointsToSet(const llvm::Value *V, u32 off)
{
  return pointsToSet(get_val_node(V), off);
}

//Get the rep node of V, or MAX_U32 if it has no node.
u32 Andersen_AA::PE(const llvm::Value* V)
{
  u32 n= get_val_node(V, 1);
  if(!n)
    return NOVAR_ID;
  return get_node_rep(n);
}

//Get the rep node of node n
u32 Andersen_AA::PE(u32 n)
{
  assert(n && n < nodes.size() && "node ID out of range");
  return get_node_rep(n);
}

bool Andersen_AA::is_null(u32 n, u32 off)
{
  assert(n && n < nodes.size() && "node ID out of range");
  bdd pts = nodes[get_node_rep(n)]->points_to;

  if (!off) { return (pts == bddfalse); }
  else
  {
    assert(off < geps.size() && geps[off] != bddfalse);
    bdd gep= bdd_replace(bdd_relprod(pts, geps[off], pts_dom), gep2pts);
    return (gep == bddfalse);
  }
}

bool Andersen_AA::is_single(u32 n, u32 off)
{
  assert(n && n < nodes.size() && "node ID out of range");
  bdd pts = nodes[get_node_rep(n)]->points_to;

  // !! is it faster to use bdd_satcountset or translate the bdd to a
  //    vector and count the size of the vector?

  if (!off) { return (bdd_satcountset(pts,pts_dom) == 1); }
  else
  {
    assert(off < geps.size() && geps[off] != bddfalse);
    bdd gep= bdd_replace(bdd_relprod(pts, geps[off], pts_dom), gep2pts);
    return (bdd_satcountset(gep,pts_dom) == 1);
  }
}


const llvm::Value * Andersen_AA::getValue(u32 n)
{
   assert(n && n < nodes.size() && "node ID out of range");
   return nodes[n]->get_val();
}

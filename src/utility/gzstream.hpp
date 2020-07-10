// ============================================================================
// gzstream, C++ iostream classes wrapping the zlib compression library.
// Copyright (C) 2001  Deepak Bandyopadhyay, Lutz Kettner
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
// ============================================================================
//
// File          : gzstream.h
// Revision      : $Revision: 1.5 $
// Revision_date : $Date: 2002/04/26 23:30:15 $
// Author(s)     : Deepak Bandyopadhyay, Lutz Kettner
//
// Standard streambuf implementation following Nicolai Josuttis, "The
// Standard C++ Library".
// ============================================================================
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
 *              Copyright (C) 2004-2020 Politecnico di Milano
 *
 *   This file is part of the PandA framework.
 *
 *   The PandA framework is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
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
 * The big change with respect to the original code is a refactoring for a header based library.
 */
#ifndef GZSTREAM_HPP
#define GZSTREAM_HPP

// standard C++ with new header file names and std:: namespace
#include <cstring>
#include <fstream>
#include <iosfwd>
#include <zlib.h>

/**
 *----------------------------------------------------------------------------
 *Internal classes to implement gzstream. See below for user classes.
 *----------------------------------------------------------------------------
 */
class gzstreambuf : public std::streambuf
{
 private:
   static const int bufferSize = 47 + 256; // size of data buff
   // totals 512 bytes under g++ for igzstream at the end.

   gzFile file;                  // file handle for compressed file
   char buffer[bufferSize];      // data buffer
   char opened;                  // open/close state of stream
   std::ios_base::openmode mode; // I/O mode

   int flush_buffer();

 public:
   inline gzstreambuf() : opened(0), mode(std::ios::out)
   {
      setp(buffer, buffer + (bufferSize - 1));
      setg(buffer + 4,  // beginning of putback area
           buffer + 4,  // read position
           buffer + 4); // end position
      // ASSERT: both input & output capabilities will not be used together
   }
   inline int is_open()
   {
      return opened;
   }
   inline gzstreambuf* open(const char* name, std::ios_base::openmode open_mode);
   inline gzstreambuf* close();
   inline ~gzstreambuf() override
   {
      close();
   }

   int overflow(int c = EOF) override;
   int underflow() override;
   int sync() override;
};

class gzstreambase : virtual public std::ios
{
 protected:
   gzstreambuf buf;

 public:
   inline gzstreambase()
   {
      init(&buf);
   }
   inline gzstreambase(const char* name, std::ios_base::openmode open_mode);
   inline ~gzstreambase() override;
   inline void open(const char* name, std::ios_base::openmode open_mode);
   inline void close();
   inline gzstreambuf* rdbuf()
   {
      return &buf;
   }
};

/** ----------------------------------------------------------------------------
 * User classes. Use igzstream and ogzstream analogously to ifstream and
 * ofstream respectively. They read and write files based on the gz*
 * function interface of the zlib. Files are compatible with gzip compression.
 * ----------------------------------------------------------------------------
 */

class igzstream : public std::istream, public gzstreambase
{
 public:
   inline igzstream() : std::istream(&buf)
   {
   }
   inline igzstream(const char* name, std::ios_base::openmode _open_mode = std::ios::in) : std::istream(&buf), gzstreambase(name, _open_mode)
   {
   }
   inline gzstreambuf* rdbuf()
   {
      return gzstreambase::rdbuf();
   }
   inline void open(const char* name, std::ios_base::openmode _open_mode = std::ios::in)
   {
      gzstreambase::open(name, _open_mode);
   }
};

class ogzstream : public std::ostream, public gzstreambase
{
 public:
   inline ogzstream() : std::ostream(&buf)
   {
   }
   inline ogzstream(const char* name, std::ios_base::openmode mode = std::ios::out) : std::ostream(&buf), gzstreambase(name, mode)
   {
   }
   inline gzstreambuf* rdbuf()
   {
      return gzstreambase::rdbuf();
   }
   inline void open(const char* name, std::ios_base::openmode _open_mode = std::ios::out)
   {
      gzstreambase::open(name, _open_mode);
   }
};

inline gzstreambuf* gzstreambuf::open(const char* name, std::ios_base::openmode open_mode)
{
   if(is_open())
      return nullptr;
   mode = open_mode;
   // no append nor read/write mode
   if((mode & std::ios::ate) || (mode & std::ios::app) || ((mode & std::ios::in) && (mode & std::ios::out)))
      return nullptr;
   char fmode[10];
   char* fmodeptr = fmode;
   if(mode & std::ios::in)
      *fmodeptr++ = 'r';
   else if(mode & std::ios::out)
      *fmodeptr++ = 'w';
   *fmodeptr++ = 'b';
   *fmodeptr = '\0';
   file = gzopen(name, fmode);
   if(file == nullptr)
      return nullptr;
   opened = 1;
   return this;
}

inline gzstreambuf* gzstreambuf::close()
{
   if(is_open())
   {
      sync();
      opened = 0;
      if(gzclose(file) == Z_OK)
         return this;
   }
   return nullptr;
}

inline int gzstreambuf::underflow()
{ // used for input buffer only
   if(gptr() && (gptr() < egptr()))
      return *reinterpret_cast<unsigned char*>(gptr());

   if(!(mode & std::ios::in) || !opened)
      return EOF;
   // Josuttis' implementation of inbuf
   std::streamsize n_putback = gptr() - eback();
   if(n_putback > 4)
      n_putback = 4;
   memcpy(buffer + (4 - static_cast<size_t>(n_putback)), gptr() - static_cast<size_t>(n_putback), static_cast<size_t>(n_putback));

   int num = gzread(file, buffer + 4, bufferSize - 4);
   if(num <= 0) // ERROR or EOF
      return EOF;

   // reset buffer pointers
   setg(buffer + (4 - n_putback), // beginning of putback area
        buffer + 4,               // read position
        buffer + 4 + num);        // end of buffer

   // return next character
   return *reinterpret_cast<unsigned char*>(gptr());
}

inline int gzstreambuf::flush_buffer()
{
   // Separate the writing of the buffer from overflow() and
   // sync() operation.
   auto w = static_cast<int>(pptr() - pbase());
   if(gzwrite(file, pbase(), static_cast<unsigned int>(w)) != w)
      return EOF;
   pbump(-w);
   return w;
}

inline int gzstreambuf::overflow(int c)
{ // used for output buffer only
   if(!(mode & std::ios::out) || !opened)
      return EOF;
   if(c != EOF)
   {
      *pptr() = static_cast<char>(c);
      pbump(1);
   }
   if(flush_buffer() == EOF)
      return EOF;
   return c;
}

inline int gzstreambuf::sync()
{
   // Changed to use flush_buffer() instead of overflow( EOF)
   // which caused improper behavior with std::endl and flush(),
   // bug reported by Vincent Ricard.
   if(pptr() && pptr() > pbase())
   {
      if(flush_buffer() == EOF)
         return -1;
   }
   return 0;
}

/**
 *--------------------------------------
 * class gzstreambase:
 *--------------------------------------
 */
inline gzstreambase::gzstreambase(const char* name, std::ios_base::openmode mode)
{
   init(&buf);
   open(name, mode);
}

inline gzstreambase::~gzstreambase()
{
   buf.close();
}

inline void gzstreambase::open(const char* name, std::ios_base::openmode _open_mode)
{
   if(!buf.open(name, _open_mode))
      clear(rdstate() | std::ios::badbit);
}

inline void gzstreambase::close()
{
   if(buf.is_open())
      if(!buf.close())
         clear(rdstate() | std::ios::badbit);
}

#endif // GZSTREAM_H

// ============================================================================
// EOF //

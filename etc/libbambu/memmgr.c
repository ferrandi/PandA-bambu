//----------------------------------------------------------------
// Statically-allocated memory manager
//
// by Eli Bendersky (eliben@gmail.com)
// Adapted by Fabrizio Ferrandi to the PandA Framework (ferrandi.fabrizio@polimi.it)
//
// This code is in the public domain.
//----------------------------------------------------------------
#include <stdio.h>

/// Bambu specific macro
#include "bambu_macros.h"

//
// Memory manager: dynamically allocates memory from
// a fixed pool that is allocated statically at link-time.
// the builtin free and malloc have been replaced with functions
// defined in this file. No, include or initialization is needed.
//
// Preprocessor flags you can define to customize the
// memory manager:
//
// DEBUG_SAMM_FATAL
//    Allow printing out a message when allocations fail
//
// DEBUG_SAMM_SUPPORT_STATS
//    Allow printing out of stats in function
//    SAMM_print_stats When this is disabled,
//    SAMM_print_stats does not exist.
//
// Note that in production code on an embedded system
// you'll probably want to keep those undefined, because
// they cause printf to be called.
//
// POOL_SIZE
//    Size of the pool for new allocations. This is
//    effectively the heap size of the application, and can
//    be changed in accordance with the available memory
//    resources.
//
// MIN_POOL_ALLOC_QUANTAS
//    Internally, the memory manager allocates memory in
//    quantas roughly the size of two size_t objects. To
//    minimize pool fragmentation in case of multiple allocations
//    and deallocations, it is advisable to not allocate
//    blocks that are too small.
//    This flag sets the minimal ammount of quantas for
//    an allocation. If the size of a size_t is 4 and you
//    set this flag to 16, the minimal size of an allocation
//    will be 4 * 2 * 16 = 128 bytes
//    If you have a lot of small allocations, keep this value
//    low to conserve memory. If you have mostly large
//    allocations, it is best to make it higher, to avoid
//    fragmentation.
//
// Notes:
// 1. This memory manager is *not thread safe*. Use it only
//    for single thread/task applications.
//

#define POOL_SIZE 384 * 1024
#define MIN_POOL_ALLOC_QUANTAS 16

union SAMM_header_union
{
   struct
   {
      // Pointer to the next block in the free list
      union SAMM_header_union* next;
      // Size of the block (in quantas of sizeof(SAMM_header_t))
      size_t size;
   } s;

   // Used to align headers in memory to a boundary
   size_t align_dummy;
};

typedef union SAMM_header_union SAMM_header_t;

// Initial empty list
static SAMM_header_t base = {{0, 0}};

// Start of free list
static SAMM_header_t* freep = 0;

// Static pool for new allocations
static unsigned char SAMM_pool[POOL_SIZE] = {0};
static size_t SAMM_pool_free_pos = 0;

#ifdef DEBUG_SAMM_SUPPORT_STATS
void SAMM_print_stats()
{
   SAMM_header_t* p;

   printf("------ Memory manager stats ------\n\n");
   printf("Pool: free_pos = %lu (%lu bytes left)\n\n", SAMM_pool_free_pos, POOL_SIZE - SAMM_pool_free_pos);

   p = (SAMM_header_t*)SAMM_pool;

   while(p < (SAMM_header_t*)(SAMM_pool + SAMM_pool_free_pos))
   {
      printf("  * Addr: %8p; Size: %8lu\n", p, p->s.size);

      p += p->s.size;
   }

   printf("\nFree list:\n\n");

   if(freep)
   {
      p = freep;

      while(1)
      {
         printf("  * Addr: %8p; Size: %8lu; Next: %8p\n", p, p->s.size, p->s.next);

         p = p->s.next;

         if(p == freep)
            break;
      }
   }
   else
   {
      printf("Empty\n");
   }

   printf("\n");
}
#endif // DEBUG_SAMM_SUPPORT_STATS

void free(void* ap);

static SAMM_header_t* __hide_get_mem_from_pool(size_t nquantas)
{
   size_t total_req_size;

   SAMM_header_t* h;

   if(nquantas < MIN_POOL_ALLOC_QUANTAS)
      nquantas = MIN_POOL_ALLOC_QUANTAS;

   total_req_size = nquantas * sizeof(SAMM_header_t);

   if(SAMM_pool_free_pos + total_req_size <= POOL_SIZE)
   {
      h = (SAMM_header_t*)(SAMM_pool + SAMM_pool_free_pos);
      h->s.size = nquantas;
      free((void*)(h + 1));
      SAMM_pool_free_pos += total_req_size;
   }
   else
   {
      return 0;
   }

   return freep;
}

// Allocations are done in 'quantas' of header size.
// The search for a free block of adequate size begins at the point 'freep'
// where the last block was found.
// If a too-big block is found, it is split and the tail is returned (this
// way the header of the original needs only to have its size adjusted).
// The pointer returned to the user points to the free space within the block,
// which begins one quanta after the header.
//
void* malloc(size_t nbytes)
{
   SAMM_header_t* p;
   SAMM_header_t* prevp;

   // Calculate how many quantas are required: we need enough to house all
   // the requested bytes, plus the header. The -1 and +1 are there to make sure
   // that if nbytes is a multiple of nquantas, we don't allocate too much
   //
   size_t nquantas = (nbytes + sizeof(SAMM_header_t) - 1) / sizeof(SAMM_header_t) + 1;

   // First alloc call, and no free list yet ? Use 'base' for an initial
   // denegerate block of size 0, which points to itself
   //
   if((prevp = freep) == 0)
   {
      base.s.next = freep = prevp = &base;
      base.s.size = 0;
   }

   for(p = prevp->s.next;; prevp = p, p = p->s.next)
   {
      // big enough ?
      if(p->s.size >= nquantas)
      {
         // exactly ?
         if(p->s.size == nquantas)
         {
            // just eliminate this block from the free list by pointing
            // its prev's next to its next
            //
            prevp->s.next = p->s.next;
         }
         else // too big
         {
            p->s.size -= nquantas;
            p += p->s.size;
            p->s.size = nquantas;
         }

         freep = prevp;
         return (void*)(p + 1);
      }
      // Reached end of free list ?
      // Try to allocate the block from the pool. If that succeeds,
      // __hide_get_mem_from_pool adds the new block to the free list and
      // it will be found in the following iterations. If the call
      // to __hide_get_mem_from_pool doesn't succeed, we've run out of
      // memory
      //
      else if(p == freep)
      {
         if((p = __hide_get_mem_from_pool(nquantas)) == 0)
         {
#ifdef DEBUG_SAMM_FATAL
            printf("!! Memory allocation failed !!\n");
#endif
            return 0;
         }
      }
   }
}

// Scans the free list, starting at freep, looking the the place to insert the
// free block. This is either between two existing blocks or at the end of the
// list. In any case, if the block being freed is adjacent to either neighbor,
// the adjacent blocks are combined.
//
void __attribute__((noinline)) free(void* ap)
{
   SAMM_header_t* block;
   SAMM_header_t* p;

   if(ap == NULL)
      return;

   // acquire pointer to block header
   block = ((SAMM_header_t*)ap) - 1;

   // Find the correct place to place the block in (the free list is sorted by
   // address, increasing order)
   //
   for(p = freep; !(block > p && block < p->s.next); p = p->s.next)
   {
      // Since the free list is circular, there is one link where a
      // higher-addressed block points to a lower-addressed block.
      // This condition checks if the block should be actually
      // inserted between them
      //
      if(p >= p->s.next && (block > p || block < p->s.next))
         break;
   }

   // Try to combine with the higher neighbor
   //
   if(block + block->s.size == p->s.next)
   {
      block->s.size += p->s.next->s.size;
      block->s.next = p->s.next->s.next;
   }
   else
   {
      block->s.next = p->s.next;
   }

   // Try to combine with the lower neighbor
   //
   if(p + p->s.size == block)
   {
      p->s.size += block->s.size;
      p->s.next = block->s.next;
   }
   else
   {
      p->s.next = block;
   }

   freep = p;
}

void* memalign(size_t align, size_t len)
{
   void *mem, *new;
   SAMM_header_t* mem_block;
   SAMM_header_t* new_block;

   if((align & -align) != align)
   {
      return 0;
   }

   if(align <= sizeof(union SAMM_header_union))
   {
      if(!(mem = malloc(len)))
         return 0;
      return mem;
   }

   if(!(mem = malloc(len + align - 1)))
      return 0;

   new = (void*)((unsigned int)mem + align - 1 & -align);
   if(new == mem)
      return mem;

   mem_block = ((SAMM_header_t*)mem) - 1;
   new_block = ((SAMM_header_t*)new) - 1;
   new_block->s.size = len;
   mem_block->s.size = new - mem;

   free(mem);
   return new;
}

void* alloca_with_align(size_t len, size_t align)
{
   return memalign(align, len);
}

void*
#if(__GNUC__ > 4)
    __attribute__((optimize("-fno-optimize-strlen")))
#endif
    calloc(size_t nmemb, size_t size)
{
   extern void* memset(void* dest, int val, size_t len);
   unsigned int upsize;
   void* mem;
   unsigned int bitsize, temp = POOL_SIZE;
   count_leading_zero_macro(32, temp, bitsize);
   bitsize = 32 - bitsize;
   unsigned long long int dsize = ((unsigned long long int)(nmemb & ((1ULL << bitsize) - 1))) *
                                  ((unsigned long long int)(size & ((1ULL << bitsize) - 1)));
   unsigned int totsize = (unsigned int)dsize;
   upsize = (dsize >> bitsize) & ((1ULL << bitsize) - 1);

   if(nmemb && upsize != 0)
      return 0;
   mem = malloc(totsize);
   if(mem == 0)
      return 0;
   memset(mem, 0, totsize);
   return mem;
}

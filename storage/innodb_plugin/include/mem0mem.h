/*****************************************************************************

Copyright (c) 1994, 2010, Innobase Oy. All Rights Reserved.

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple
Place, Suite 330, Boston, MA 02111-1307 USA

*****************************************************************************/

/**************************************************//**
@file include/mem0mem.h
The memory management

Created 6/9/1994 Heikki Tuuri
*******************************************************/

#ifndef mem0mem_h
#define mem0mem_h

#include "univ.i"
#include "ut0mem.h"
#include "ut0byte.h"
#include "ut0rnd.h"
#ifndef UNIV_HOTBACKUP
# include "sync0sync.h"
#endif /* UNIV_HOTBACKUP */
#include "ut0lst.h"
#include "mach0data.h"

/* -------------------- MEMORY HEAPS ----------------------------- */

/* The info structure stored at the beginning of a heap block */
typedef struct mem_block_info_struct mem_block_info_t;

/* A block of a memory heap consists of the info structure
followed by an area of memory */
typedef mem_block_info_t	mem_block_t;

/* A memory heap is a nonempty linear list of memory blocks */
typedef mem_block_t	mem_heap_t;

/* A data structure used to cache memory blocks for reuse */
typedef struct mem_block_cache_struct mem_block_cache_t;

/* Types of allocation for memory heaps: DYNAMIC means allocation from the
dynamic memory pool of the C compiler, BUFFER means allocation from the
buffer pool; the latter method is used for very big heaps */

#define MEM_HEAP_DYNAMIC	0	/* the most common type */
#define MEM_HEAP_BUFFER		1
#define MEM_HEAP_BTR_SEARCH	2	/* this flag can optionally be
					ORed to MEM_HEAP_BUFFER, in which
					case heap->free_block is used in
					some cases for memory allocations,
					and if it's NULL, the memory
					allocation functions can return
					NULL. */

/* The following start size is used for the first block in the memory heap if
the size is not specified, i.e., 0 is given as the parameter in the call of
create. The standard size is the maximum (payload) size of the blocks used for
allocations of small buffers. */

#define MEM_BLOCK_START_SIZE		64
#define MEM_BLOCK_STANDARD_SIZE		\
	(UNIV_PAGE_SIZE >= 16384 ? 8000 : MEM_MAX_ALLOC_IN_BUF)

/* If a memory heap is allowed to grow into the buffer pool, the following
is the maximum size for a single allocated buffer: */
#define MEM_MAX_ALLOC_IN_BUF		(UNIV_PAGE_SIZE - 200)

/******************************************************************//**
Initializes the memory system. */
UNIV_INTERN
void
mem_init(
/*=====*/
	ulint	size);	/*!< in: common pool size in bytes */
/******************************************************************//**
Closes the memory system. */
UNIV_INTERN
void
mem_close(void);
/*===========*/

/**************************************************************//**
Use this macro instead of the corresponding function! Macro for memory
heap creation. */

#define mem_heap_create(N)	mem_heap_create_func(\
		(N), MEM_HEAP_DYNAMIC, NULL, __FILE__, __LINE__)

/**************************************************************//**
This macro tries to return a mem_heap_t object whose mem_blocks are
cached. If N <= cache->block_size, then subsequent blocks of this heap are also
cached. If N > cache->block_size then none of the mem_blocks of this heap
is cached. In that case this function would be equivalent to calling
mem_heap_create(). */
#define mem_heap_create_cached(N, cache) mem_heap_create_func(\
		(N), MEM_HEAP_DYNAMIC, (cache), __FILE__, __LINE__)
/**************************************************************//**
Use this macro instead of the corresponding function! Macro for memory
heap creation. */

#define mem_heap_create_in_buffer(N)	mem_heap_create_func(\
		(N), MEM_HEAP_BUFFER, NULL, __FILE__, __LINE__)
/**************************************************************//**
Use this macro instead of the corresponding function! Macro for memory
heap creation. */

#define mem_heap_create_in_btr_search(N)	mem_heap_create_func(\
		(N), MEM_HEAP_BTR_SEARCH | MEM_HEAP_BUFFER, NULL, \
		__FILE__, __LINE__)

/**************************************************************//**
Use this macro instead of the corresponding function! Macro for memory
heap freeing. */

#define mem_heap_free(heap) mem_heap_free_func(\
					  (heap), __FILE__, __LINE__)
/*****************************************************************//**
NOTE: Use the corresponding macros instead of this function. Creates a
memory heap. For debugging purposes, takes also the file name and line as
arguments.
@return own: memory heap, NULL if did not succeed (only possible for
MEM_HEAP_BTR_SEARCH type heaps) */
UNIV_INLINE
mem_heap_t*
mem_heap_create_func(
/*=================*/
	ulint		n,		/*!< in: desired start block size,
					this means that a single user buffer
					of size n will fit in the block,
					0 creates a default size block */
	ulint		type,		/*!< in: heap type */
	mem_block_cache_t* cache, /*!< in: if not NULL, the blocks of this heap will
					be cached instead of being free()'d when mem_heap_free() is called. */
	const char*	file_name,	/*!< in: file name where created */
	ulint		line);		/*!< in: line where created */
/*****************************************************************//**
NOTE: Use the corresponding macro instead of this function. Frees the space
occupied by a memory heap. In the debug version erases the heap memory
blocks. */
UNIV_INLINE
void
mem_heap_free_func(
/*===============*/
	mem_heap_t*	heap,		/*!< in, own: heap to be freed */
	const char*	file_name,	/*!< in: file name where freed */
	ulint		line);		/*!< in: line where freed */
/***************************************************************//**
Allocates and zero-fills n bytes of memory from a memory heap.
@return	allocated, zero-filled storage */
UNIV_INLINE
void*
mem_heap_zalloc(
/*============*/
	mem_heap_t*	heap,	/*!< in: memory heap */
	ulint		n);	/*!< in: number of bytes; if the heap is allowed
				to grow into the buffer pool, this must be
				<= MEM_MAX_ALLOC_IN_BUF */
/***************************************************************//**
Allocates n bytes of memory from a memory heap.
@return allocated storage, NULL if did not succeed (only possible for
MEM_HEAP_BTR_SEARCH type heaps) */
UNIV_INLINE
void*
mem_heap_alloc(
/*===========*/
	mem_heap_t*	heap,	/*!< in: memory heap */
	ulint		n);	/*!< in: number of bytes; if the heap is allowed
				to grow into the buffer pool, this must be
				<= MEM_MAX_ALLOC_IN_BUF */
/*****************************************************************//**
Returns a pointer to the heap top.
@return	pointer to the heap top */
UNIV_INLINE
byte*
mem_heap_get_heap_top(
/*==================*/
	mem_heap_t*	heap);	/*!< in: memory heap */
/*****************************************************************//**
Frees the space in a memory heap exceeding the pointer given. The
pointer must have been acquired from mem_heap_get_heap_top. The first
memory block of the heap is not freed. */
UNIV_INLINE
void
mem_heap_free_heap_top(
/*===================*/
	mem_heap_t*	heap,	/*!< in: heap from which to free */
	byte*		old_top);/*!< in: pointer to old top of heap */
/*****************************************************************//**
Empties a memory heap. The first memory block of the heap is not freed. */
UNIV_INLINE
void
mem_heap_empty(
/*===========*/
	mem_heap_t*	heap);	/*!< in: heap to empty */
/*****************************************************************//**
Returns a pointer to the topmost element in a memory heap.
The size of the element must be given.
@return	pointer to the topmost element */
UNIV_INLINE
void*
mem_heap_get_top(
/*=============*/
	mem_heap_t*	heap,	/*!< in: memory heap */
	ulint		n);	/*!< in: size of the topmost element */
/*****************************************************************//**
Frees the topmost element in a memory heap.
The size of the element must be given. */
UNIV_INLINE
void
mem_heap_free_top(
/*==============*/
	mem_heap_t*	heap,	/*!< in: memory heap */
	ulint		n);	/*!< in: size of the topmost element */
/*****************************************************************//**
Returns the space in bytes occupied by a memory heap. */
UNIV_INLINE
ulint
mem_heap_get_size(
/*==============*/
	mem_heap_t*	heap);		/*!< in: heap */
/**************************************************************//**
Use this macro instead of the corresponding function!
Macro for memory buffer allocation */

#define mem_zalloc(N)	memset(mem_alloc(N), 0, (N));

#define mem_alloc(N)	mem_alloc_func((N), NULL, __FILE__, __LINE__)
#define mem_alloc2(N,S)	mem_alloc_func((N), (S), __FILE__, __LINE__)
/***************************************************************//**
NOTE: Use the corresponding macro instead of this function.
Allocates a single buffer of memory from the dynamic memory of
the C compiler. Is like malloc of C. The buffer must be freed
with mem_free.
@return	own: free storage */
UNIV_INLINE
void*
mem_alloc_func(
/*===========*/
	ulint		n,		/*!< in: requested size in bytes */
	ulint*		size,		/*!< out: allocated size in bytes,
					or NULL */
	const char*	file_name,	/*!< in: file name where created */
	ulint		line);		/*!< in: line where created */

/**************************************************************//**
Use this macro instead of the corresponding function!
Macro for memory buffer freeing */

#define mem_free(PTR)	mem_free_func((PTR), __FILE__, __LINE__)
/***************************************************************//**
NOTE: Use the corresponding macro instead of this function.
Frees a single buffer of storage from
the dynamic memory of C compiler. Similar to free of C. */
UNIV_INLINE
void
mem_free_func(
/*==========*/
	void*		ptr,		/*!< in, own: buffer to be freed */
	const char*	file_name,	/*!< in: file name where created */
	ulint		line);		/*!< in: line where created */

/**********************************************************************//**
Duplicates a NUL-terminated string.
@return	own: a copy of the string, must be deallocated with mem_free */
UNIV_INLINE
char*
mem_strdup(
/*=======*/
	const char*	str);	/*!< in: string to be copied */
/**********************************************************************//**
Makes a NUL-terminated copy of a nonterminated string.
@return	own: a copy of the string, must be deallocated with mem_free */
UNIV_INLINE
char*
mem_strdupl(
/*========*/
	const char*	str,	/*!< in: string to be copied */
	ulint		len);	/*!< in: length of str, in bytes */

/**********************************************************************//**
Duplicates a NUL-terminated string, allocated from a memory heap.
@return	own: a copy of the string */
UNIV_INTERN
char*
mem_heap_strdup(
/*============*/
	mem_heap_t*	heap,	/*!< in: memory heap where string is allocated */
	const char*	str);	/*!< in: string to be copied */
/**********************************************************************//**
Makes a NUL-terminated copy of a nonterminated string,
allocated from a memory heap.
@return	own: a copy of the string */
UNIV_INLINE
char*
mem_heap_strdupl(
/*=============*/
	mem_heap_t*	heap,	/*!< in: memory heap where string is allocated */
	const char*	str,	/*!< in: string to be copied */
	ulint		len);	/*!< in: length of str, in bytes */

/**********************************************************************//**
Concatenate two strings and return the result, using a memory heap.
@return	own: the result */
UNIV_INTERN
char*
mem_heap_strcat(
/*============*/
	mem_heap_t*	heap,	/*!< in: memory heap where string is allocated */
	const char*	s1,	/*!< in: string 1 */
	const char*	s2);	/*!< in: string 2 */

/**********************************************************************//**
Duplicate a block of data, allocated from a memory heap.
@return	own: a copy of the data */
UNIV_INTERN
void*
mem_heap_dup(
/*=========*/
	mem_heap_t*	heap,	/*!< in: memory heap where copy is allocated */
	const void*	data,	/*!< in: data to be copied */
	ulint		len);	/*!< in: length of data, in bytes */

/****************************************************************//**
A simple (s)printf replacement that dynamically allocates the space for the
formatted string from the given heap. This supports a very limited set of
the printf syntax: types 's' and 'u' and length modifier 'l' (which is
required for the 'u' type).
@return	heap-allocated formatted string */
UNIV_INTERN
char*
mem_heap_printf(
/*============*/
	mem_heap_t*	heap,	/*!< in: memory heap */
	const char*	format,	/*!< in: format string */
	...) __attribute__ ((format (printf, 2, 3)));

#ifdef MEM_PERIODIC_CHECK
/******************************************************************//**
Goes through the list of all allocated mem blocks, checks their magic
numbers, and reports possible corruption. */
UNIV_INTERN
void
mem_validate_all_blocks(void);
/*=========================*/
#endif

/*#######################################################################*/

/* The info header of a block in a memory heap */

struct mem_block_info_struct {
	ulint	magic_n;/* magic number for debugging */
	char	file_name[8];/* file name where the mem heap was created */
	ulint	line;	/*!< line number where the mem heap was created */
	mem_block_cache_t* cache; /* The cache to which this block belongs.
			This is NULL if the block is not cached */
	UT_LIST_BASE_NODE_T(mem_block_t) base; /* In the first block in the
			the list this is the base node of the list of blocks;
			in subsequent blocks this is undefined */
	UT_LIST_NODE_T(mem_block_t) list; /* This contains pointers to next
			and prev in the list. The first block allocated
			to the heap is also the first block in this list,
			though it also contains the base node of the list. */
	UT_LIST_NODE_T(mem_block_t) cache_list; /* This contains pointers
			to next and prev blocks in the cache if this block is cached. */
	ulint	len;	/*!< physical length of this block in bytes */
	ulint	total_size; /* physical length in bytes of all blocks
			in the heap. This is defined only in the base
			node and is set to ULINT_UNDEFINED in others. */
	ulint	type;	/*!< type of heap: MEM_HEAP_DYNAMIC, or
			MEM_HEAP_BUF possibly ORed to MEM_HEAP_BTR_SEARCH */
	ulint	free;	/*!< offset in bytes of the first free position for
			user data in the block */
	ulint	start;	/*!< the value of the struct field 'free' at the
			creation of the block */
#ifndef UNIV_HOTBACKUP
	void*	free_block;
			/* if the MEM_HEAP_BTR_SEARCH bit is set in type,
			and this is the heap root, this can contain an
			allocated buffer frame, which can be appended as a
			free block to the heap, if we need more space;
			otherwise, this is NULL */
	void*	buf_block;
			/* if this block has been allocated from the buffer
			pool, this contains the buf_block_t handle;
			otherwise, this is NULL */
#endif /* !UNIV_HOTBACKUP */
#ifdef MEM_PERIODIC_CHECK
	UT_LIST_NODE_T(mem_block_t) mem_block_list;
			/* List of all mem blocks allocated; protected
			by the mem_comm_pool mutex */
#endif
};

/**********************************************************************//**
Data structure for caching mem_blocks of fixed size. It contains a pointer to
the list of all the cached blocks that were allocated before. It also contains
fileds to collect statistics for misses and hits. A hit means a call to
malloc() was saved and a miss means the cache was empty so we needed to call
malloc() */
struct mem_block_cache_struct {
	mutex_t m;
			/* access to the members of mem_block_cache is protected by this mutex */
	UT_LIST_BASE_NODE_T(mem_block_t) base;
			/* pointer to the LIFO queue of malloc()'ed blocks */
	ulint *capacity, block_size, hits, misses;
};

/**********************************************************************//**
Initializes the mem_block_cache. capacity is specified as a pointer so
that it can point to an external integer that can be set outside. */
UNIV_INLINE
void
mem_block_cache_init(
	mem_block_cache_t* cache,
	ulint block_size,
	ulint* capacity);

/**********************************************************************//**
Frees the mem_block_cache and all of the cached mem_blocks in the queue.
Should be called on shutdown. */
UNIV_INLINE
void
mem_block_cache_free(mem_block_cache_t* cache);

/**********************************************************************//**
Returns a mem_block whose size is specified by cache->block_size or NULL
if the cache is empty.
n is the requested memory size. The caller has to make sure that n
is less than cache->block_size.
@return a mem_block from cache or NULL if cache is empty. */
UNIV_INLINE
mem_block_t*
mem_block_cache_find_block(
	mem_block_cache_t* cache,
	ulint n);

/**********************************************************************//**
Adds a block to the cache. The caller must make sure that the block is cached
by making sure that block->cache is not NULL before calling this function.
@return TRUE if the block is added to the cache, FALSE if the cache was full. */
UNIV_INLINE
ibool
mem_block_cache_add_block(
	mem_block_cache_t* cache,
	mem_heap_t* heap,
	mem_block_t* block);

#define MEM_BLOCK_MAGIC_N	764741555
#define MEM_FREED_BLOCK_MAGIC_N	547711122

/* Header size for a memory heap block */
#define MEM_BLOCK_HEADER_SIZE	ut_calc_align(sizeof(mem_block_info_t),\
							UNIV_MEM_ALIGNMENT)
#include "mem0dbg.h"

#ifndef UNIV_NONINL
#include "mem0mem.ic"
#endif

#endif

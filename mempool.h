/* Copyright (c) 2015, Schnusch
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

       1. Redistributions of source code must retain the above copyright notice,
       this list of conditions and the following disclaimer.

       2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

       3. Neither the name of the copyright holder nor the names of its
       contributors may be used to endorse or promote products derived from this
       software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE. */

#ifndef MEMPOOL_H_INCLUDED
#define MEMPOOL_H_INCLUDED

typedef struct memchunk_t memchunk_t;
typedef struct {
	void *(*malloc)(size_t, void *);
	void (*free)(void *, void *);
	void *payload;
	size_t size;
	size_t nmemb;
	memchunk_t *chunk;
} __mempool_transparent_t;

#ifdef __MEMPOOL_TRANSPARENT
typedef __mempool_transparent_t mempool_t;
#else
typedef union {
	char __size[sizeof(__mempool_transparent_t)];
	int __align;
} mempool_t;
#endif

/**
 * Initialize a mempool.
 * \param  pool     mempool
 * \param  nmemb    number of items per mempool chunk
 * \param  size     size of the through the mempool allocatable item
 * \param  malloc   custom malloc function (first parameter is requested size)
 * \param  free     custom free function (first parameter is pointer to free)
 * \param  payload  pointer to pass to \pmalloc or \pfree as second argument
 * \return          0 on success, otherwise -1
 */
int mempool_init(mempool_t *pool, size_t nmemb, size_t size,
		void *(*malloc)(size_t, void *), void (*free)(void *, void *),
		void *payload);

/**
 * Get an object of in mempool_init specified size.
 * \param  pool  mempool
 * \return       pointer to the requested memory or NULL on error
 */
void *mempool_alloc(mempool_t *pool);

/**
 * Free an object allocated by mempool_alloc.
 * \param  pool  mempool
 * \param  p     pointer
 * \return       0 if an object was freed, otherwise -1
 */
int mempool_free_warn(mempool_t *pool, void *p);

/**
 * Free an object allocated by mempool_alloc. (do nothing if pointer is invalid)
 * \param  pool  mempool
 * \param  p     pointer
 */
void mempool_free(mempool_t *pool, void *p);

/**
 * Destroy mempool and free all allocated memory.
 * \param  pool  mempool
 */
void mempool_destroy(mempool_t *pool);

#endif

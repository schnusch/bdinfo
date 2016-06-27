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

#include <stdlib.h>

#define __MEMPOOL_TRANSPARENT
#include "mempool.h"

struct memchunk_t {
	void **free;
	size_t nfree;
	memchunk_t *next;
};

static void *mempool_malloc_wrapper(mempool_t *pool, size_t n)
{
	if(pool->malloc == NULL)
		return malloc(n);
	else
		return pool->malloc(n, pool->payload);
}

static void mempool_free_wrapper(mempool_t *pool, void *p)
{
	if(pool->free == NULL)
		free(p);
	else
		pool->free(p, pool->payload);
}

static void memchunk_free_item(memchunk_t *chunk, void *p)
{
	void **item = p;

	*item = chunk->free;
	chunk->free = item;
	chunk->nfree++;
}

#include <stdio.h>

static memchunk_t *mempool_expand(mempool_t *pool)
{
	memchunk_t *chunk = mempool_malloc_wrapper(pool, sizeof(memchunk_t) + pool->size * pool->nmemb);
	if(chunk == NULL)
		return NULL;
	chunk->free = NULL;
	chunk->nfree = 0;
	chunk->next = NULL;

	void *buf = chunk + 1;
	for(void *p = (char *)buf + pool->size * (pool->nmemb - 1); p >= buf; p = (char *)p - pool->size)
		memchunk_free_item(chunk, p);

	return chunk;
}

int mempool_init(mempool_t *pool, size_t nmemb, size_t size,
		void *(*malloc_)(size_t, void *), void (*free_)(void *, void *),
		void *payload)
{
	pool->malloc  = malloc_;
	pool->free    = free_;
	pool->payload = payload;
	if(size < sizeof(void *))
		pool->size = sizeof(void *);
	else
		pool->size = size;
	pool->nmemb   = nmemb;
	pool->chunk   = mempool_expand(pool);
	return -(pool->chunk == NULL);
}

void *mempool_alloc(mempool_t *pool)
{
	memchunk_t **chunk = &pool->chunk;
	while(*chunk)
	{
		if((*chunk)->nfree > 0)
		{
		flupp:
			// return first free item
			;
			void *p = (*chunk)->free;
			(*chunk)->free = *(void **)(*chunk)->free;
			(*chunk)->nfree--;
			return p;
		}
		else
			chunk = &(*chunk)->next;
	}
	// create new pool
	*chunk = mempool_expand(pool);
	if(*chunk == NULL)
		return NULL;
	goto flupp;
}

int mempool_free_warn(mempool_t *pool, void *p)
{
	size_t nfree = 0;
	memchunk_t **chunk = &pool->chunk;
	while(*chunk)
	{
		// get right chunk
		if(*chunk + 1 <= (memchunk_t *)p
				&& (char *)p < (char *)(*chunk + 1) + pool->nmemb * pool->size)
		{
			// test if its really a valid item
			if(((char *)p - (char *)(*chunk + 1)) % pool->size == 0)
			{
				for(memchunk_t *chunk2 = (*chunk)->next; chunk2; chunk2 = chunk2->next)
					nfree += chunk2->nfree;
				memchunk_free_item(*chunk, p);
				if((*chunk)->nfree == pool->nmemb)
				{
					memchunk_t *tmp = (*chunk)->next;
					mempool_free_wrapper(pool, *chunk);
					*chunk = tmp;
				}
				return 0;
			}
			break;
		}
		nfree += (*chunk)->nfree;
		chunk = &(*chunk)->next;
	}
	return -1;
}

void mempool_free(mempool_t *pool, void *p)
{
	mempool_free_warn(pool, p);
}

void mempool_destroy(mempool_t *pool)
{
	for(memchunk_t *chunk = pool->chunk, *next; chunk; chunk = next)
	{
		next = chunk->next;
		mempool_free_wrapper(pool, chunk);
	}
	pool->chunk = NULL;
}

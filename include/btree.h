/*
 * Copyright (c) 2017, Xiaoye Meng
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * adapted from CLRS, Chapter 18
 */

#ifndef BTREE_INCLUDED
#define BTREE_INCLUDED

/* exported types */
typedef struct btree_t      *btree_t;
typedef struct btree_node_t *btree_node_t;

/* FIXME: exported functions */
extern btree_t       btree_new(int t, int cmp(const void *x, const void *y),
			void kfree(const void *key), void vfree(void *value));
extern void          btree_free(btree_t* bp);
extern unsigned long btree_length(btree_t btree);
extern btree_node_t  btree_sentinel(btree_t btree);
extern int           btree_node_n(btree_node_t node);
extern btree_node_t  btree_node_next(btree_node_t node);
extern const void   *btree_node_key(btree_node_t node, int i);
extern void         *btree_node_value(btree_node_t node, int i);
extern void         *btree_insert(btree_t btree, const void *key, void *value);
extern btree_node_t  btree_find(btree_t btree, const void *key, int *ip);
extern void         *btree_remove(btree_t btree, const void *key);

#endif /* BTREE_INCLUDED */


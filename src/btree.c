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

#include <linux/compiler.h>
#include <linux/list.h>
#include <linux/slab.h>
#include "btree.h"

/* FIXME */
struct btree_t {
	/* minimum degree */
	int			t;
	unsigned long		length;
	btree_node_t		root, sentinel;
	int			(*cmp)(const void *x, const void *y);
	void			(*kfree)(const void *key);
	void			(*vfree)(void *value);
};
struct btree_node_t {
	struct list_head	list;
	unsigned char		leaf:1;
	/* the number of keys currently stored in the node */
	int			n;
	union {
		/* pointers to the node's children */
		btree_node_t	*children;
		struct {
			btree_node_t	prev, next;
		}		pn;
	}			u;
	struct {
		const void	*key;
		void		*value;
	}			*bindings;
};

#define BSEARCH(x, skey, i, k) \
	{ \
		int lo = 0, hi = x->n - 1; \
		while (lo <= hi) { \
			i = (lo + hi) >> 1; \
			if ((k = btree->cmp(skey, x->bindings[i].key)) == 0) \
				break; \
			k > 0 ? (lo = i + 1) : (hi = i - 1); \
		} \
	}

/* FIXME */
static btree_node_t node_new(int t) {
	btree_node_t node;

	if (unlikely((node = kzalloc(sizeof *node, GFP_KERNEL)) == NULL))
		return NULL;
	if ((node->bindings = kzalloc((2 * t - 1) * sizeof *node->bindings, GFP_KERNEL)) == NULL) {
		kfree(node);
		return NULL;
	}
	return node;
}

/* FIXME */
static void node_free(btree_node_t node) {
	if (node) {
		kfree(node->bindings);
		if (!node->leaf)
			kfree(node->u.children);
		kfree(node);
	}
}

static int cmpdefault(const void *x, const void *y) {
	return strcmp((char *)x, (char *)y);
}

static int btree_split_child(btree_t btree, btree_node_t x, int i) {
	btree_node_t y, z;
	int j;

	y = x->u.children[i];
	if ((z = node_new(btree->t)) == NULL)
		return -1;
	if (!y->leaf) {
		z->u.children = kzalloc(2 * btree->t * sizeof *z->u.children, GFP_KERNEL);
		if (z->u.children == NULL) {
			node_free(z);
			return -1;
		}
		/* FIXME: memmove? */
		for (j = 0; j < btree->t; ++j) {
			z->u.children[j] = y->u.children[j + btree->t];
			y->u.children[j + btree->t] = NULL;
		}
	} else {
		z->u.pn.prev = y;
		z->u.pn.next = y->u.pn.next;
		y->u.pn.next->u.pn.prev = z;
		y->u.pn.next = z;
	}
	z->leaf = y->leaf;
	z->n    = btree->t - 1;
	/* FIXME: memmove? */
	for (j = 0; j < btree->t - 1; ++j) {
		z->bindings[j].key   = y->bindings[j + btree->t].key;
		z->bindings[j].value = y->bindings[j + btree->t].value;
		y->bindings[j + btree->t].key   = NULL;
		y->bindings[j + btree->t].value = NULL;
	}
	for (j = x->n; j > i; --j)
		x->u.children[j + 1] = x->u.children[j];
	x->u.children[i + 1] = z;
	for (j = x->n - 1; j > i - 1; --j) {
		x->bindings[j + 1].key   = x->bindings[j].key;
		x->bindings[j + 1].value = x->bindings[j].value;
	}
	x->bindings[i].key   = y->bindings[btree->t - 1].key;
	x->bindings[i].value = y->bindings[btree->t - 1].value;
	if (!y->leaf) {
		y->bindings[btree->t - 1].key   = NULL;
		y->bindings[btree->t - 1].value = NULL;
		y->n = btree->t - 1;
	} else
		y->n = btree->t;
	++x->n;
	return 0;
}

static void *btree_put_nonfull(btree_t btree, btree_node_t node, const void *key, void *value) {
	/* avoid gcc warning */
	int i = -1, j, k = 1;

	while (!node->leaf) {
		BSEARCH(node, key, i, k);
		if (k > 0)
			++i;
		if (node->u.children[i]->n == 2 * btree->t - 1) {
			if (btree_split_child(btree, node, i) == -1)
				return NULL;
			if (btree->cmp(key, node->bindings[i].key) > 0)
				++i;
		}
		node = node->u.children[i];
	}
	i = -1, k = 1;
	BSEARCH(node, key, i, k);
	if (k == 0) {
		void *prev;

		prev = node->bindings[i].value;
		node->bindings[i].value = value;
		return prev;
	} else if (k < 0)
		--i;
	/* think about the first pair got inserted */
	for (j = node->n - 1; j > i; --j) {
		node->bindings[j + 1].key   = node->bindings[j].key;
		node->bindings[j + 1].value = node->bindings[j].value;
	}
	node->bindings[i + 1].key   = key;
	node->bindings[i + 1].value = value;
	++node->n;
	++btree->length;
	return NULL;
}

btree_t btree_new(int t, int cmp(const void *x, const void *y),
	void kfree(const void *key), void vfree(void *value)) {
	btree_node_t x, y;
	btree_t btree;

	/* 2-3-4 tree when t == 2 */
	if (t < 2)
		return NULL;
	if ((x = node_new(t)) == NULL)
		return NULL;
	if ((y = node_new(t)) == NULL) {
		node_free(x);
		return NULL;
	}
	x->leaf      = 1;
	y->leaf      = 1;
	x->n         = 0;
	y->n         = 0;
	x->u.pn.next = x->u.pn.prev = y;
	y->u.pn.next = y->u.pn.prev = x;
	if (unlikely((btree = kmalloc(sizeof *btree, GFP_KERNEL)) == NULL)) {
		node_free(y);
		node_free(x);
		return NULL;
	}
	btree->t        = t;
	btree->length   = 0;
	btree->root     = x;
	btree->sentinel = y;
	btree->cmp      = cmp ? cmp : cmpdefault;
	btree->kfree    = kfree;
	btree->vfree    = vfree;
	return btree;
}

/* FIXME */
void btree_free(btree_t *bp) {
	struct list_head queue;

	if (unlikely(bp == NULL || *bp == NULL))
		return;
	INIT_LIST_HEAD(&queue);
	list_add_tail(&(*bp)->root->list, &queue);
	while (!list_empty(&queue)) {
		btree_node_t node = list_first_entry(&queue, typeof(*node), list);
		int i;

		list_del(&node->list);
		if (!node->leaf)
			for (i = 0; i <= node->n; ++i)
				list_add_tail(&node->u.children[i]->list, &queue);
		else
			for (i = 0; i < node->n; ++i) {
				if ((*bp)->kfree)
					(*bp)->kfree(node->bindings[i].key);
				if ((*bp)->vfree)
					(*bp)->vfree(node->bindings[i].value);
			}
		node_free(node);
	}
	node_free((*bp)->sentinel);
	kfree(*bp);
}

unsigned long btree_length(btree_t btree) {
	if (unlikely(btree == NULL))
		return 0;
	return btree->length;
}

btree_node_t btree_sentinel(btree_t btree) {
	if (unlikely(btree == NULL))
		return NULL;
	return btree->sentinel;
}

int btree_node_n(btree_node_t node) {
	if (unlikely(node == NULL))
		return 0;
	return node->n;
}

btree_node_t btree_node_next(btree_node_t node) {
	if (unlikely(node == NULL))
		return NULL;
	return node->u.pn.next;
}

const void *btree_node_key(btree_node_t node, int i) {
	if (unlikely(node == NULL))
		return NULL;
	return node->bindings[i].key;
}

void *btree_node_value(btree_node_t node, int i) {
	if (unlikely(node == NULL))
		return NULL;
	return node->bindings[i].value;
}

void *btree_insert(btree_t btree, const void *key, void *value) {
	btree_node_t x;

	if (unlikely(btree == NULL || key == NULL))
		return NULL;
	x = btree->root;
	if (x->n == 2 * btree->t - 1) {
		btree_node_t y;

		if ((y = node_new(btree->t)) == NULL)
			return NULL;
		y->leaf       = 0;
		y->n          = 0;
		y->u.children = kzalloc(2 * btree->t * sizeof *y->u.children, GFP_KERNEL);
		if (y->u.children == NULL) {
			node_free(y);
			return NULL;
		}
		y->u.children[0] = x;
		btree->root = y;
		if (btree_split_child(btree, y, 0) == -1)
			return NULL;
		return btree_put_nonfull(btree, y, key, value);
	} else
		return btree_put_nonfull(btree, x, key, value);
}

/* FIXME */
btree_node_t btree_find(btree_t btree, const void *key, int *ip) {
	btree_node_t node;
	/* avoid gcc warning */
	int i = -1, k = 1;

	if (unlikely(btree == NULL || key == NULL))
		return NULL;
	node = btree->root;
	while (!node->leaf) {
		BSEARCH(node, key, i, k);
		if (k > 0)
			++i;
		node = node->u.children[i];
	}
	i = -1, k = 1;
	BSEARCH(node, key, i, k);
	if (k == 0) {
		*ip = i;
		return node;
	}
	return NULL;
}

/* FIXME */
void *btree_remove(btree_t btree, const void *key) {
	return NULL;
}


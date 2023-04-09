/*
libtreestore - a library for reading/writing hierarchical data as text or binary
Copyright (C) 2016-2023 John Tsiombikas <nuclear@mutantstargoat.com>

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
OF SUCH DAMAGE.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "treestor.h"
#include "dynarr.h"

#if defined(__GNUC__) || (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L)
#include <stdint.h>
#elif defined(__unix__)
#include <sys/types.h>
#elif defined(_MSC_VER)
typedef __int8 int8_t;
typedef __int16 int16_t;
typedef __int32 int32_t;
typedef unsigned __int8 uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
#elif defined(__DOS__)
typedef char int8_t;
typedef short int16_t;
typedef long int32_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;
#endif

struct fnode {
	long offs, size, nameid;
	struct ts_node *tsnode;

	struct fnode *chead, *ctail;
	struct fnode *next;
};

struct string_table {
	char **str;
};

static struct fnode *mkftree(struct ts_node *tree, struct string_table *strtab);
static int init_strtab(struct string_table *strtab);
static void destroy_strtab(struct string_table *strtab);
static int stratom(struct string_table *strtab, const char *name);


struct ts_node *ts_bin_load(struct ts_io *io)
{
	return 0;
}

int ts_bin_save(struct ts_node *tree, struct ts_io *io)
{
	int res = -1;
	struct fnode *fileroot;
	struct string_table strtab;

	if(init_strtab(&strtab) == -1) {
		return -1;
	}

	if(!(fileroot = mkftree(tree, &strtab))) {
		goto end;
	}

	/* ... */

	res = 0;
end:
	destroy_strtab(&strtab);
	return res;
}


static struct fnode *mkftree(struct ts_node *tree, struct string_table *strtab)
{
	struct fnode *fnode, *fsub;
	struct ts_node *sub;

	if(!tree) return 0;

	if(!(fnode = calloc(1, sizeof *fnode))) {
		return 0;
	}
	fnode->nameid = stratom(strtab, tree->name);
	fnode->tsnode = tree;

	sub = tree->child_list;
	while(sub) {
		if((fsub = mkftree(sub, strtab))) {
			if(fnode->chead) {
				fnode->ctail->next = fsub;
				fnode->ctail = fsub;
			} else {
				fnode->chead = fnode->ctail = fsub;
			}
		}
		sub = sub->next;
	}

	return fnode;
}

static int init_strtab(struct string_table *strtab)
{
	if(!(strtab->str = ts_dynarr_alloc(0, sizeof *strtab->str))) {
		return -1;
	}
	return 0;
}

static void destroy_strtab(struct string_table *strtab)
{
	int i, count = ts_dynarr_size(strtab->str);

	for(i=0; i<count; i++) {
		free(strtab->str[i]);
	}

	ts_dynarr_free(strtab->str);
}

static int stratom(struct string_table *strtab, const char *name)
{
	int i, count = ts_dynarr_size(strtab->str);
	char *str;
	char **tmptab;

	for(i=0; i<count; i++) {
		if(strcmp(strtab->str[i], name) == 0) {
			return i;
		}
	}

	if(!(str = strdup(name)) || !(tmptab = ts_dynarr_push(strtab->str, &str))) {
		fprintf(stderr, "ts_bin_save: failed to resize string table\n");
		free(str);
		return -1;
	}
	strtab->str = tmptab;
	return count;
}

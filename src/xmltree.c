/*
This file is part of the s-ray renderer <http://code.google.com/p/sray>.
Copyright (C) 2009 John Tsiombikas <nuclear@member.fsf.org>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
/* TODO list
 * - fix cdata handling. make a nodes's cdata into a number of ordered children
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <expat.h>
#include "xmltree.h"

#ifdef _MSC_VER
typedef int ssize_t;
#endif

struct parse_data {
	XML_Parser parser;
	struct xml_node *cur_node;

	char *buf;
	int buf_size;
};

static void start(void *udata, const char *name, const char **atts);
static void end(void *udata, const char *name);
static void cdata(void *udata, const char *text, int len);
static void finish_cdata(struct parse_data *pdata);
static void abort_parsing(XML_Parser p);
static int write_rec(struct xml_node *x, FILE *fp, int lvl);

#define IND_SPACES	4
static void indent(FILE *fp, int lvl);

struct xml_attr *xml_create_attr(const char *name, const char *val)
{
	struct xml_attr *attr;

	if(!(attr = malloc(sizeof *attr))) {
		return 0;
	}
	memset(attr, 0, sizeof *attr);

	if(name) {
		if(!(attr->name = malloc(strlen(name) + 1))) {
			free(attr);
			return 0;
		}
		strcpy(attr->name, name);
	}

	if(val) {
		if(!(attr->str = malloc(strlen(val) + 1))) {
			free(attr->name);
			free(attr);
			return 0;
		}
		strcpy(attr->str, val);

		if(isdigit(val[0]) || ((val[0] == '+' || val[0] == '-') && isdigit(val[1]))) {
			int i;
			attr->type = strchr(val, '.') ? ATYPE_FLT : ATYPE_INT;
			attr->ival = atoi(val);
			attr->fval = atof(val);

			attr->vval[0] = attr->vval[1] = attr->vval[2] = attr->vval[3] = 1.0;

			for(i=0; i<4; i++) {
				if(!*val) break;
				attr->vval[i] = atof(val);

				while(*val && !isspace(*val)) val++;
				while(*val && isspace(*val)) val++;
			}

			if(i > 1) {
				attr->type = ATYPE_VEC;
			}
		}
	}

	return attr;
}

void xml_free_attr(struct xml_attr *attr)
{
	free(attr->name);
	free(attr->str);
	free(attr);
}

void xml_free_attr_list(struct xml_attr *alist)
{
	while(alist) {
		struct xml_attr *tmp = alist;
		alist = alist->next;

		xml_free_attr(tmp);
	}
}

struct xml_attr *xml_get_attr(struct xml_node *node, const char *attr_name)
{
	struct xml_attr *attr;

	attr = node->attr;
	while(attr) {
		if(strcmp(attr->name, attr_name) == 0) {
			return attr;
		}
		attr = attr->next;
	}
	return 0;
}

struct xml_node *xml_create_tree(void)
{
	struct xml_node *x;

	if(!(x = malloc(sizeof *x))) {
		return 0;
	}
	memset(x, 0, sizeof *x);

	return x;
}

void xml_free_tree(struct xml_node *x)
{
	while(x->chld) {
		void *tmp = x->chld;

		x->chld = x->chld->next;
		xml_free_tree(tmp);
	}

	while(x->attr) {
		struct xml_attr *tmp = x->attr;
		x->attr = x->attr->next;

		free(tmp->name);
		free(tmp->str);
		free(tmp);
	}

	free(x->cdata);
	free(x->name);
	free(x);
}

struct xml_node *xml_read_tree(const char *fname)
{
	FILE *fp;
	struct xml_node *xml;

	if(!(fp = fopen(fname, "rb"))) {
		return 0;
	}
	xml = xml_read_tree_file(fp);
	fclose(fp);
	return xml;
}

struct xml_node *xml_read_tree_file(FILE *fp)
{
	struct parse_data pdata;
	struct xml_node node, *tree;
	XML_Parser p;
	ssize_t rdsz;
	void *buf;

	memset(&node, 0, sizeof node);

	if(!(p = XML_ParserCreate(0))) {
		return 0;
	}
	XML_SetElementHandler(p, start, end);
	XML_SetCharacterDataHandler(p, cdata);
	XML_SetUserData(p, &pdata);

	memset(&pdata, 0, sizeof pdata);
	pdata.parser = p;
	pdata.cur_node = &node;

	do {
		if(!(buf = XML_GetBuffer(p, 4096))) {
			break;
		}

		if((rdsz = fread(buf, 1, 4096, fp)) == -1) {
			break;
		}

		if(!XML_ParseBuffer(p, rdsz, rdsz < 4096)) {
			fprintf(stderr, "XML parsing error: %d: %s\n", (int)XML_GetCurrentLineNumber(p),
					XML_ErrorString(XML_GetErrorCode(p)));
			break;
		}
	} while(rdsz == 4096);

	tree = node.chld;
	if(tree) {
		assert(tree->next == 0);
	}

	if(pdata.cur_node != &node) {	/* aborted */
		xml_free_tree(tree);
		tree = 0;
	}

	if(pdata.buf) {
		free(pdata.buf);
	}

	XML_ParserFree(p);
	return tree;
}

static void start(void *udata, const char *name, const char **atts)
{
	struct xml_node *node = 0;
	struct parse_data *pdata = udata;

	finish_cdata(pdata);

	if(!(node = malloc(sizeof *node))) {
		goto err;
	}
	memset(node, 0, sizeof *node);

	if(!(node->name = malloc(strlen(name) + 1))) {
		goto err;
	}
	strcpy(node->name, name);

	while(*atts) {
		struct xml_attr *attr;

		if(!(attr = xml_create_attr(atts[0], atts[1]))) {
			goto err;
		}
		attr->next = node->attr;
		node->attr = attr;

		atts += 2;
	}

	xml_add_child(pdata->cur_node, node);
	pdata->cur_node = node;
	return;

err:
	if(node) {
		free(node->name);
		xml_free_attr_list(node->attr);
	}
	free(node);
	abort_parsing(pdata->parser);
}

static void end(void *udata, const char *name)
{
	struct parse_data *pdata = udata;

	finish_cdata(pdata);

	pdata->cur_node = pdata->cur_node->up;
}

static void cdata(void *udata, const char *text, int len)
{
	char *tmp;
	struct parse_data *pdata = udata;

	if(!(tmp = realloc(pdata->buf, pdata->buf_size + len))) {
		abort_parsing(pdata->parser);
		return;
	}
	memcpy(tmp + pdata->buf_size, text, len);
	pdata->buf = tmp;
	pdata->buf_size += len;
}

static void finish_cdata(struct parse_data *pdata)
{
	char *tmp;

	if(!pdata->buf) {
		return;
	}

	if(!(tmp = realloc(pdata->buf, pdata->buf_size + 1))) {
		abort_parsing(pdata->parser);
		return;
	}
	tmp[pdata->buf_size] = 0;
	pdata->cur_node->cdata = tmp;

	pdata->buf = 0;
	pdata->buf_size = 0;
}

static void abort_parsing(XML_Parser p)
{
	perror("abort XML parsing");

	XML_SetElementHandler(p, 0, 0);
	XML_SetCharacterDataHandler(p, 0);
	XML_StopParser(p, 0);
}

int xml_write_tree(struct xml_node *x, const char *fname)
{
	FILE *fp;
	int res;

	if(!(fp = fopen(fname, "wb"))) {
		return -1;
	}
	res = xml_write_tree_file(x, fp);
	fclose(fp);
	return res;
}

int xml_write_tree_file(struct xml_node *x, FILE *fp)
{
	return write_rec(x, fp, 0);
}

static int write_rec(struct xml_node *x, FILE *fp, int lvl)
{
	struct xml_node *c;
	struct xml_attr *attr;

	indent(fp, lvl);
	fputc('<', fp);
	fputs(x->name, fp);

	attr = x->attr;
	while(attr) {
		switch(attr->type) {
		case ATYPE_INT:
			fprintf(fp, " %s=\"%d\"", attr->name, attr->ival);
			break;

		case ATYPE_FLT:
			fprintf(fp, " %s=\"%.4f\"", attr->name, attr->fval);
			break;

		case ATYPE_VEC:
			fprintf(fp, " %s=\"%.4f %.4f %.4f %.4f\"", attr->name, attr->vval[0], 
					attr->vval[1], attr->vval[2], attr->vval[3]);
			break;
		
		case ATYPE_STR:
		default:
			fprintf(fp, " %s=\"%s\"", attr->name, attr->str);
			break;
		}

		attr = attr->next;
	}

	if(!x->chld && !x->cdata) {
		fputc('/', fp);
	}
	fprintf(fp, ">\n");

	if(x->cdata) {
		indent(fp, lvl + 1);
		fprintf(fp, "%s\n", x->cdata);
	}

	c = x->chld;
	while(c) {
		write_rec(c, fp, lvl + 1);
		c = c->next;
	}

	if(x->chld || x->cdata) {
		indent(fp, lvl);
		fprintf(fp, "</%s>\n", x->name);
	}
	return 0;
}

void xml_add_child(struct xml_node *x, struct xml_node *chld)
{
	chld->next = 0;

	if(x->chld) {
		x->chld_tail->next = chld;
		x->chld_tail = chld;
	} else {
		x->chld = x->chld_tail = chld;
	}

	chld->up = x;
	x->chld_count++;
}

void xml_remove_subtree(struct xml_node *sub)
{
	struct xml_node *x;

	if(sub->up) {
		x = sub->up->chld;
		if(x == sub) {
			sub->up->chld = sub->next;
		} else {
			while(x) {
				if(x->next == sub) {
					x->next = x->next->next;
					break;
				}
				x = x->next;
			}
		}
	}

	sub->up->chld_count--;
	sub->up = 0;
}

static void indent(FILE *fp, int lvl)
{
	int i, ind = lvl * IND_SPACES;

	for(i=0; i<ind; i++) {
		fputc(' ', fp);
	}
}

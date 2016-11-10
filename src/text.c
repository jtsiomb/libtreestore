#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "treestore.h"

enum { TOK_SYM, TOK_ID, TOK_NUM, TOK_STR };

static struct ts_node *read_node(FILE *fp);
static int next_token(FILE *fp, char *buf, int bsz);

static void print_attr(struct ts_attr *attr, FILE *fp, int level);
static void print_value(struct ts_value *value, FILE *fp);
static int tree_level(struct ts_node *n);
static const char *indent(int x);

struct ts_node *ts_text_load(FILE *fp)
{
	char token[256];
	struct ts_node *node;

	if(next_token(fp, token, sizeof token) != TOK_ID ||
			!(next_token(fp, token, sizeof token) == TOK_SYM && token[0] == '{')) {
		fprintf(stderr, "ts_text_load: invalid file format\n");
		return 0;
	}
	if(!(node = read_node(fp))) {
		return 0;
	}
	if(!(node->name = strdup(token))) {
		perror("failed to allocate node name");
		free(node);
		return 0;
	}
	return node;
}


#define EXPECT(type) \
	if(next_token(fp, token, sizeof token) != (type)) goto err
#define EXPECT_SYM(c) \
	if(next_token(fp, token, sizeof token) != TOK_SYM || token[0] != (c)) goto err

static struct ts_node *read_node(FILE *fp)
{
	char token[256];
	struct ts_node *node;

	if(!(node = ts_alloc_node())) {
		perror("failed to allocate treestore node");
		return 0;
	}

	while(next_token(fp, token, sizeof token) == TOK_ID) {
		char *id;

		if(!(id = strdup(token))) {
			goto err;
		}

		EXPECT(TOK_SYM);

		if(token[0] == '=') {
			/* attribute */
			struct ts_attr *attr;
			int toktype;

			if(!(attr = ts_alloc_attr())) {
				goto err;
			}

			if((toktype = next_token(fp, token, sizeof token)) == -1) {
				ts_free_attr(attr);
				goto err;
			}
			attr->name = id;
			ts_set_value(&attr->val, token);

			ts_add_attr(node, attr);

		} else if(token[0] == '{') {
			/* child */
			struct ts_node *child;

			if(!(child = read_node(fp))) {
				ts_free_node(node);
				return 0;
			}
			child->name = id;

			ts_add_child(node, child);

		} else {
			fprintf(stderr, "unexpected token: %s\n", token);
			goto err;
		}
	}

err:
	fprintf(stderr, "treestore read_node failed\n");
	ts_free_node(node);
	return 0;
}

static int next_token(FILE *fp, char *buf, int bsz)
{
	int c;
	char *ptr;

	// skip whitespace
	while((c = fgetc(fp)) != -1 && isspace(c)) {
		if(c == '#') { // skip to end of line
			while((c = fgetc(fp)) != -1 && c != '\n');
			if(c == -1) return -1;
		}
	}
	if(c == -1) return -1;

	buf[0] = c;
	buf[1] = 0;

	if(isdigit(c)) {
		// token is a number
		ptr = buf + 1;
		while((c = fgetc(fp)) != -1 && isdigit(c)) {
			*ptr++ = c;
		}
		if(c != -1) ungetc(c, fp);
		*ptr = 0;
		return TOK_NUM;
	}
	if(isalpha(c)) {
		// token is an identifier
		ptr = buf + 1;
		while((c = fgetc(fp)) != -1 && isalnum(c)) {
			*ptr++ = c;
		}
		if(c != -1) ungetc(c, fp);
		*ptr = 0;
		return TOK_ID;
	}
	if(c == '"') {
		// token is a string constant
		ptr = buf;
		while((c = fgetc(fp)) != -1 && c != '"' && c != '\r' && c != '\n') {
			*ptr++ = c;
		}
		if(c != '"') {
			return -1;
		}
		*ptr = 0;
		return TOK_STR;
	}

	return TOK_SYM;
}

int ts_text_save(struct ts_node *tree, FILE *fp)
{
	struct ts_node *c;
	struct ts_attr *attr;
	int lvl = tree_level(tree);

	fprintf(fp, "%s%s {\n", indent(lvl), tree->name);

	attr = tree->attr_list;
	while(attr) {
		print_attr(attr, fp, lvl);
		attr = attr->next;
	}

	c = tree->child_list;
	while(c) {
		ts_text_save(c, fp);
	}

	fprintf(fp, "%s}\n", indent(lvl));
}

static void print_attr(struct ts_attr *attr, FILE *fp, int level)
{
	fprintf(fp, "%s%s = ", indent(level + 1), attr->name);
	print_value(&attr->val, fp);
	fputc('\n', fp);
}

static void print_value(struct ts_value *value, FILE *fp)
{
	int i;

	switch(value->type) {
	case TS_NUMBER:
		fprintf(fp, "%g", value->fnum);
		break;

	case TS_VECTOR:
		fputc('[', fp);
		for(i=0; i<value->vec_size; i++) {
			if(i == 0) {
				fprintf(fp, "%g", value->vec[i]);
			} else {
				fprintf(fp, ", %g", value->vec[i]);
			}
		}
		fputc(']', fp);
		break;

	case TS_ARRAY:
		fputc('[', fp);
		for(i=0; i<value->array_size; i++) {
			if(i > 0) {
				fprintf(fp, ", ");
			}
			print_value(value->array + i, fp);
		}
		fputc(']', fp);
		break;

	default:
		fputs(value->str, fp);
	}
}

static int tree_level(struct ts_node *n)
{
	if(!n->parent) return 0;
	return tree_level(n->parent) + 1;
}

static const char *indent(int x)
{
	static const char *buf = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
	const char *end = buf + sizeof buf - 1;
	return x > sizeof buf - 1 ? buf : end - x;
}

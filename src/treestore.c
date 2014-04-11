#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "treestore.h"

/* ---- ts_value implementation ---- */

int ts_init_value(struct ts_value *tsv)
{
	memset(tsv, 0, sizeof *tsv);
	return 0;
}

void ts_destroy_value(struct ts_value *tsv)
{
	int i;

	free(tsv->str);
	free(tsv->vec);

	for(i=0; i<tsv->array_size; i++) {
		ts_destroy_value(tsv->array + i);
	}
}


struct ts_value *ts_alloc_value(void)
{
	struct ts_value *v = malloc(sizeof *v);
	if(!v || ts_init_value(v) == -1) {
		free(v);
		return 0;
	}
	return v;
}

void ts_free_value(struct ts_value *tsv)
{
	ts_destroy_value(tsv);
	free(tsv);
}


int ts_copy_value(struct ts_value *dest, struct ts_value *src)
{
	int i;

	if(dest == src) return 0;

	*dest = *src;

	dest->str = 0;
	dest->vec = 0;
	dest->array = 0;

	if(src->str) {
		if(!(dest->str = malloc(strlen(src->str) + 1))) {
			goto fail;
		}
		strcpy(dest->str, src->str);
	}
	if(src->vec && src->vec_size > 0) {
		if(!(dest->vec = malloc(src->vec_size * sizeof *src->vec))) {
			goto fail;
		}
		memcpy(dest->vec, src->vec, src->vec_size * sizeof *src->vec);
	}
	if(src->array && src->array_size > 0) {
		if(!(dest->array = calloc(src->array_size, sizeof *src->array))) {
			goto fail;
		}
		for(i=0; i<src->array_size; i++) {
			if(ts_copy_value(dest->array + i, src->array + i) == -1) {
				goto fail;
			}
		}
	}
	return 0;

fail:
	free(dest->str);
	free(dest->vec);
	if(dest->array) {
		for(i=0; i<dest->array_size; i++) {
			ts_destroy_value(dest->array + i);
		}
		free(dest->array);
	}
	return -1;
}


int ts_set_value(struct ts_value *tsv, const char *str)
{
	if(tsv->str) {
		ts_destroy_value(tsv);
		if(ts_init_value(tsv) == -1) {
			return -1;
		}
	}

	if(!(tsv->str = malloc(strlen(str) + 1))) {
		return -1;
	}
	strcpy(tsv->str, str);
	return 0;
}

int ts_set_valueiv(struct ts_value *tsv, int count, ...)
{
	int res;
	va_list ap;
	va_start(ap, count);
	res = ts_set_valueiv_va(tsv, count, ap);
	va_end(ap);
	return res;
}

#define MAKE_NUMSTR_FUNC(typestr, fmt) \
	static char *make_##typestr##str(int x) \
	{ \
		static char scrap[128]; \
		char *str; \
		int sz = snprintf(scrap, sizeof scrap, fmt, x); \
		if(!(str = malloc(sz + 1))) return 0; \
		sprintf(str, fmt, x); \
		return str; \
	}

MAKE_NUMSTR_FUNC(int, "%d")
MAKE_NUMSTR_FUNC(float, "%d")

#define ARGS_ARE_INT	((enum ts_value_type)42)

int ts_set_valueiv_va(struct ts_value *tsv, int count, va_list ap)
{
	if(count < 1) return -1;
	if(count == 1) {
		int num = va_arg(ap, int);
		if(!(tsv->str = make_intstr(tsv->inum))) {
			return -1;
		}

		tsv->type = TS_NUMBER;
		tsv->inum = num;
		tsv->fnum = (float)num;
		return 0;
	}

	/* otherwise it's an array, let ts_set_valuefv_va handle it */
	/* XXX: va_arg will need to be called with int instead of float. set a special
	 *      value to the type field before calling this, to signify that.
	 */
	tsv->type = ARGS_ARE_INT;
	return ts_set_valuefv_va(tsv, count, ap);
}

int ts_set_valuei(struct ts_value *tsv, int inum)
{
	return ts_set_valueiv(tsv, 1, inum);
}


int ts_set_valuefv(struct ts_value *tsv, int count, ...)
{
	int res;
	va_list ap;
	va_start(ap, count);
	res = ts_set_valuefv_va(tsv, count, ap);
	va_end(ap);
	return res;
}

int ts_set_valuefv_va(struct ts_value *tsv, int count, va_list ap)
{
	int i;

	if(count < 1) return -1;
	if(count == 1) {
		int num = va_arg(ap, int);
		if(!(tsv->str = make_floatstr(tsv->inum))) {
			return -1;
		}

		tsv->type = TS_NUMBER;
		tsv->inum = num;
		tsv->fnum = (float)num;
		return 0;
	}

	/* otherwise it's an array, we need to create the ts_value array, and
	 * the simplified vector
	 */
	if(!(tsv->vec = malloc(count * sizeof *tsv->vec))) {
		return -1;
	}
	tsv->vec_size = count;

	for(i=0; i<count; i++) {
		if(tsv->type == ARGS_ARE_INT) {	/* only when called by ts_set_valueiv_va */
			tsv->vec[i] = (float)va_arg(ap, int);
		} else {
			tsv->vec[i] = va_arg(ap, double);
		}
	}

	if(!(tsv->array = malloc(count * sizeof *tsv->array))) {
		free(tsv->vec);
	}
	tsv->array_size = count;

	for(i=0; i<count; i++) {
		ts_init_value(tsv->array + i);
		if(tsv->type == ARGS_ARE_INT) {	/* only when called by ts_set_valueiv_va */
			ts_set_valuei(tsv->array + i, (int)tsv->vec[i]);
		} else {
			ts_set_valuef(tsv->array + i, tsv->vec[i]);
		}
	}

	tsv->type = TS_VECTOR;
	return 0;
}

int ts_set_valuef(struct ts_value *tsv, int fnum)
{
	return ts_set_valuefv(tsv, 1, fnum);
}


int ts_set_valuev(struct ts_value *tsv, int count, ...)
{
	int res;
	va_list ap;
	va_start(ap, count);
	res = ts_set_valuev_va(tsv, count, ap);
	va_end(ap);
	return res;
}

int ts_set_valuev_va(struct ts_value *tsv, int count, va_list ap)
{
	if(count <= 1) return -1;
	return -1;	/* TODO */
}

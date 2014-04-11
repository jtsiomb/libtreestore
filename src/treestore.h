#ifndef TREESTORE_H_
#define TREESTORE_H_

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

enum ts_value_type { TS_UNKNOWN, TS_NUMBER, TS_VECTOR, TS_ARRAY };

/** treestore node attribute value */
struct ts_value {
	enum ts_value_type type;

	char *str;		/**< all values have a string representation */
	int inum;		/**< numeric values (TS_INT/TS_FLOAT) will have this set */
	float fnum;		/**< numeric values (TS_INT/TS_FLOAT) will have this set */

	/** vector values (arrays containing ONLY numbers) will have this set */
	float *vec;		/**< elements of the vector */
	int vec_size;	/**< size of the vector (in elements), same as array_size */

	/** array values (including vectors) will have this set */
	struct ts_value *array;	/**< elements of the array */
	int array_size;			/**< size of the array (in elements) */
};

int ts_init_value(struct ts_value *tsv);
void ts_destroy_value(struct ts_value *tsv);

struct ts_value *ts_alloc_value(void);		/**< also calls ts_init_value */
void ts_free_value(struct ts_value *tsv);	/**< also calls ts_destroy_value */

/** perform a deep-copy of a ts_value */
int ts_copy_value(struct ts_value *dest, struct ts_value *src);

/** ts_set_value will try to parse the string and initialize the value type fields */
int ts_set_value(struct ts_value *tsv, const char *str);

/** set a ts_value from a list of integers */
int ts_set_valueiv(struct ts_value *tsv, int count, ...);
int ts_set_valueiv_va(struct ts_value *tsv, int count, va_list ap);
int ts_set_valuei(struct ts_value *tsv, int inum);	/**< equiv: ts_set_valueiv(val, 1, inum) */

/** set a ts_value from a list of floats */
int ts_set_valuefv(struct ts_value *tsv, int count, ...);
int ts_set_valuefv_va(struct ts_value *tsv, int count, va_list ap);
int ts_set_valuef(struct ts_value *tsv, int fnum);	/**< equiv: ts_set_valuefv(val, 1, fnum) */

/** set a ts_value from a list of ts_value pointers. they are deep-copied as per ts_copy_value */
int ts_set_valuev(struct ts_value *tsv, int count, ...);
int ts_set_valuev_va(struct ts_value *tsv, int count, va_list ap);


/** treestore node attribute */
struct ts_attr {
	char *name;

	struct ts_value val;

	struct ts_attr *next;
};


/** treestore node */
struct ts_node {
	char *name;

	int attr_count;
	struct ts_attr *attr_list, *attr_tail;

	int child_count;
	struct ts_node *child_list, *child_tail;
	struct ts_node *parent;

	struct ts_node *next;	/* next sibling */
};

struct ts_node *ts_create_node(void);
void ts_free_node(struct ts_node *n);
void ts_free_tree(struct ts_node *tree);

#ifdef __cplusplus
}
#endif

#endif	/* TREESTORE_H_ */

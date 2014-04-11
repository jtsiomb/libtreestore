#ifndef TREESTOREPP_H_
#define TREESTOREPP_H_

#include "treestore.h"

/// wraps a C ts_value in a convenient class
class TSValue {
private:
	ts_value *ctsv;

public:
	TSValue();
	~TSValue();

	TSValue(const TSValue &tsv);
	TSValue &operator =(const TSValue &tsv);

#ifdef TS_USE_CPP11
	TSValue(const TSValue &&tsv);
	TSValue &operator =(const TSValue &&tsv);
#endif

	bool set(const char *str);
	bool set_int(int inum);
	bool set_int(int count, ...);
	bool set_float(float fnum);
	bool set_float(int count, ...);
	bool set_array(int count, const TSValue &v0, ...);

	const char *get() const;

	int get_int() const;
	int *get_intv() const;

	float get_float() const;
	float *get_floatv() const;

	const TSValue *get_array() const;
	int get_array_size() const;

	int get_vec_size() const;	//< equiv: get_array_size */
};

#endif	// TREESTOREPP_H_

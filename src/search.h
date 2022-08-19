#ifndef MATTER_SEARCH
#define MATTER_SEARCH

#define R_NO_REMAP

#include <R.h>

#include <cmath>
#include <cfloat>

#include "utils.h"
#include "signal.h"

#define SEARCH_ERROR -1

//// Linear search
//-----------------

// fuzzy linear search (fallback for unsorted arrays)
template<typename T>
index_t linear_search(T x, T * table, size_t start, size_t end,
	double tol, int tol_ref, int nomatch, bool nearest = FALSE, bool ind1 = FALSE)
{
	index_t pos = nomatch;
	double diff, diff_min = DBL_MAX;
	for ( size_t i = start; i < end; i++ )
	{
		diff = rel_diff(x, table[i], tol_ref);
		if ( diff == 0 )
			return i + ind1;
		if ( diff < diff_min ) {
			diff_min = diff;
			pos = i;
		}
	}
	if ( diff_min < tol || nearest )
		return pos + ind1;
	else
		return nomatch;
}

template<typename T>
index_t linear_search(T x, SEXP table, double tol, int tol_ref,
	int nomatch, bool nearest = FALSE, bool ind1 = FALSE)
{
	return linear_search(x, DataPtr<T>(table), 0, XLENGTH(table),
		tol, tol_ref, nomatch, nearest, ind1);
}

template<typename T>
index_t do_linear_search(int * ptr, T * x, size_t xlen, T * table,
	size_t start, size_t end, double tol, int tol_ref, int nomatch,
	bool nearest = FALSE, bool ind1 = FALSE)
{
	size_t num_matches = 0;
	for ( size_t i = 0; i < xlen; i++ ) {
		if ( isNA(x[i]) )
			ptr[i] = nomatch;
		else {
			index_t pos = linear_search<T>(x[i], table, start, end,
				tol, tol_ref, nomatch, nearest, ind1);
			ptr[i] = pos;
			num_matches++;
		}
	}
	return num_matches;
}

template<typename T>
index_t do_linear_search(int * ptr, T * x, size_t xlen, SEXP table,
	double tol, int tol_ref, int nomatch, bool nearest = FALSE, bool ind1 = FALSE)
{
	return do_linear_search(ptr, x, xlen, DataPtr<T>(table),
		0, XLENGTH(table), tol, tol_ref, nomatch, nearest, ind1);
}

//// Binary search
//-----------------

// fuzzy binary search returning position of 'x' in 'table'
template<typename T>
index_t binary_search(T x, T * table, size_t start, size_t end,
	double tol, int tol_ref, int nomatch, bool nearest = FALSE,
	bool ind1 = FALSE, int err = SEARCH_ERROR)
{
	double diff;
	index_t min = start, max = end, mid = nomatch;
	while ( start < end )
	{
		mid = start + (end - start) / 2;
		double d1 = rel_change(table[start], table[mid]);
		double d2 = rel_change(table[mid], table[end - 1]);
		if ( d1 > 0 || d2 > 0 )
			return err; // table is not sorted
		diff = rel_change(x, table[mid], tol_ref);
		if ( diff < 0 )
			end = mid;
		else if ( diff > 0 )
			start = mid + 1;
		else
			return mid + ind1;
	}
	if ( (nearest || tol > 0) && (max - min) > 0 )
	{
		index_t left = mid >= min + 1 ? mid - 1 : min;
		index_t right = mid < max - 1 ? mid + 1 : max - 1;
		double dleft = rel_diff(x, table[left], tol_ref);
		double dmid = rel_diff(x, table[mid], tol_ref);
		double dright = rel_diff(x, table[right], tol_ref);
		if ( (mid == left && diff < 0) && (nearest || dleft < tol) )
			return left + ind1;
		else if ( (mid == right && diff > 0) && (nearest || dright < tol) )
			return right + ind1;
		else {
			if ( (dleft <= dmid && dleft <= dright) && (nearest || dleft < tol) )
				return left + ind1;
			else if ( (dmid <= dleft && dmid <= dright) && (nearest || dmid < tol) )
				return mid + ind1;
			else if ( nearest || dright < tol )
				return right + ind1;
		}
	}
	return nomatch;
}

template<typename T>
index_t binary_search(T x, SEXP table, double tol, int tol_ref, int nomatch,
	bool nearest = FALSE, bool ind1 = FALSE, int err = SEARCH_ERROR)
{
	return binary_search(x, DataPtr<T>(table), 0, XLENGTH(table),
		tol, tol_ref, nomatch, nearest, ind1, err);
}

template<typename T>
index_t do_binary_search(int * ptr, T * x, size_t xlen, T * table,
	size_t start, size_t end, double tol, int tol_ref, int nomatch,
	bool nearest = FALSE, bool ind1 = FALSE, int err = SEARCH_ERROR)
{
	size_t num_matches = 0;
	for ( size_t i = 0; i < xlen; i++ ) {
		if ( isNA(x[i]) )
			ptr[i] = nomatch;
		else {
			index_t pos = binary_search<T>(x[i], table, start, end,
				tol, tol_ref, nomatch, nearest, ind1, err);
			if ( pos != err ) {
				ptr[i] = pos;
				num_matches++;
			}
			else
				return err;
		}
	}
	return num_matches;
}

template<typename T>
index_t do_binary_search(int * ptr, T * x, size_t xlen, SEXP table,
	double tol, int tol_ref, int nomatch, bool nearest = FALSE,
	bool ind1 = FALSE, int err = SEARCH_ERROR)
{
	return do_binary_search(ptr, x, xlen, DataPtr<T>(table),
		0, XLENGTH(table), tol, tol_ref, nomatch, nearest, ind1, err);
}

template<typename T>
SEXP do_binary_search(SEXP x, SEXP table, double tol, int tol_ref,
	int nomatch, bool nearest = FALSE, bool ind1 = FALSE)
{
	SEXP pos;
	PROTECT(pos = Rf_allocVector(INTSXP, XLENGTH(x)));
	do_binary_search<T>(INTEGER(pos), DataPtr<T>(x), XLENGTH(x),
		table, tol, tol_ref, nomatch, nearest, ind1);
	UNPROTECT(1);
	return pos;
}

//// Approximate search
//----------------------

// search for 'values' indexed by 'keys' w/ interpolation
template<typename Tkey, typename Tval>
Pair<index_t,Tval> approx_search(Tkey x, Tkey * keys, Tval * values,
	size_t start, size_t end, double tol, int tol_ref, Tval nomatch,
	int interp = EST_NEAR, bool sorted = TRUE)
{
	index_t pos = NA_INTEGER;
	Tval val = nomatch;
	Pair<index_t,Tval> result;
	if ( isNA(x) )
		return result;
	if ( sorted )
		pos = binary_search<Tkey>(x, keys,
			start, end, tol, tol_ref, NA_INTEGER);
	else
		pos = linear_search<Tkey>(x, keys,
			start, end, tol, tol_ref, NA_INTEGER);
	if ( !isNA(pos) && pos >= 0 )
	{
		if ( tol > 0 )
			val = interp1<Tkey,Tval>(x, keys, values,
				pos, end, tol, tol_ref, interp, sorted);
		else
			val = values[pos];
	}
	result = {pos, val};
	return result;
}

template<typename Tkey, typename Tval>
Pair<index_t,Tval> approx_search(Tkey x, SEXP keys, SEXP values,
	double tol, int tol_ref, Tval nomatch, int interp = EST_NEAR, bool sorted = TRUE)
{
	return approx_search(x, DataPtr<Tkey>(keys), DataPtr<Tval>(values),
		0, XLENGTH(keys), tol, tol_ref, nomatch, interp, sorted);
}

template<typename Tkey, typename Tval>
index_t do_approx_search(Tval * ptr, Tkey * x, size_t xlen, Tkey * keys, Tval * values,
	size_t start, size_t end, double tol, int tol_ref, Tval nomatch, int interp = EST_NEAR,
	bool sorted = TRUE, size_t stride = 1)
{
	index_t num_matches;
	int pos [xlen];
	if ( sorted )
	{
		num_matches = do_binary_search(pos, x, xlen,
			keys, start, end, tol, tol_ref, NA_INTEGER);
		if ( num_matches == SEARCH_ERROR )
			sorted = FALSE;
	}
	if ( !sorted )
		num_matches = do_linear_search(pos, x, xlen,
			keys, start, end, tol, tol_ref, NA_INTEGER);
	for ( size_t i = 0; i < xlen; i++ )
	{
		if ( isNA(pos[i]) )
			ptr[i * stride] = nomatch;
		else
			ptr[i * stride] = interp1<Tkey,Tval>(x[i], keys, values,
				pos[i], end, tol, tol_ref, interp, sorted);
	}
	return num_matches;
}

template<typename Tkey, typename Tval>
index_t do_approx_search(Tval * ptr, Tkey * x, size_t xlen, SEXP keys, SEXP values,
	double tol, int tol_ref, Tval nomatch, int interp = EST_NEAR,
	bool sorted = TRUE, size_t stride = 1)
{
	return do_approx_search(ptr, x, xlen, DataPtr<Tkey>(keys), DataPtr<Tval>(values),
		0, XLENGTH(keys), tol, tol_ref, nomatch, interp, sorted, stride);
}

template<typename Tkey, typename Tval>
SEXP do_approx_search(SEXP x, SEXP keys, SEXP values, double tol, int tol_ref,
	Tval nomatch, int interp = EST_NEAR, bool sorted = TRUE)
{
	SEXP result;
	PROTECT(result = Rf_allocVector(TYPEOF(values), XLENGTH(x)));
	Tval * pResult = DataPtr<Tval>(result);
	Tkey * pX = DataPtr<Tkey>(x);
	do_approx_search<Tkey, Tval>(pResult, pX, XLENGTH(x),
		keys, values, tol, tol_ref, nomatch, interp, sorted);
	UNPROTECT(1);
	return result;
}

#endif // MATTER_SEARCH

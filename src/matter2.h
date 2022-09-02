#ifndef MATTER2
#define MATTER2

#include "matterDefines.h"
#include "atoms.h"

//// Matter class
//----------------

class Matter2 {

	public:

		Matter2(SEXP x) : _data(R_do_slot(x, Rf_install("data")))
		{
			_type = R_do_slot(x, Rf_install("type"));
			_dim = R_do_slot(x, Rf_install("dim"));
			_names = R_do_slot(x, Rf_install("names"));
			_dimnames = R_do_slot(x, Rf_install("dimnames"));
			_self = x;
		}

		~Matter2() {
			_data.self_destruct();
		}

		SEXP self() {
			return _self;
		}

		void self_destruct() {
			_data.self_destruct();
		}

		Atoms2 * data() {
			return &_data;
		}

		int type() {
			return INTEGER_ELT(_type, 0);
		}

		int type(int i) {
			if ( XLENGTH(_type) > i )
				return INTEGER_ELT(_type, i);
			else
				return NA_INTEGER;
		}

		int rank() {
			return LENGTH(_dim);
		}

		SEXP dim() {
			return _dim;
		}

		R_xlen_t dim(int i) {
			if ( i < rank() )
				return IndexElt(_dim, i);
			else
				return NA_INTEGER;
		}

		R_xlen_t length() {
			R_xlen_t size = 1;
			for ( size_t i = 0; i < rank(); i++ )
				size *= dim(i);
			return size;
		}

		SEXP names() {
			return _names;
		}

		SEXP names(int i) {
			if ( XLENGTH(_names) > i )
				return STRING_ELT(_names, i);
			else
				return NA_STRING;
		}

		SEXP dimnames() {
			return _dimnames;
		}

		SEXP dimnames(int i) {
			if ( i < rank() )
				return VECTOR_ELT(_dimnames, i);
			else
				return R_NilValue;
		}

	protected:

		SEXP _self;
		Atoms2 _data;
		SEXP _type;
		SEXP _dim;
		SEXP _names;
		SEXP _dimnames;

};

class Matter2Array : public Matter2 {

	public:

		Matter2Array(SEXP x) : Matter2(x)
		{
			_ops = R_do_slot(x, Rf_install("ops"));
			_lastMaj = Rf_asLogical(R_do_slot(x, Rf_install("lastMaj")));
		}

		bool last_dim_major() {
			return _lastMaj;
		}

		template<typename T>
		size_t getRegion(index_t i, size_t size, T * buffer, int stride = 1)
		{
			return data()->flatten()->get_region<T>(buffer, i, size, 0, stride);
		}

		template<typename T>
		size_t setRegion(index_t i, size_t size, T * buffer, int stride = 1)
		{
			return data()->flatten()->set_region<T>(buffer, i, size, 0, stride);
		}

		SEXP getRegion(index_t i, size_t size)
		{
			SEXP x;
			switch(type()) {
				case R_RAW:
					PROTECT(x = Rf_allocVector(RAWSXP, size));
					getRegion(i, size, RAW(x));
					break;
				case R_LOGICAL:
					PROTECT(x = Rf_allocVector(LGLSXP, size));
					getRegion(i, size, LOGICAL(x));
					break;
				case R_INTEGER:
					PROTECT(x = Rf_allocVector(INTSXP, size));
					getRegion(i, size, INTEGER(x));
					break;
				case R_DOUBLE:
					PROTECT(x = Rf_allocVector(REALSXP, size));
					getRegion(i, size, REAL(x));
					break;
				default:
					self_destruct();
					Rf_error("invalid matter array type");
			}
			UNPROTECT(1);
			return x;
		}

		SEXP setRegion(index_t i, SEXP value)
		{
			switch(type()) {
				case R_RAW:
					setRegion(i, XLENGTH(value), RAW(value));
					break;
				case R_LOGICAL:
					setRegion(i, XLENGTH(value), LOGICAL(value));
					break;
				case R_INTEGER:
					setRegion(i, XLENGTH(value), INTEGER(value));
					break;
				case R_DOUBLE:
					setRegion(i, XLENGTH(value), REAL(value));
					break;
				default:
					self_destruct();
					Rf_error("invalid matter array type");
			}
			return self();
		}

		template<typename T>
		size_t getElements(SEXP indx, T * buffer, int stride = 1)
		{
			return data()->flatten()->get_elements<T>(buffer, indx, 0, stride);
		}

		template<typename T>
		size_t setElements(SEXP indx, T * buffer, int stride = 1)
		{
			return data()->flatten()->set_elements<T>(buffer, indx, 0, stride);
		}

		SEXP getElements(SEXP indx)
		{
			SEXP x;
			if ( Rf_isNull(indx) )
				return getRegion(0, length());
			switch(type()) {
				case R_RAW:
					PROTECT(x = Rf_allocVector(RAWSXP, XLENGTH(indx)));
					getElements(indx, RAW(x));
					break;
				case R_LOGICAL:
					PROTECT(x = Rf_allocVector(LGLSXP, XLENGTH(indx)));
					getElements(indx, LOGICAL(x));
					break;
				case R_INTEGER:
					PROTECT(x = Rf_allocVector(INTSXP, XLENGTH(indx)));
					getElements(indx, INTEGER(x));
					break;
				case R_DOUBLE:
					PROTECT(x = Rf_allocVector(REALSXP, XLENGTH(indx)));
					getElements(indx, REAL(x));
					break;
				default:
					self_destruct();
					Rf_error("invalid matter array type");
			}
			UNPROTECT(1);
			return x;
		}

		SEXP setElements(SEXP indx, SEXP value)
		{
			if ( Rf_isNull(indx) )
				return setRegion(0, value);
			switch(type()) {
				case R_RAW:
					setElements(indx, RAW(value));
					break;
				case R_LOGICAL:
					setElements(indx, LOGICAL(value));
					break;
				case R_INTEGER:
					setElements(indx, INTEGER(value));
					break;
				case R_DOUBLE:
					setElements(indx, REAL(value));
					break;
				default:
					self_destruct();
					Rf_error("invalid matter array type");
			}
			return self();
		}

	protected:

		SEXP _ops;
		bool _lastMaj;

};

#endif // MATTER2
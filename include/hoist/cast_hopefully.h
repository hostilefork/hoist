//
//  cast_hopefully.h - Variant of "safe" casting systems like boost's
//      numeric_cast, designed to work with hoist::codeplace.  It
//      lets you check things like whether casting a negative
//      integer into an unsigned is throwing away the sign.
//      Rather than using exceptions, it is built on the error
//      reporting mechanism of hoist::hopefully and identifies
//      the location where the failed cast occurred.
//
// SEE ALSO:
//      boost_numericconversion/improved_numeric_cast
//
// NOTE:
//      This was derived from code graciously provided by
//      user "litb" on the FreeNode #c++ channel, under a "do what
//      you want with it" license.  To the extent that contract is
//      contentious with the boost license, litb's wishes override for
//      this particular file.  His original code is also on StackOverflow
//
//	http://stackoverflow.com/questions/998571/
//
//       Copyright (c) 2009 Johannes Schaub (litb)
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)
//
// See http://hostilefork.com/hoist/ for documentation.
//

#ifndef HOIST_HOPEFULLY_CAST_H
#define HOIST_HOPEFULLY_CAST_H

#include "codeplace.h"
#include "hopefully.h"

#include <climits> // SCHAR_MIN, MAX, etc, should convert to use numeric_limits

namespace hoist {

/* ranks */
template<typename> struct int_rank;
#define RANK(T, I) template<> struct int_rank<T> \
	{ static int const value = I; }

RANK(char, 1); RANK(unsigned char, 1); RANK(signed char, 1);
RANK(short, 2); RANK(unsigned short, 2);
RANK(int, 3); RANK(unsigned int, 3);
RANK(long, 4); RANK(unsigned long, 4);
#undef RANK

/* usual arith. conversions for ints (pre-condition: A, B differ) */
template<int> struct uac_at;
template<> struct uac_at<1> { typedef int type; };
template<> struct uac_at<2> { typedef unsigned int type; };
template<> struct uac_at<3> { typedef long type; };
template<> struct uac_at<4> { typedef unsigned long type; };

template<typename A, typename B>
struct uac_type {
	static char (&f(int))[1];
	static char (&f(unsigned int))[2];
	static char (&f(long))[3];
	static char (&f(unsigned long))[4];
	typedef typename uac_at<sizeof f(0 ? A() : B())>::type type;
};

/* signed games */
template<typename> struct is_signed { static bool const value = false; };
#define SG(X, TT) template<> struct is_signed<X> { \
	static bool const value = true;                \
	static X const v_min = TT##_MIN;               \
	static X const v_max = TT##_MAX;               \
}

SG(signed char, SCHAR);
SG(short, SHRT);
SG(int, INT);
SG(long, LONG);
#undef SG

template<> struct is_signed<char> {
	static bool const value = (CHAR_MIN < 0);
	static char const v_min = CHAR_MIN; // just in case it's signed...
	static char const v_max = CHAR_MAX;
};

template<typename To, typename From,
		 bool to_signed = is_signed<To>::value,
		 bool from_signed = is_signed<From>::value,
		 bool rank_fine = (int_rank<To>::value >= int_rank<From>::value)>
struct do_conv;

/* these conversions never overflow, like int -> int,
 * or  int -> long. */
template<typename To, typename From, bool Sign>
struct do_conv<To, From, Sign, Sign, true> {
	static To call(From f) {
		return static_cast<To>(f);
	}
};

template<typename To, typename From>
struct do_conv<To, From, false, false, false> {
	static To call(From f) {
		hopefully(f <= static_cast<To>(-1), HERE);
		return static_cast<To>(f);
	}
};

template<typename To, typename From>
struct do_conv<To, From, false, true, true> {
	typedef typename uac_type<To, From>::type type;
	static To call(From f, codeplace const & cp) {
		/* no need to check whether To's positive range will
		 * store From's positive range: Because the rank is
		 * fine, and To is unsigned.
		 * Fixes GCC warning "comparison is always true" */
	hopefully(f >= 0, "Trying to convert a negative value to an unsigned number", cp);
		return static_cast<To>(f);
	}
};

template<typename To, typename From>
struct do_conv<To, From, false, true, false> {
	typedef typename uac_type<To, From>::type type;
	static To call(From f, codeplace const & cp) {
	hopefully((f >= 0) and (static_cast<type>(f) <= static_cast<type>(f)-1),
		"Trying to convert a negative value to an unsigned number", cp);
	return static_cast<To>(f);
	}
};

template<typename To, typename From, bool Rank>
struct do_conv<To, From, true, false, Rank> {
	typedef typename uac_type<To, From>::type type;
	static To call(From f, codeplace const & cp) {
		hopefully(static_cast<type>(f) <= static_cast<type>(is_signed<To>::v_max),
		"Trying to convert an unsigned value to signed value that exceeds the maximum positive value of signed values", cp);
		return static_cast<To>(f);
	}
};

template<typename To, typename From>
struct do_conv<To, From, true, true, false> {
	static To call(From f, codeplace const & cp) {
	hopefully((f >= is_signed<To>::v_min) and (f <= is_signed<To>::v_max),
		"Trying to convert an unsigned value to signed value that exceeds the maximum positive value of signed values", cp);
		return static_cast<To>(f);
	}
};

template<typename To, typename From>
To cast_hopefully(From f, codeplace const & cp) { return do_conv<To, From>::call(f, cp); }

// See: http://www.gotw.ca/publications/mill17.htm
template<typename ToPtr, typename From>
ToPtr cast_hopefully(From* fp, codeplace const & cp)
{
	if (not fp)
		return nullptr;
	ToPtr tp (dynamic_cast<ToPtr>(fp));
	hopefully(tp, "cast_hopefully failed to dynamically cast between class types", cp);
	return tp;
}

template<typename ToPtr, typename From>
ToPtr cast_hopefully(const From* fp, codeplace const & cp)
{
	if (not fp)
		return nullptr;
	ToPtr tp (dynamic_cast<ToPtr>(fp));
	hopefully(tp, "cast_hopefully failed to dynamically cast between class types", cp);
	return tp;
}

#if 0
int main() {
/* unsigned -> signed, overflow */
cast_hopefully<short>(UINT_MAX);

/* unsigned -> unsigned, overflow */
cast_hopefully<unsigned char>(ULONG_MAX);

/* signed -> unsigned, overflow */
cast_hopefully<unsigned long>(-1);

/* signed -> signed, overflow */
cast_hopefully<signed char>(INT_MAX);

/* always works (no check done) */
cast_hopefully<long>(INT_MAX);
}
#endif

// we moc this file, though whether there are any QObjects or not may vary
// this dummy object suppresses the warning "No relevant classes found" w/moc
class HOPEFULLY_CAST_no_moc_warning : public QObject { Q_OBJECT };

} // end namespace hoist

#endif

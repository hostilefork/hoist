//
//  hopefully.h - Definitions of assert-like helpers which are used
//  in coordination with codeplace to coordinate with a triage
//  database regarding problems at specific locations in the
//  code.  The fact that these locations are identified by a
//  QUuid that is invariant in the source means that code
//  reorganizations will not disrupt the tie between a particular
//  "hope" and its large numeric identity.
//
//          Copyright (c) 2009-2014 HostileFork.com
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)
//
// See http://hostilefork.com/hoist/ for documentation.
//

#ifndef HOIST_HOPEFULLY_H
#define HOIST_HOPEFULLY_H

#include "codeplace.h"

namespace hoist {

bool hopefullyNotReached (QString const & message, codeplace const & cp);


inline bool hopefullyNotReached (char const * message, codeplace const & cp) {
    return hopefullyNotReached(QString (message), cp);
}


inline bool hopefullyNotReached (codeplace const & cp) {
    return hopefullyNotReached("Program Integrity Protection Triggered", cp);
}


inline bool hopefully (
    bool const condition,
    QString const & message,
    codeplace const & cp
) {
    if (not condition)
        hopefullyNotReached(message, cp);
    return condition;
}


inline bool hopefully (
    bool const condition,
    char const * message,
    codeplace const & cp
) {
    if (not condition)
        hopefullyNotReached(QString (message), cp);
    return condition;
}


inline bool hopefully (bool const condition, codeplace const & cp) {
    if (not condition)
        hopefullyNotReached(cp);
    return condition;
}


// hopefullyAlter and hopefullyTransition were inspired by tracked<T>, but
// were useful for general assignments also.

template <typename T, typename U>
inline bool hopefullyAlter (
    T & variable,
    U const & value,
    codeplace const & cp
) {
    bool result = hopefully(variable != value, cp);
    variable = value;
    return result;
}

template <typename T, typename U, typename V>
inline bool hopefullyTransition (
    T & variable,
    U const & oldValue,
    V const & newValue,
    codeplace const & cp
) {
    bool result = hopefully(variable == oldValue, cp);
    variable = newValue;
    return result;
}


// default hope failed handler is not very interesting, you can make your own

typedef void (* hope_failed_handler) (
QString const & message,
    codeplace const & cp
); 


hope_failed_handler setHopeFailedHandlerAndReturnOldHandler (
    hope_failed_handler const & newHandler
);


inline void setHopeFailedHandler (hope_failed_handler const & newHandler) {
    static_cast<void>(setHopeFailedHandlerAndReturnOldHandler(newHandler));
}

} // end namespace hoist

#endif

//
//  chronicle.h - Prototype for a function that gives debug output, with
//  the source location of the line that generated the output generated.
//  It also knows the source location of the flag which controls the
//  choice to produce that output, along with the source location where
//  it was last set.
//
//          Copyright (c) 2009-2014 HostileFork.com
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)
//
// See http://hostilefork.com/hoist/ for documentation.
//

#ifndef HOIST_CHRONICLE_H
#define HOIST_CHRONICLE_H

#include <functional>

#include "codeplace.h"
#include "tracked.h"

namespace hoist {

typedef std::function<void(QDebug)> chronicle_function;

bool chronicle (
    tracked<bool> const & enabled,
    QString const & message,
    codeplace const & cp
);

bool chronicle (
    tracked<bool> const & enabled,
    chronicle_function function,
    codeplace const & cp
);

} // end namespace hoist

#endif

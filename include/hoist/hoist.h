//
// hoist.h - A single include which does a #include for all the hoist
//  library classes, provided as a convenience.  You may also
//  include them individually, though some depend on others.
//
//          Copyright (c) 2009-2014 HostileFork.com
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)
//
// See http://hostilefork.com/hoist/ for documentation.
//

#ifndef HOIST_HOIST_H
#define HOIST_HOIST_H

#include "codeplace.h"
#include "hopefully.h"
#include "tracked.h"
#include "stacked.h"
#include "listed.h"
#include "mapped.h"
#include "cast_hopefully.h"
#include "chronicle.h"

// we moc this file, though whether there are any QObjects or not may vary
// this dummy object suppresses the warning "No relevant classes found" w/moc
class HOIST_no_moc_warning : public QObject { Q_OBJECT };

#endif

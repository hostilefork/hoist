//
//  chronicle.h - Prototype for a function that gives debug output, with
//	the source location of the line that generated the output generated.
//	It also knows the source location of the flag which controls the
//	choice to produce that output, along with the source location where
//	it was last set.
//
//              Copyright (c) 2009 HostileFork.com
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)
//
// See http://hostilefork.com/hoist/ for documentation.
//

#ifndef HOIST__CHRONICLE_H
#define HOIST__CHRONICLE_H

#include "codeplace.h"
#include "tracked.h"

namespace hoist {

// Currently this is quite similar to the callback, but I remove the boolean (since we know it must be true)
// There may be additional processing to be done in the future.

bool chronicle(const tracked< bool >& enabled, const QString& message, const codeplace& cp);

// The default chronicle handler is not very interesting, you can make your own

typedef void (*chronicle_handler)(const codeplace& cpEnableWhereConstructed, const codeplace& cpEnableWhereLastAssigned, const QString& message, const codeplace& cpOutput);

chronicle_handler setChronicleHandlerAndReturnOldHandler(const chronicle_handler& newHandler);

inline void setChronicleHandler(const chronicle_handler& newHandler)
{
	static_cast< void >(setChronicleHandlerAndReturnOldHandler(newHandler));
}

// we moc this file, though whether there are any QObjects or not may vary
// this dummy object suppresses the warning "No relevant classes found" w/moc
class CHRONICLE_no_moc_warning : public QObject { Q_OBJECT };

} // end namespace hoist

#endif
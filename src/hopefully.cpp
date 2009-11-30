//
//  hopefully.cpp - Provides a simple default implementation of the
//	HopeFailed handler which will be used by default with hoist.
//
//              Copyright (c) 2009 HostileFork.com
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)
//
// See http://hostilefork.com/hoist/ for documentation.
//

#include "hoist/hopefully.h"

#include<QDebug>

namespace hoist {

hope_failed_handler globalHopeFailedHandler (NULL);

// I'd like to include a dialog-based implementation that communicates with a server
// or Trac and passes it the QUuid, but my current implementation is too tied with
// the codebase hoist was taken out of.  It's on the agenda!
void onHopeFailedBasic(const QString& message, const codeplace& cp)
{
	qDebug() << message << endl
		<< "     output from: " << cp.toString() << endl;
	while (true) {
		// infinite loop for now to stop the code
		// but if you have more threads that won't stop them
		// need a better idea here.
	}
}

hope_failed_handler setHopeFailedHandlerAndReturnOldHandler(const hope_failed_handler& newHandler)
{
	hope_failed_handler result (globalHopeFailedHandler ? globalHopeFailedHandler : &onHopeFailedBasic);
	if (hopefully(newHandler, "Null pointer passed to setHopeFailedHandler", HERE)) {
		if (newHandler == &onHopeFailedBasic) {
			globalHopeFailedHandler = NULL;
		} else {
			globalHopeFailedHandler = newHandler;
		}
	}
	return result;
}

// Not sure if this should be called "hopeFailed" or "hopefullyNotReached".  I like
// hopefullyNotReached because it expresses the failure as saying "I hope this
// line of code doesn't run", which keeps the emphasis on what you hope is
// true rather than what is not true...
bool hopefullyNotReached(const QString& message, const codeplace& cp)
{
	if (globalHopeFailedHandler) {
		(*globalHopeFailedHandler)(message, cp);
	} else {
		onHopeFailedBasic(message, cp);
	}

	// always returns false, for consistency with other hopefully(*) boolean-returners
	return false;
}

} // end namespace hoist
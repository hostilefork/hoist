//
//  hopefully.h - Definitions of assert-like helpers which are used
// 	in coordination with codeplace to coordinate with a triage
//	database regarding problems at specific locations in the
//	code.  The fact that these locations are identified by a
//	QUuid that is invariant in the source means that code
//	reorganizations will not disrupt the tie between a particular
//	"hope" and its large numeric identity.
//
//              Copyright (c) 2009 HostileFork.com
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)
//
// See http://hostilefork.com/hoist/ for documentation.
//

#ifndef HOIST__HOPEFULLY_H
#define HOIST__HOPEFULLY_H

#include "codeplace.h"

namespace hoist {

void hopefullyNotReached(const QString& message, const codeplace& cp);

inline void hopefullyNotReached(const char* message, const codeplace& cp)
{
	hopefullyNotReached(QString (message), cp);
}

inline void hopefullyNotReached(const codeplace& cp)
{
	hopefullyNotReached("Program Integrity Protection Triggered", cp);
}

inline bool hopefully(const bool condition, const QString& message, const codeplace& cp)
{
	if (not condition)
		hopefullyNotReached(message, cp);
	return condition;
}

inline bool hopefully(const bool condition, const char* message, const codeplace& cp)
{
	if (not condition)
		hopefullyNotReached(QString (message), cp);
	return condition;
}

inline bool hopefully(const bool condition, const codeplace& cp)
{
	if (not condition)
		hopefullyNotReached(cp);
	return condition;
}

// The default hope failed handler is not very interesting, you can make your own

typedef void (*hope_failed_handler)(const QString& message, const codeplace& cp);

hope_failed_handler setHopeFailedHandlerAndReturnOldHandler(const hope_failed_handler& newHandler);

inline void setHopeFailedHandler(const hope_failed_handler& newHandler)
{
	static_cast< void >(setHopeFailedHandlerAndReturnOldHandler(newHandler));
}

// we moc this file.  but currently no Qt objects.  doesn't mean there won't ever be
// so this dummy object suppresses the warning "No relevant classes found" during moc
class HOPEFULLY_no_moc_warning : public QObject { Q_OBJECT };

} // end namespace hoist

#endif
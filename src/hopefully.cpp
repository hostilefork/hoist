//
//  hopefully.cpp - Provides a simple default implementation of the
//  HopeFailed handler which will be used by default with hoist.
//
//          Copyright (c) 2009-2014 HostileFork.com
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)
//
// See http://hostilefork.com/hoist/ for documentation.
//

#include "hoist/hopefully.h"

#include <QDebug>

namespace hoist {

hope_failed_handler globalHopeFailedHandler = nullptr;


// I'd like to include a dialog-based implementation that communicates with a
// server or tracker, but my current implementation is too tied with the
// codebase hoist was taken out of.  It's on the agenda!

void onHopeFailedBasic(QString const & message, codeplace const & cp)
{
    qDebug() << message << endl
        << "     output from: " << cp.toString() << endl;

    qt_assert_x(
        message.toLatin1(),
        cp.getUuid().toString().toLatin1(),
        cp.getFilename().toLatin1(),
        cp.getLine()
    );

    // hoist encourages "ship what you test" and the hopefully functions do
    // not disappear in the release build.  Yet they return a value which can
    // be tested and error handling (if any) run.

    // the nuances of when you are allowed to return false instead of halting
    // execution here are something I need to write up better in the hoist
    // docs, but this simple substitute header file doesn't have the tools to
    // make that discernment (e.g. checking a web database or getting a
    // passcode from a developer).

    // So if you're in the debugger and want to continue, then skip this
    // next call using set-next-statement

    qFatal(
        "%s in %s of %s, line %ld",
        message.toLocal8Bit().data(),
        cp.getUuid().toString().toLocal8Bit().data(),
        cp.getFilename().toLocal8Bit().data(),
        cp.getLine()
    );
}


hope_failed_handler setHopeFailedHandlerAndReturnOldHandler(
    hope_failed_handler const & newHandler
) {
    hope_failed_handler result = 
        globalHopeFailedHandler
        ? globalHopeFailedHandler
        : &onHopeFailedBasic;

    if (hopefully(newHandler, "Null passed to setHopeFailedHandler", HERE)) {
        if (newHandler == &onHopeFailedBasic) {
            globalHopeFailedHandler = nullptr;
        } else {
            globalHopeFailedHandler = newHandler;
        }
    }
    return result;
}


// Not sure if this should be called "hopeFailed" or "hopefullyNotReached".
// I like hopefullyNotReached because it expresses the failure as saying
// "I hope this line of code doesn't run", which keeps the emphasis on what
// you hope is true rather than what is not true...

bool hopefullyNotReached (
    QString const & message,
    codeplace const & cp
) {
    if (globalHopeFailedHandler) {
        (*globalHopeFailedHandler)(message, cp);
    } else {
        onHopeFailedBasic(message, cp);
    }

    // return false, for consistency with other hopefully(*) boolean-returners
    return false;
}

} // end namespace hoist

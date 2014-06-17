//
//  chronicle.cpp - Provides a simple default implementation of the
//  chronicle handler which will be used by default with hoist.
//
//          Copyright (c) 2009-2014 HostileFork.com
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)
//
// See http://hostilefork.com/hoist/ for documentation.
//

#include "hoist/chronicle.h"

#include <QDebug>

namespace hoist {

chronicle_handler globalChronicleHandler = nullptr;


void onChronicleBasic(
    codeplace const & cpEnableWhereConstructed,
    codeplace const & cpEnableWhereLastAssigned,
    QString const & message,
    codeplace const & cpOutput
) {
    Q_UNUSED(cpEnableWhereConstructed);

    qDebug() << message << endl
        << "     output from: " << cpOutput.toString() << endl
        << "     enabled by: " << cpEnableWhereLastAssigned.toString() << endl;
}


chronicle_handler setChronicleHandlerAndReturnOldHandler (
    chronicle_handler const & newHandler)
{
    chronicle_handler result
        = globalChronicleHandler
        ? globalChronicleHandler
        : &onChronicleBasic;

    if (hopefully(newHandler, "Null passed to setChronicleHandler", HERE)) {
        if (newHandler == &onChronicleBasic) {
            globalChronicleHandler = nullptr;
        } else {
            globalChronicleHandler = newHandler;
        }
    }
    return result;
}


bool chronicle(
    tracked<bool> const & enabled,
    QString const & message,
    codeplace const & cp
) {
    if (enabled) {
        if (globalChronicleHandler) {
            (*globalChronicleHandler)(
                enabled.whereConstructed(),
                enabled.whereLastAssigned(),
                message,
                cp
            );
        } else {
            onChronicleBasic(
                enabled.whereConstructed(),
                enabled.whereLastAssigned(),
                message,
                cp
            );
        }
    }
    return enabled;
}

} // end namespace hoist

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


QDebug chronicleCore (
    codeplace const & whereEnableConstructed,
    codeplace const & whereEnableLastAssigned,
    codeplace const & cpOutput
) {
    Q_UNUSED(whereEnableConstructed);

    return qDebug()
        << "debug output from:" << cpOutput.toString() << endl
        << "output enabled by:" << whereEnableLastAssigned.toString() << endl;
}


bool chronicle (
    tracked<bool> const & enabled,
    QString const & message,
    codeplace const & cp
) {
    if (enabled) {
        chronicleCore(
            enabled.whereConstructed(),
            enabled.whereLastAssigned(),
            cp
        ) << message << endl;
    }
    return enabled;
}


bool chronicle (
    tracked<bool> const & enabled,
    chronicle_function function,
    codeplace const & cp
) {
    if (enabled) {
        function(chronicleCore(
            enabled.whereConstructed(),
            enabled.whereLastAssigned(),
            cp
        ));
    }
    return enabled;
}


} // end namespace hoist

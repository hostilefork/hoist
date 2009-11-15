//
//  chronicle.cpp - Provides a simple default implementation of the
//	chronicle handler which will be used by default with hoist.
//
//              Copyright (c) 2009 HostileFork.com
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)
//
// See http://hostilefork.com/hoist/ for documentation.
//

#include "hoist/chronicle.h"

#include<QDebug>

namespace hoist {

chronicle_handler globalChronicleHandler (NULL);

void onChronicleBasic(const codeplace& /*cpEnableWhereConstructed*/, const codeplace& cpEnableWhereLastAssigned, const QString& message, const codeplace& cpOutput)
{
	qDebug() << message << endl
		<< "     output from: " << cpOutput.toString() << endl
		<< "     enabled by: " << cpEnableWhereLastAssigned.toString() << endl;
}

chronicle_handler setChronicleHandlerAndReturnOldHandler(const chronicle_handler& newHandler)
{
	chronicle_handler result (globalChronicleHandler ? globalChronicleHandler : &onChronicleBasic);
	if (hopefully(newHandler, "Null pointer passed to setChronicleHandler", HERE)) {
		if (newHandler == &onChronicleBasic) {
			globalChronicleHandler = NULL;
		} else {
			globalChronicleHandler = newHandler;
		}
	}
	return result;
}

bool chronicle(const tracked< bool >& enabled, const QString& message, const codeplace& cp)
{
	if (enabled) {
		if (globalChronicleHandler) {
			(*globalChronicleHandler)(enabled.whereConstructed(), enabled.whereLastAssigned(), message, cp);
		} else {
			onChronicleBasic(enabled.whereConstructed(), enabled.whereLastAssigned(), message, cp);
		}
	}
	return enabled;
}

} // end namespace hoist
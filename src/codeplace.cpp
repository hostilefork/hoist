//
//  codeplace.cpp - Static definitions required by the codeplace.h
//	implementation.
//
//              Copyright (c) 2009 HostileFork.com
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)
//
// See http://hostilefork.com/hoist/ for documentation.
//

#include "hoist/codeplace.h"

#include <QTextStream>
#include <QDataStream>

namespace hoist {

QByteArray Base64StringFromUuid(const QUuid& uuid)
{
	QByteArray buf;
	QDataStream ds (&buf, QIODevice::WriteOnly);
	ds << uuid;
	QByteArray result = buf.toBase64();
	assert(UuidFromBase64String(result) == uuid);
	return result;
}

QUuid UuidFromBase64String(const QByteArray& str)
{
	QByteArray buf = QByteArray::fromBase64(str);
	QDataStream ds (&buf, QIODevice::ReadOnly);
	QUuid uuid;
	ds >> uuid;
	return uuid;
}

QUuid UuidFrom128Bits(const QByteArray& bytes)
{
	assert(bytes.length() == 128/8);
	// don't know why datastream requires a non-const byte array pointer in read only mode :(
	QDataStream ds (const_cast<QByteArray*>(&bytes), QIODevice::ReadOnly);
	QUuid uuid;
	ds >> uuid;
	return uuid;
}

} // end namespace hoist
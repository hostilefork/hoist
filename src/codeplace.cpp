//
//  codeplace.cpp - Static definitions required by the codeplace.h
//  implementation.
//
//          Copyright (c) 2009-2014 HostileFork.com
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

QByteArray Base64StringFromUuid(QUuid const & uuid) {
    QByteArray buf;
    QDataStream ds (&buf, QIODevice::WriteOnly);
    ds << uuid;
    QByteArray result = buf.toBase64();
    assert(UuidFromBase64String(result) == uuid);
    return result;
}


QUuid UuidFromBase64String(QByteArray const & str) {
    QByteArray buf = QByteArray::fromBase64(str);
    QDataStream ds (&buf, QIODevice::ReadOnly);
    QUuid uuid;
    ds >> uuid;
    return uuid;
}


QUuid UuidFrom128Bits(QByteArray const & bytes) {
    assert(bytes.length() == 128/8);
    // why does datastream need a mutable byte array pointer in R/O mode? :(
    QDataStream ds (const_cast<QByteArray *>(&bytes), QIODevice::ReadOnly);
    QUuid uuid;
    ds >> uuid;
    return uuid;
}



///
/// codeplace
///

// I do not like this very much but default constructible is needed for
// several purposes, including qRegisterMetaType.
codeplace::codeplace () :
    _options (Options::None),
    _filenameEither (nullptr),
    _line (-1),
    _uuidEither (nullptr)
{
}


// copy constructor is public
codeplace::codeplace (codeplace const & other) :
    _options (Options::None),
    _filenameEither (nullptr),
    _line (other._line),
    _uuidEither (nullptr)
{
    transitionFromNull(other);
}


codeplace::~codeplace () {
    // Optimization; for some reason transitionToNull is (relatively) slow
    // even if it is a no-op
    if (_options >= Options::FilenameIsQString)
        transitionToNull();
}


codeplace & codeplace::operator= (codeplace const & rhs) {
    // protect against self-assignment
    if (&rhs != this) {
        // free any strings we may have responsibility to free
        transitionToNull();

        // copy and potentially allocate new QStrings
        transitionFromNull(rhs);
    }
    return *this;
}


int codeplace::operator== (codeplace const & rhs) const {
    // Should we allow comparisons of null codeplaces?

    // TODO: current semantics is uuid equality, is that sensible?
    // also what to do about null uuids?
    return this->getUuid() == rhs.getUuid();
}


QString codeplace::getFilename () const {
    assert(_options != Options::None);

    // TODO: behind the scenes mutation into using a QString?
    // would need to be thread safe if so
    if ((_options & Options::FilenameIsQString) != Options::None) {
        return *_filenameQString;
    }
    return QString(_filenameCString);
}


long codeplace::getLine () const {
    assert(_options != Options::None);
    return _line;
}


QUuid codeplace::getUuid () const {
    assert(_options != Options::None);

    if ((_options & Options::Permanent) != Options::None) {
        if ((_options & Options::UuidIsQString) != Options::None) {
            return UuidFromBase64String((*_uuidQString).toLatin1());
        }
        return UuidFromBase64String(QByteArray(_uuidCString));
    } else if ((_options & Options::Hashed) != Options::None) {
        // TODO: Decide what string format will hash best
        QString fileAndLine;
        if ((_options & Options::FilenameIsQString) != Options::None)
            fileAndLine = QString::number(_line, 10) + *_filenameQString;
        else
            fileAndLine = QString::number(_line, 10) + _filenameCString;

        QByteArray bytes = QCryptographicHash::hash(
            fileAndLine.toUtf8(), QCryptographicHash::Md4
        );

        return UuidFrom128Bits(bytes);
    } else {
        // null uuid for null codeplace... should be caught above
        return QUuid ();
    }
}


codeplace::operator QUuid () const {
    return getUuid();
}


QString codeplace::toString () const {
    QString result;
    QTextStream ts (&result);
    ts << "File: '" << getFilename() << "' - " << " Line # " << getLine();
    return result;
}


bool codeplace::isPermanent () const {
    return (_options & Options::Permanent) != Options::None;
}


void codeplace::transitionToNull () {
    if ((_options & Options::FilenameIsQString) != Options::None) {
        delete _filenameQString;
        _filenameQString = nullptr;
    }

    if (
        (_options & (Options::Permanent | Options::UuidIsQString))
        != Options::None
    ) {
        delete _uuidQString;
        _uuidQString = nullptr;
    }
}

void codeplace::transitionFromNull (codeplace const & other) {
    _options = other._options;

    if (other._options == Options::None)
        return;

    if ((other._options & Options::FilenameIsQString) != Options::None) {
        _filenameQString = new QString (*other._filenameQString);
    } else {
        _filenameCString = other._filenameCString;
    }

    if ((other._options & Options::Permanent) != Options::None) {
        if ((other._options & Options::UuidIsQString) != Options::None) {
            _uuidQString = new QString (*other._uuidQString);
        } else {
            _uuidCString = other._uuidCString;
        }
    }

    _line = other._line;
}


codeplace::codeplace (
    QString const & filename,
    long const & line,
    QString const & uuidString
) :
    _options (
        Options::Permanent
        | Options::FilenameIsQString
        | Options::UuidIsQString
    ),
    _filenameQString (new QString (filename)),
    _line (line),
    _uuidQString (new QString (uuidString))
{
}


codeplace::codeplace (
    char const * filename,
    long const & line,
    char const * uuidString
) :
    _options (Options::Permanent),
    _filenameCString (filename),
    _line (line),
    _uuidCString (uuidString)
{
}


codeplace::codeplace (QString const & filename, long const & line) :
    _options (Options::Hashed | Options::FilenameIsQString),
    _filenameQString (new QString (filename)),
    _line (line),
    _uuidEither (nullptr)
{
}


codeplace::codeplace (char const * filename, long const & line) :
    _options (Options::Hashed),
    _filenameCString (filename),
    _line (line),
    _uuidEither (nullptr)
{
}


///
/// Static Methods
///

codeplace codeplace::makeHere (char const * filename, long const & line) {
    return codeplace (filename, line);
}


codeplace codeplace::makeHere (QString const & filename, long const & line) {
    return codeplace (filename, line);
}


codeplace codeplace::makePlace (
    char const * filename,
    long const & line,
    char const * uuidString
) {
    return codeplace (filename, line, uuidString);
}


codeplace codeplace::makePlace (
    QString const & filename,
    long const & line,
    QString const & uuidString
) {
    return codeplace (filename, line, uuidString);
}


codeplace codeplace::makeThere (
    QString const & filename,
    long const & line,
    codeplace const & cp
) {
    QString uuidString = Base64StringFromUuid(cp.getUuid());
    return codeplace (filename, line, uuidString);
}


codeplace codeplace::makeYonder (
    QString const & yonderString,
    codeplace const & cp
) {
    // faster than Md5 and we're not worried about security/attacks
    // NOTE: Qt hash function is thread-safe
    QByteArray bytes = QCryptographicHash::hash(
        yonderString.toUtf8(), QCryptographicHash::Md4
    );

    // TODO: Fix this to comply with UUID format rules, random or hashed
    // bits will contain lies in the bits indicating what protocol the
    // UUID was generated in accordance with (e.g. a pseudo-UUID)
    QString uuidString = Base64StringFromUuid(UuidFrom128Bits(bytes));

    return codeplace (cp.getFilename(), cp.getLine(), uuidString);
}


} // end namespace hoist

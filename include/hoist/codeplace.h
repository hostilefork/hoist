//
//  codeplace.h - Class that implements an object that
//     encapsulates the notion of an invariant source location.
//     This object can report the line and file number where it
//     was declared, as well as a QUuid which may be either
//     literally provided in the source or temporarily generated
//     through hashes.
//
//          Copyright (c) 2009-2014 HostileFork.com
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)
//
// See http://hostilefork.com/hoist/ for documentation.
//

#ifndef HOIST_CODEPLACE_H
#define HOIST_CODEPLACE_H

// for what should be obvious reasons, codeplace cannot use "hopefully"
// as its assertion service (hopefully is implemented in terms of codeplace!)
#include <cassert>

#include <QUuid>
#include <QByteArray>
#include <QTextStream>
#include <QCryptographicHash>
#include <QCoreApplication>

namespace hoist {

QByteArray Base64StringFromUuid (QUuid const & uuid);

QUuid UuidFromBase64String (QByteArray const & str);

QUuid UuidFrom128Bits (QByteArray const & bytes);


// TODO: filenames in a string table to avoid redundant QString allocations?
// TODO: figure out compilation on files with unicode __FILE__ ?
class codeplace
{
public:
    // REVIEW: These functions used to live inside a statically allocated
    // "manager" object.  At one time this manager allowed one to look
    // up (at runtime) a codeplace from a UUID, and there were some
    // other caching tricks.  Over time the global nature of the manager
    // created problems with shutdown and initialization and the
    // usage pattern of global char const * instead of QString emerged
    // as dominant, so the manager has been removed for now.

    static codeplace makeHere (char const * filename, long const & line) {
        return codeplace (filename, line);
    }

    static codeplace makeHere (QString const & filename, long const & line) {
        return codeplace (filename, line);
    }

    static codeplace makePlace (
        char const * filename,
        long const & line,
        char const * uuidString
    ) {
        return codeplace (filename, line, uuidString);
    }

    static codeplace makePlace (
        QString const & filename,
        long const & line,
        QString const & uuidString
    ) {
        return codeplace (filename, line, uuidString);
    }

    static codeplace makeThere (
        QString const & filename,
        long const & line,
        codeplace const & cp
    ) {
        QString uuidString = Base64StringFromUuid(cp.getUuid());
        return codeplace (filename, line, uuidString);
    }

    static codeplace makeYonder (
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


public:
    // I do not like this very much but default constructible is needed for
    // several purposes, including qRegisterMetaType.
    codeplace () :
        _options (NullCodeplace),
        _filenameEither (nullptr),
        _line (-1),
        _uuidEither (nullptr)
    {
    }

    // copy constructor is public
    codeplace (codeplace const & other) :
        _options (NullCodeplace),
        _filenameEither (nullptr),
        _line (other._line),
        _uuidEither (nullptr)
    {
        transitionFromNull(other);
    }

    virtual ~codeplace () {
        transitionToNull();
    }


public:
    codeplace & operator= (codeplace const & rhs)
    {
        // protect against self-assignment
        if (&rhs != this) {
            // free any strings we may have responsibility to free
            transitionToNull();

            // copy and potentially allocate new QStrings
            transitionFromNull(rhs);
        }
        return *this;
    }

    int operator== (codeplace const & rhs) const
    {
        // Should we allow comparisons of null codeplaces?
        /* assert(options != NullCodeplace); */

        // TODO: current semantics is uuid equality, is that sensible?
        // also what to do about null uuids?
        return this->getUuid() == rhs.getUuid();
    }

    QString getFilename () const {
        assert(_options != NullCodeplace);

        // TODO: behind the scenes mutation into using a QString?
        // would need to be thread safe if so
        if (_options & FilenameIsQString) {
            return *_filenameQString;
        }
        return QString(_filenameCString);
    }

    long getLine () const {
        assert(_options != NullCodeplace);
        return _line;
    }

    QUuid getUuid () const {
        assert(_options != NullCodeplace);

        if (_options & Permanent) {
            if (_options & UuidIsQString) {
                return UuidFromBase64String((*_uuidQString).toLatin1());
            }
            return UuidFromBase64String(QByteArray(_uuidCString));
        } else if (_options & Hashed) {
            // TODO: Decide what string format will hash best
            QString fileAndLine;
            if (_options & FilenameIsQString)
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

    operator QUuid () const {
        return getUuid();
    }

    QString toString () const {
        QString result;
        QTextStream ts (&result);
        ts << "File: '" << getFilename() << "' - " << " Line # " << getLine();
        return result;
    }

    bool isPermanent () const {
        return (_options & Permanent);
    }


private:
    void transitionToNull () {
        if (_options & FilenameIsQString) {
            delete _filenameQString;
            _filenameQString = nullptr;
        }

        if ((_options & Permanent) and (_options & UuidIsQString)) {
            delete _uuidQString;
            _uuidQString = nullptr;
        }
    }

    void transitionFromNull (codeplace const & other) {
        _options = other._options;

        if (other._options & NullCodeplace)
            return;

        if (other._options & FilenameIsQString) {
            _filenameQString = new QString (*other._filenameQString);
        } else {
            _filenameCString = other._filenameCString;
        }

        if (other._options & Permanent) {
            if (other._options & UuidIsQString) {
                _uuidQString = new QString (*other._uuidQString);
            } else {
                _uuidCString = other._uuidCString;
            }
        }

        _line = other._line;
    }


public:
    enum Option {
         // I don't like the fact that we need default constructible
         // for qRegisterMetaType
        NullCodeplace = 0x0,

        // is either "permanent" or "hashed", can't actually be both
        Hashed = 0x1,
        Permanent = 0x2,

        // codeplace is optimized to avoid making a lot of QStrings out of
        // immutable string literals that you're already paying for in the
        // compiler's constant pool. The only time it actually uses QStrings
        // is if you pass in a UUID or filename string that needs to have its
        // own memory management.
        FilenameIsQString = 0x4,
        UuidIsQString = 0x8

        // Note that as a general rule... if you enable string pooling a.k.a.
        // string interning you can save memory, e.g. when 100 asserts in the
        // same file are all using __FILE__ and thus producing a copy of the
        // literal "foo.cpp" you only pay for the memory to hold that once.
        //
        //    http://en.wikipedia.org/wiki/String_interning
        //    http://msdn.microsoft.com/en-us/library/s0s0asdt(VS.80).aspx
    };
    Q_DECLARE_FLAGS(Options, Option)


protected:
    codeplace (
        QString const & filename,
        long const & line,
        QString const & uuidString
    ) :
        _options (Permanent | FilenameIsQString | UuidIsQString),
        _filenameQString (new QString (filename)),
        _line (line),
        _uuidQString (new QString (uuidString))
    {
    }

    codeplace (
        char const * filename,
        long const & line,
        char const * uuidString
    ) :
        _options (Permanent),
        _filenameCString (filename),
        _line (line),
        _uuidCString (uuidString)
    {
    }

    codeplace (QString const & filename, long const & line) :
        _options (Hashed | FilenameIsQString),
        _filenameQString (new QString (filename)),
        _line (line),
        _uuidEither (nullptr)
    {
    }

    codeplace (char const * filename, long const & line) :
        _options (Hashed),
        _filenameCString (filename),
        _line (line),
        _uuidEither (nullptr)
    {
    }

private:
    Options _options;
    union {
        // dynamically allocated, we are responsible for freeing
        QString* _filenameQString;

        // must be in constant pool, assumed valid forever
        char const * _filenameCString;

        // use this to copy or set null
        void* _filenameEither;
    };

    long _line; // boost::assert used long, I'm just following their lead

    union {
        // dynamically allocated, we are responsible for freeing
        QString * _uuidQString;

        // must be in constant pool, assumed valid forever
        char const * _uuidCString;

        // use this to copy or set null
        void * _uuidEither;
    };
};

Q_DECLARE_OPERATORS_FOR_FLAGS(codeplace::Options)

// we moc this file, though whether there are any QObjects or not may vary
// this dummy object suppresses the warning "No relevant classes found" w/moc
class CODEPLACE_no_moc_warning : public QObject { Q_OBJECT };

} // end namespace hoist


//
// "HERE" is a macro which returns a codeplace whose QUuid is derived
// from the hash of the filename and the line number where the macro
// is instantiated:
//
//     codeplace cp = HERE;
//
// If you use HERE more than once on the same line, then
// each call will return the same ID.  Such cases *require* supplying an
// invariant uuid as a parameter to distinguish the points (or just bumping to
// the next line).
//

#define HERE \
    hoist::codeplace::makeHere(__FILE__, __LINE__)

//
// PLACE() is what you ideally use instead of HERE wherever possible.
// You should use one of the tools included with hoist to go through your
// sources and give every codeplace a more invariant number
//
//    [Line 10] something();
//    [Line 11] codeplace cp = PLACE("cRBhRW1wQ+ZJk+22SUv4Lg");
//    [Line 12] somethingelse();
//
// With this codeplace, if you cut and paste it over into another file
// bar.cpp at different lines, it could still be identified as the codeplace
// between something() and somethingelse()... despite reporting a different
// line and file.  That is of more value over the long run than using HERE.
//

#define PLACE(uuidString) \
    hoist::codeplace::makePlace(__FILE__, __LINE__, (uuidString))

//
// "THERE" is for cases where you want to talk about a remote source line and
// file, and have a consistent codeplace for describing it.  It is for
// useful when a client library offers an assert hook with a file
// and line, and you would like to employ the logic used to make ephemeral
// codeplaces.
//

#define THERE(filename, line, cp) \
    hoist::codeplace::makeThere((filename), (line), (cp))

//
// With "YONDER", the supplied string is hashed to produce the uuid, while
// the supplied codeplace provides the file and the line to report.  You'd
// use this if you can't break out the file and line from the error message,
// e.g. when you're not given this information (such as in a Qt warning
// handler).  As long as the message string stays consistent letter-for-letter,
// you will generate the same codeplace.
//

#define YONDER(yonderString, cp) \
    hoist::codeplace::makeYonder((yonderString), (cp))

#endif

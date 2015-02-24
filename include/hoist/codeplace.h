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


class codeplace final
{
template <class> friend struct ::std::hash;

public: // switch to private and declare operators as friends...
    enum class Options {
        None = 0,

        // is either "permanent" or "hashed", can't actually be both

        Hashed = 1 << 0,
        Permanent = 1 << 1,

        // codeplace is optimized to avoid making a lot of QStrings out of
        // immutable string literals that you're already paying for in the
        // compiler's constant pool. The only time it actually uses QStrings
        // is if you pass in a UUID or filename string that needs to have its
        // own memory management.

        FilenameIsQString = 1 << 2,
        UuidIsQString = 1 << 3

        // Note that as a general rule... if you enable string pooling a.k.a.
        // string interning you can save memory, e.g. when 100 asserts in the
        // same file are all using __FILE__ and thus producing a copy of the
        // literal "foo.cpp" you only pay for the memory to hold that once.
        //
        // Enabled by default in gcc.
        //
        //    http://en.wikipedia.org/wiki/String_interning
        //    http://msdn.microsoft.com/en-us/library/s0s0asdt(VS.80).aspx
        //    http://stackoverflow.com/questions/11399682/
    };


public:
    // REVIEW: These functions used to live inside a statically allocated
    // "manager" object.  At one time this manager allowed one to look
    // up (at runtime) a codeplace from a UUID, and there were some
    // other caching tricks.  Over time the global nature of the manager
    // created problems with shutdown and initialization and the
    // usage pattern of global char const * instead of QString emerged
    // as dominant, so the manager has been removed for now.

    static codeplace makeHere (char const * filename, long const & line);

    static codeplace makeHere (QString const & filename, long const & line);

    static codeplace makePlace (
        char const * filename,
        long const & line,
        char const * uuidString
    );

    static codeplace makePlace (
        QString const & filename,
        long const & line,
        QString const & uuidString
    );

    static codeplace makeThere (
        QString const & filename,
        long const & line,
        codeplace const & cp
    );

    static codeplace makeYonder (
        QString const & yonderString,
        codeplace const & cp
    );


public:
    // I do not like this very much but default constructible is needed for
    // several purposes, including qRegisterMetaType.
    codeplace ();

    // copy constructor is also public
    codeplace (codeplace const & other);

    // destructor has to be public
    ~codeplace ();


public:
    codeplace & operator= (codeplace const & rhs);

    int operator== (codeplace const & rhs) const;

    QString getFilename () const;

    long getLine () const;

    QUuid getUuid () const;

    operator QUuid () const;

    QString toString () const;

    bool isPermanent () const;


private:
    void transitionToNull ();

    void transitionFromNull (codeplace const & other);


protected:
    codeplace (
        QString const & filename,
        long const & line,
        QString const & uuidString
    );

    codeplace (
        char const * filename,
        long const & line,
        char const * uuidString
    );

    codeplace (QString const & filename, long const & line);

    codeplace (char const * filename, long const & line);

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



// C++11 type-safe bitflag operations compatible with "enum class"
// (Qt's QFlags will not work with this feature.)
//
//    http://stackoverflow.com/a/4227264/211160
//    http://stackoverflow.com/a/8357462/211160
//
// Must test for == or != to Options::None as bool/int conversion is not
// possible for an enum class.

inline codeplace::Options operator| (
    codeplace::Options const & a,
    codeplace::Options const & b
) {
    using Options = codeplace::Options;
    typedef std::underlying_type<Options>::type ut;

    return static_cast<Options>(
        static_cast<ut>(a) | static_cast<ut>(b)
    );
}

inline codeplace::Options operator& (
    codeplace::Options const & a,
    codeplace::Options const & b
) {
    using Options = codeplace::Options;
    typedef std::underlying_type<Options>::type ut;

    return static_cast<Options>(
        static_cast<ut>(a) & static_cast<ut>(b)
    );
}


} // end namespace hoist



namespace std {

    // Need for std::unordered_set and unordered_map to work on codeplace
    // http://stackoverflow.com/questions/8157937/

    template <>
    struct hash<hoist::codeplace>
    {
        size_t operator()(
            hoist::codeplace const & cp
        ) const
        {
            using Options = hoist::codeplace::Options;

            if (cp._options == Options::None)
                return 0;

            return qHash(cp.getUuid());
        }
    };

}



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

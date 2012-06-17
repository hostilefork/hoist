//
//  codeplace.h - Class that implements an object that
//     encapsulates the notion of an invariant source location.
//     This object can report the line and file number where it
//     was declared, as well as a QUuid which may be either
//     literally provided in the source or temporarily generated
//     through hashes.
//
//              Copyright (c) 2009 HostileFork.com
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)
//
// See http://hostilefork.com/hoist/ for documentation.
//

#ifndef HOIST__CODEPLACE_H
#define HOIST__CODEPLACE_H

// for what should be obvious reasons, codeplace cannot use "hopefully"
// as its assertion service (hopefully is implemented in terms of codeplace!)
#include <cassert>

#include <QUuid>
#include <QByteArray>
#include <QTextStream>
#include <QCryptographicHash>
#include <QCoreApplication>

namespace hoist {

QByteArray Base64StringFromUuid(const QUuid& uuid) ;
QUuid UuidFromBase64String(const QByteArray& str);
QUuid UuidFrom128Bits(const QByteArray& bytes);

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
	// usage pattern of global const char* instead of QString emerged
	// as dominant, so the manager has been removed for now.

	static codeplace makeHere(const char* filename, const long& line)
	{
		return codeplace (filename, line);
	}

	static codeplace makeHere(const QString& filename, const long& line)
	{
		return codeplace (filename, line);
	}

	static codeplace makePlace(const char*& filename, const long& line, const char*& uuidString)
	{
		return codeplace (filename, line, uuidString);
	}

	static codeplace makePlace(const QString& filename, const long& line, const QString& uuidString)
	{
		return codeplace (filename, line, uuidString);
	}

	static codeplace makeThere(const QString& filename, const long& line, const codeplace& cp)
	{
		QString uuidString (Base64StringFromUuid(cp.getUuid()));
		return codeplace (filename, line, uuidString);
	}

	static codeplace makeYonder(const QString& yonderString, const codeplace& cp)
	{
		// faster than Md5 and we're not worried about security/attacks
		// NOTE: Qt hash function is thread-safe
		QByteArray bytes (QCryptographicHash::hash(yonderString.toUtf8(), QCryptographicHash::Md4));
		// TODO: Fix this to comply with UUID format rules, random or hashed bits will
		// contain lies in the bits indicating what protocol the UUID was generated in
		// accordance with (e.g. a pseudo-UUID)
		QString uuidString (Base64StringFromUuid(UuidFrom128Bits(bytes)));
		return codeplace (cp.getFilename(), cp.getLine(), uuidString);
	}

public:
	// I do not like this very much but default constructible is needed for several
	// purposes, including qRegisterMetaType.
	codeplace () :
		options (NullCodeplace),
		filenameEither (NULL),
		line (-1),
		uuidEither (NULL)
	{
	}

	// copy constructor is public
	codeplace (const codeplace& other) :
		options (NullCodeplace),
		filenameEither (NULL),
		line (other.line),
		uuidEither (NULL)
	{
		transitionFromNull(other);
	}

	virtual ~codeplace()
	{
		transitionToNull();
	}

public:
	codeplace& operator= (const codeplace& rhs)
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

	int operator== (const codeplace& rhs) const
	{
		// Should we allow comparisons of null codeplaces?
		/* assert(options != NullCodeplace); */

		// TODO: current semantics is uuid equality, is that sensible?
		// also what to do about null uuids?
		return this->getUuid() == rhs.getUuid();
	}

	QString getFilename() const
	{
		assert(options != NullCodeplace);

		// TODO: behind the scenes mutation into using a QString?
		// would need to be thread safe if so
		if (options & FilenameIsQString) {
			return *filenameQString;
		}
		return QString(filenameCString);
	}

	long getLine() const
	{
		assert(options != NullCodeplace);
		return line;
	}

	QUuid getUuid() const
	{
		assert(options != NullCodeplace);

		if (options & Permanent) {
			if (options & UuidIsQString) {
				return UuidFromBase64String((*uuidQString).toAscii());
			}
			return UuidFromBase64String(QByteArray(uuidCString));
		} else if (options & Hashed) {

			// TODO: Decide what string format will hash best
			QString filenameAndLine;
			if (options & FilenameIsQString)
				filenameAndLine = QString::number(line, 10) + *filenameQString;
			else
				filenameAndLine = QString::number(line, 10) + filenameCString;
			QByteArray bytes (QCryptographicHash::hash(filenameAndLine.toUtf8(), QCryptographicHash::Md4));
			return UuidFrom128Bits(bytes);
		} else {
			// null uuid for null codeplace... should be caught above
			return QUuid ();
		}
	}

	operator QUuid () const
	{
		return getUuid();
	}

	QString toString() const
	{
		QString result;
		QTextStream ts (&result);
		ts << "File: '" << getFilename() << "' - " << " Line # " << getLine();
		return result;
	}

	bool isPermanent() const
	{
		return (options & Permanent);
	}

private:
	void transitionToNull()
	{
		if (options & FilenameIsQString) {
			delete filenameQString;
			filenameQString = NULL;
		}

		if ((options & Permanent) and (options & UuidIsQString)) {
			delete uuidQString;
			uuidQString = NULL;
		}
	}

	void transitionFromNull(const codeplace& other)
	{
		options = other.options;

		if (other.options & NullCodeplace)
			return;

		if (other.options & FilenameIsQString) {
			filenameQString = new QString (*other.filenameQString);
		} else {
			filenameCString = other.filenameCString;
		}

		if (other.options & Permanent) {
			if (other.options & UuidIsQString) {
				uuidQString = new QString (*other.uuidQString);
			} else {
				uuidCString = other.uuidCString;
			}
		}

		line = other.line;
	}

public:
	enum Option {
		 // I don't like the fact that we need default constructible for qRegisterMetaType
		NullCodeplace = 0x0,

		// is either "permanent" or "hashed", can't actually be both
		Hashed = 0x1,
		Permanent = 0x2,

		// codeplace is optimized to avoid making a lot of QStrings out of immutable
		// string literals that you're already paying for in the compiler's constant pool.
		// The only time it actually uses QStrings is if you pass in a UUID or filename
		// string that needs to have its own memory management.
		FilenameIsQString = 0x4,
		UuidIsQString = 0x8

		// Note that as a general rule... if you enable string pooling a.k.a.
		// string interning you can save memory, e.g. when 100 asserts in the
		// same file are all using __FILE__ and thus producing a copy of the literal
		// "foo.cpp" you only pay for the memory to hold that string one time.
		//
		//    http://en.wikipedia.org/wiki/String_interning
		//    http://msdn.microsoft.com/en-us/library/s0s0asdt(VS.80).aspx
	};
        Q_DECLARE_FLAGS(Options, Option)

protected:
	codeplace (const QString& filename, const long& line, const QString& uuidString) :
		options (Permanent | FilenameIsQString | UuidIsQString),
		filenameQString (new QString (filename)),
		line (line),
		uuidQString (new QString (uuidString))
	{
	}

	codeplace (const char* filename, const long& line, const char* uuidString) :
		options (Permanent),
		filenameCString (filename),
		line (line),
		uuidCString (uuidString)
	{
	}

	codeplace (const QString& filename, const long& line) :
		options (Hashed | FilenameIsQString),
		filenameQString (new QString (filename)),
		line (line),
		uuidEither (NULL)
	{
	}

	codeplace (const char* filename, const long& line) :
		options (Hashed),
		filenameCString (filename),
		line (line),
		uuidEither (NULL)
	{
	}

private:
	Options options;
	union {
		QString* filenameQString; // dynamically allocated, we are responsible for freeing
		const char* filenameCString; // must be in constant pool, assumed valid forever
		void* filenameEither; // use this to copy or set null
	};
	long line; // boost::assert used long, I'm just following their lead
	union {
		QString* uuidQString; // dynamically allocated, we are responsible for freeing
		const char* uuidCString; // must be in constant pool, assumed valid forever
		void* uuidEither; // use this to copy or set null
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
// the supplied codeplace provides the file and the line to report.  You'd use this
// if you can't break out the file and line from the error message, e.g. when you're
// not given this information (such as in a Qt warning handler).  As long as
// the message string stays consistent letter-for-letter, you will generate the
// same codeplace.
//

#define YONDER(yonderString, cp) \
	hoist::codeplace::makeYonder((yonderString), (cp))

#endif

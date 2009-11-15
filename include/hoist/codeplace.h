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
#include <QSharedPointer>
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
		FilenameIsCString = 0x4,
		UuidIsCString = 0x8

		// Note that as a general rule... if you enable string pooling a.k.a.
		// string interning you can save memory, e.g. when 100 asserts in the
		// same file are all using __FILE__ and thus producing a copy of the literal
		// "foo.cpp" you only pay for the memory to hold that string one time.
		//
		//    http://en.wikipedia.org/wiki/String_interning
		//    http://msdn.microsoft.com/en-us/library/s0s0asdt(VS.80).aspx
	};
        Q_DECLARE_FLAGS(Options, Option)

public:
	class manager {
	public:
		codeplace makeHere(const char* filename, const long& line)
		{
			return codeplace (filename, line);
		}

		codeplace makeHere(const QString& filename, const long& line)
		{
			return codeplace (filename, line);
		}

		codeplace makePlace(const char*& filename, const long& line, const char*& uuidString)
		{
			return codeplace (filename, line, uuidString);
		}

		codeplace makePlace(const QString& filename, const long& line, const QString& uuidString)
		{
			return codeplace (filename, line, uuidString);
		}

		codeplace makeThere(const QString& filename, const long& line, const codeplace& cp)
		{
			QString uuidString (Base64StringFromUuid(cp.getUuid()));
			return codeplace (filename, line, uuidString);
		}

		codeplace makeYonder(const QString& yonderString, const codeplace& cp)
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

		// because codeplace is sometimes used to initialize global variables, the
		// manager is created implicitly when the first instance is created.  Due to this
		// pattern, if codeplace ever gets fancier and uses QMutex and other
		// primitives it must distinguish between the time before the qApp has been
		// created and after.  At the moment those dependencies are not in use.
		manager()
		{
		}

	private:
		 bool canHaveThreads()
		{
			// we cannot use read write locks/etc. if the QCoreApplication is not initialized
			// but if it *is* initialized, we must!
			//
			// (it should never become initialized while we are in mid-method call because
			// at the moment of initializing there is only one thread running, and if your
			// program is properly written it should not become uninitialized in mid-method
			// call because you shouldn't have threads still running after your QCoreApplication
			// has been destroyed.)
			return qApp;
		}
	public:
		static QSharedPointer< manager > instance;
		static manager& ensureInstance()
		{
			if (not instance)
				instance = QSharedPointer< manager >(new manager ());
			return *instance;
		}
		virtual ~manager()
		{
		}
	};

private:
	Options options;
	QString filenameQString; // can't union a QString type, meh.
	const char* filenameCString;
	long line; // boost::assert used long, I'm just following their lead
	QString uuidQString; // once again, can't union a QString
	const char* uuidCString;

public:
	// I do not like this very much but default constructible is needed for several
	// purposes, including qRegisterMetaType.
	codeplace () :
		options (NullCodeplace),
		filenameQString (),
		filenameCString (NULL),
		line (-1),
		uuidQString (),
		uuidCString (NULL)
	{
	}

protected:
	codeplace (const QString& filename, const long& line, const QString& uuidString) :
		options (Permanent),
		filenameQString (filename),
		filenameCString (NULL),
		line (line),
		uuidQString (uuidString),
		uuidCString (NULL)
	{
	}

	codeplace (const char* filename, const long& line, const char* uuidString) :
		options (Permanent | FilenameIsCString | UuidIsCString),
		filenameQString (),
		filenameCString (filename),
		line (line),
		uuidQString (),
		uuidCString (uuidString)
	{
	}

	codeplace (const QString& filename, const long& line) :
		options (Hashed | FilenameIsCString),
		filenameQString (filename),
		filenameCString (NULL),
		line (line),
		uuidQString (),
		uuidCString (NULL)
	{
	}

	codeplace (const char* filename, const long& line) :
		options (Hashed | FilenameIsCString),
		filenameQString (),
		filenameCString (filename),
		line (line),
		uuidQString (),
		uuidCString (NULL)
	{
	}

public:
	// copy constructor is public
	codeplace (const codeplace& other) :
		options (other.options),
		filenameQString (other.filenameQString),
		filenameCString (other.filenameCString),
		line (other.line),
		uuidQString (other.uuidQString),
		uuidCString (other.uuidCString)
	{
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
		if (options & FilenameIsCString) {
			return QString(filenameCString);
		}
		return filenameQString;
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
			if (options & UuidIsCString) {
				return UuidFromBase64String(QByteArray(uuidCString));
			}
			return UuidFromBase64String(uuidQString.toAscii());
		} else if (options & Hashed) {

			// TODO: Decide what string format will hash best
			QString filenameAndLine;
			if (options & FilenameIsCString)
				filenameAndLine = QString::number(line, 10) + filenameCString;
			else
				filenameAndLine = QString::number(line, 10) + filenameQString;
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

	virtual ~codeplace()
	{
	}
};

Q_DECLARE_OPERATORS_FOR_FLAGS(codeplace::Options)

// we moc this file.  but currently no Qt objects.  doesn't mean there won't ever be
// so this dummy object suppresses the warning "No relevant classes found" during moc
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
	hoist::codeplace::manager::ensureInstance().makeHere(__FILE__, __LINE__)

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
	hoist::codeplace::manager::ensureInstance().makePlace(__FILE__, __LINE__, (uuidString))

//
// "THERE" is for cases where you want to talk about a remote source line and
// file, and have a consistent codeplace for describing it.  It is for
// useful when a client library offers an assert hook with a file
// and line, and you would like to employ the logic used to make ephemeral
// codeplaces.
//

#define THERE(filename, line, cp) \
	hoist::codeplace::manager::ensureInstance().makeThere((filename), (line), (cp))

//
// With "YONDER", the supplied string is hashed to produce the uuid, while
// the supplied codeplace provides the file and the line to report.  You'd use this
// if you can't break out the file and line from the error message, e.g. when you're
// not given this information (such as in a Qt warning handler).  As long as
// the message string stays consistent letter-for-letter, you will generate the
// same codeplace.
//

#define YONDER(yonderString, cp) \
	hoist::codeplace::manager::ensureInstance().makeYonder((yonderString), (cp))

#endif
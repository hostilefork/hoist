//
// listed.h - The "listed" template gives you the ability to enumerate
//	over a list of the instances of the wrapped type that are in
//	existence.  It's thread-safe, however it only provides a snapshot
//	of the list at a moment in time.  If these semantics are not suitable
//	then you have to tackle it with additional synchronization logic.
//	The object requirements are the same as for tracked (copyable,
//	QTextStream output operator).  In order to achieve thread safety,
//	you will be paying for 2 copies of the value by virtue of using the
//	listed type.  However, implicit sharing means you can often
//	leverage that second copy as the same copy from the list of
//	objects you retrieve when you call getList()
//
//              Copyright (c) 2009 HostileFork.com
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)
//
// See http://hostilefork.com/hoist/ for documentation.
//

#ifndef HOIST__LISTED_H
#define HOIST__LISTED_H

#include "codeplace.h"
#include "hopefully.h"
#include "tracked.h"

#include <QReadWriteLock>

namespace hoist {

template< class ListType > class listed : public tracked< ListType >
{
public:
	class manager
	{
		Q_DISABLE_COPY(manager)

	public:
		manager()
		{
		}

		virtual ~manager()
		{
			foreach(listed< ListType >* ptr, list) {
				// This test should always fail, but reports the
				// allocation point during the failure
				ptr->hopefullyNotEqualTo(*ptr, HERE);
			}
			hopefully(resultCache.empty(), HERE);
		}

	public:
		// due to threading we can't give listed references back because
		// they might get destroyed by another thread while we're holding
		// onto them.  also... due to semantics we can't give listed
		// instances back because then they'd get into the list also.  yet
		// if they're in a QList, then that's the only thing distinguishing them
		// from a tracked... so we upcast and copy construct
		QList< tracked< ListType > > getList()
		{
			listLock.lockForRead();
			QList< tracked< ListType > > result (resultCache);
			listLock.unlock();
			return result;
		}

	private:
		QReadWriteLock listLock;
		QList< listed< ListType >* > list;
		QList< tracked< ListType > > resultCache;
		friend class listed;
	};

public:
	listed (const ListType& value, manager& mgr, const codeplace& cp) :
		tracked< ListType > (value, cp),
		mgr (mgr)
	{
		mgr.listLock.lockForWrite();
		mgr.list.append(this);
		mgr.resultCache.append(*static_cast< tracked< ListType >* >(this));
		mgr.listLock.unlock();
	}

	virtual ~listed()
	{
		mgr.listLock.lockForWrite();
		int indexToRemove = mgr.list.indexOf(this);
		hopefully(indexToRemove != -1, HERE);
		mgr.list.removeAt(indexToRemove);
		mgr.resultCache.removeAt(indexToRemove);
		mgr.listLock.unlock();
	}

private:
	manager& mgr;
};

// we moc this file.  but currently no Qt objects.  doesn't mean there won't ever be
// so this dummy object suppresses the warning "No relevant classes found" during moc
class LISTED_no_moc_warning : public QObject { Q_OBJECT };

} // end namespace hoist

#endif
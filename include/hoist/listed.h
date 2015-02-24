//
// listed.h - The "listed" template gives you the ability to enumerate
//  over a list of the instances of the wrapped type that are in
//  existence.  It's thread-safe, however it only provides a snapshot
//  of the list at a moment in time.  If these semantics are not suitable
//  then you have to tackle it with additional synchronization logic.
//  The object requirements are the same as for tracked (copyable,
//  QTextStream output operator).  In order to achieve thread safety,
//  you will be paying for 2 copies of the value by virtue of using the
//  listed type.  However, implicit sharing means you can often
//  leverage that second copy as the same copy from the list of
//  objects you retrieve when you call getList()
//
//          Copyright (c) 2009-2014 HostileFork.com
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)
//
// See http://hostilefork.com/hoist/ for documentation.
//

#ifndef HOIST_LISTED_H
#define HOIST_LISTED_H

#include "codeplace.h"
#include "hopefully.h"
#include "tracked.h"

#include <QReadWriteLock>

namespace hoist {

template <class T>
class listed : public tracked<T>
{
public:
    class manager
    {
        Q_DISABLE_COPY(manager)

    public:
        manager ()
        {
        }

        virtual ~manager () {
            for (listed<T> * ptr : list) {
                // This test should always fail, but reports the
                // allocation point during the failure
                ptr->hopefullyNotEqualTo(*ptr, HERE);
            }
            hopefully(resultCache.empty(), HERE);
        }


    public:
        // Due to threading we can't give listed references back because
        // they might get destroyed by another thread while we're holding
        // onto them.  Also... due to semantics we can't give listed
        // instances back because then they'd get into the list also.  Yet
        // if they're in a QList, then that's the only thing distinguishing
        // them from a tracked... so we upcast and copy construct
        QList<tracked<T>> getList ()
        {
            QReadLocker lock (&listLock);

            // Makes a thread-safe copy before the unlock
            return resultCache;
        }


    private:
        QReadWriteLock listLock;
        QList<listed<T> *> list;
        QList<tracked<T>> resultCache;
        friend class listed;
    };


public:
    listed (
        T const & value,
        manager & mgr,
        codeplace const & cp
    ) :
        tracked<T> (value, cp),
        mgr (mgr)
    {
        QWriteLocker lock (&mgr.listLock);

        mgr.list.append(this);
        mgr.resultCache.append(*static_cast<tracked<T> *>(this));
    }


    ~listed() override {
        QWriteLocker lock (&mgr.listLock);

        int indexToRemove = mgr.list.indexOf(this);
        hopefully(indexToRemove != -1, HERE);
        mgr.list.removeAt(indexToRemove);
        mgr.resultCache.removeAt(indexToRemove);
    }

private:
    manager & mgr;
};

} // end namespace hoist

#endif

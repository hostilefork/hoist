//
// stacked.h - The "stacked" template implements something that
//  per-thread has a current item on the stack.  the current
//  item will be the last item of type stacked that was constructed
//  on that thread.  You may only destroy stacked types in the
//  reverse order they were created, so general use will be to
//  stack-allocate them (it's unlikely that you will get useful
//  behavior by allocating them dynamically!)
//
//          Copyright (c) 2009-2014 HostileFork.com
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)
//
// See http://hostilefork.com/hoist/ for documentation.
//

#ifndef HOIST_STACKED_H
#define HOIST_STACKED_H

#include "codeplace.h"
#include "hopefully.h"
#include "tracked.h"

#include <QThread>
#include <QReadWriteLock>
#include <QMap>

namespace hoist {

template <class T>
class stacked : public tracked<T>
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
        }

        // gets the top of stack and guarantees you won't pay for a copy-on
        // write copy of the stack structure.  If you want to check if the
        // stack is empty and then take action on that, you probably
        // need to capture the entire stack because otherwise it might
        // change between when you asked and when you make the next call

        tracked<T> getTopHopefully (codeplace const & cp) const {
            QThread * thread = QThread::currentThread();

            QReadLocker lock (&_mapLock);

            QList<tracked<T>> stack = _map[thread];

            if (stack.empty()) {
                throw hopefullyNotReached(
                    "no stacked<> types on stack in getTopHopefully()", cp
                );
            } else {
                return stack.first();
            }
        }

        // NOTE: Due to technical limitations of QStack, I have to use QList.
        // QStack is built upon QVector, and that requires the items you put
        // in it to be default constructible.  That is an unacceptable
        // requirement to put onto tracked<> so you should simply treat the
        // front of the list as the "top" of the stack.

        QList<tracked<T>> getStack () const {
            QThread * thread = QThread::currentThread();

            QReadLocker lock (&_mapLock);

            // Thread-safe copy made before lock is released
            return _map[thread];
        }


    private:
        mutable QReadWriteLock _mapLock;
        QMap<QThread *, QList<tracked<T>>> _map;
        friend class stacked;
    };


public:
    stacked (T value, manager & mgr, codeplace const & cp) :
        tracked<T> (value, cp),
        _mgr (mgr)
    {
        QThread * thread = QThread::currentThread();

        QWriteLocker lock (&_mgr._mapLock);

        _mgr._map[thread].push_front(*static_cast<tracked<T> *>(this));
    }

    ~stacked () override {
        QThread * thread = QThread::currentThread();

        QWriteLocker lock (&_mgr._mapLock);

        QList<tracked<T>> stack = _mgr._map[thread];
        if (stack.front().get() != this->get()) {
            QString message;
            QTextStream ts (&message);
            ts
                << "expected stacked type constructed at "
                << stack.front().whereConstructed().toString()
                << " to have been destroyed before the one constructed at "
                << this->whereConstructed().toString()
                << " (which is currently being destroyed)";
            hopefullyNotReached(message, this->whereConstructed());
        }
        _mgr._map[thread].pop_front();
    }

private:
    manager & _mgr;
};

// we moc this file, though whether there are any QObjects or not may vary
// this dummy object suppresses the warning "No relevant classes found" w/moc
class STACKED_no_moc_warning : public QObject { Q_OBJECT };

} // end namespace hoist

#endif

//
// mapped.h - The "mapped" template implements a variant of
//  the "listed" template in which manages the thread-safe
//  automatic insertion and removal of items into a map.
//  As an added bonus, mapped types save a copy of their key
//  which you can retrieve with getKey().  This abstraction
//  is a helpful alternative in particular to maintaining
//  explicit code which does an insert of an object's
//  pointer into a map when the object is constructed,
//  and then removal on destruction.
//
//          Copyright (c) 2009-2014 HostileFork.com
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)
//
// See http://hostilefork.com/hoist/ for documentation.
//

#ifndef HOIST_MAPPED_H
#define HOIST_MAPPED_H

#include "codeplace.h"
#include "hopefully.h"
#include "tracked.h"

#include <QReadWriteLock>
#include <QMap>

namespace hoist {

template <class Key, class T>
class mapped : public tracked<T>
{
public:
    class manager
    {
        Q_DISABLE_COPY(manager)

    public:
        manager()
        {
        }

        virtual ~manager ()
        {
            for (tracked<T> value : _resultCache) {
                value.hopefullyNotEqualTo(value, HERE);
            }
        }


    public:
        // due to threading we can't give mapped references back because
        // they might get destroyed by another thread while we're holding
        // onto them.  also... due to semantics we can't give listed
        // instances back because then they'd get into the list also.  Yet
        // if they're in a QList, then that's the only thing distinguishing
        // them from a tracked... so we upcast and copy construct.  This drops
        // the key but since you have the QMap the key will be available
        // during any iteration.

        QMap<Key, tracked<T>> getMap () const {
            QReadLocker lock (&_mapLock);

            // The return will make a thread-safe copy before releasing the
            // lock in its destructor
            return _resultCache;
        }


        const T lookupValue (
            Key const & key,
            T const & defaultValue
        ) {
            QReadLocker lock (&_mapLock);

            auto iter = _resultCache.find(key);
            if (iter == _resultCache.end())
                return defaultValue;
            
            return iter.value().get();
        }


        tracked<T> lookupHopefully (
            Key const & key,
            codeplace const & cp
        )
            const
        {
            QReadLocker lock (&_mapLock);

            auto iter = _resultCache.find(key);
            if (iter == _resultCache.end())
                throw hopefullyNotReached(cp);
            
            return iter.value();
        }


    private:
        mutable QReadWriteLock _mapLock;
        QMap<Key, tracked<T>> _resultCache;
        friend class mapped;
    };


public:
    mapped (
        Key const & key,
        T const & value,
        manager & mgr,
        codeplace const & cp
    ) :
        tracked<T> (value, cp),
        _mgr (mgr),
        _key (key)
    {
        QWriteLocker lock (&_mgr._mapLock);
        hopefully(
            not _mgr._resultCache.contains(_key),
            "mapped<> item already exists with key",
            cp
        );
        _mgr._resultCache.insert(_key, *static_cast<tracked<T> *>(this));
    }


    ~mapped () override
    {
        QWriteLocker lock (&_mgr.mapLock);
        hopefully(_mgr._resultCache.remove(_key) == 1, HERE);
    }


public:
    Key const & getKey () const
    {
        return _key;
    }

    virtual void assign (T const & newValue, codeplace const & cp) override
    {
        tracked<T>::assign(newValue, cp);

        QWriteLocker lock (&_mgr.mapLock);
        hopefully(1 == _mgr._resultCache.remove(_key), cp);
        _mgr._resultCache.insert(_key, *static_cast<tracked<T> *>(this));
    }


private:
    manager & _mgr;
    Key _key;
};

} // end namespace hoist

#endif

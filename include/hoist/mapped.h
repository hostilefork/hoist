//
// mapped.h - The "mapped" template implements a variant of
//	the "listed" template in which manages the thread-safe
//	automatic insertion and removal of items into a map.
//	As an added bonus, mapped types save a copy of their key
//	which you can retrieve with getKey().  This abstraction
//	is a helpful alternative in particular to maintaining
//	explicit code which does an insert of an object's
//	pointer into a map when the object is constructed,
//	and then removal on destruction.
//
//              Copyright (c) 2009 HostileFork.com
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

template<class Key, class T> class mapped : public tracked<T>
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
            foreach(tracked<T> value, resultCache) {
				value.hopefullyNotEqualTo(value, HERE);
			}
		}

	public:
		// due to threading we can't give mapped references back because
		// they might get destroyed by another thread while we're holding
		// onto them.  also... due to semantics we can't give listed
		// instances back because then they'd get into the list also.  yet
		// if they're in a QList, then that's the only thing distinguishing them
		// from a tracked... so we upcast and copy construct.  This drops
		// the key but since you have the QMap the key will be available
		// during any iteration.
        QMap<Key, tracked<T> > getMap() const
		{
			mapLock.lockForRead();
            QMap<Key, tracked<T> > result (resultCache);
			mapLock.unlock();
			return result;
		}

        const T lookupValue(const Key& key, const T& defaultValue)
		{
			mapLock.lockForRead();
            typename QMap<Key, tracked<T> >::const_iterator i (resultCache.find(key));
			if (i == resultCache.end()) {
				mapLock.unlock();
				return defaultValue;
			}
            T result (i.value().get());
			mapLock.unlock();
			return result;
		}

        tracked<T> lookupHopefully(const Key& key, const codeplace& cp) const
		{
			mapLock.lockForRead();
            typename QMap<Key, tracked<T> >::const_iterator i (resultCache.find(key));
			if (i == resultCache.end()) {
				hopefullyNotReached(cp);
				mapLock.unlock();
				return i.value(); // this will crash but what else do I return?
                // TODO: revisit default value semantics, I don't want to force T to be default constructible...
			}
            tracked<T> result (i.value());
			mapLock.unlock();
			return result;
		}

	private:
		mutable QReadWriteLock mapLock;
        QMap<Key, tracked<T> > resultCache;
		friend class mapped;
	};

public:
    mapped (const Key& key, const T& value, manager& mgr, const codeplace& cp) :
        tracked<T> (value, cp),
		mgr (mgr),
		key (key)
	{
		mgr.mapLock.lockForWrite();
		hopefully(not mgr.resultCache.contains(key), "mapped<> item already exists with key", cp);
        mgr.resultCache.insert(key, *static_cast< tracked<T>* >(this));
		mgr.mapLock.unlock();
	}

    ~mapped() override
	{
		mgr.mapLock.lockForWrite();
		hopefully(mgr.resultCache.remove(key) == 1, HERE);
		mgr.mapLock.unlock();
	}

public:
    const Key& getKey() const
	{
		return key;
	}

    virtual void assign(const T& newValue, const codeplace& cp) override
	{
		mgr.mapLock.lockForWrite();
		hopefully(1 == mgr.resultCache.remove(key), cp);
        tracked<T>::assign(newValue, cp);
        mgr.resultCache.insert(key, *static_cast< tracked<T>* >(this));
		mgr.mapLock.unlock();
	}

private:
	manager& mgr;
    Key key;
};

// we moc this file, though whether there are any QObjects or not may vary
// this dummy object suppresses the warning "No relevant classes found" w/moc
class MAPPED_no_moc_warning : public QObject { Q_OBJECT };

} // end namespace hoist

#endif

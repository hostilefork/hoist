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

#ifndef HOIST__MAPPED_H
#define HOIST__MAPPED_H

#include "codeplace.h"
#include "hopefully.h"
#include "tracked.h"

#include <QReadWriteLock>
#include <QMap>

namespace hoist {

template<class KeyType, class ValueType> class mapped : public tracked<ValueType>
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
            foreach(tracked<ValueType> value, resultCache) {
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
        QMap<KeyType, tracked<ValueType> > getMap() const
		{
			mapLock.lockForRead();
            QMap<KeyType, tracked<ValueType> > result (resultCache);
			mapLock.unlock();
			return result;
		}

		const ValueType lookupValue(const KeyType& key, const ValueType& defaultValue)
		{
			mapLock.lockForRead();
            typename QMap<KeyType, tracked<ValueType> >::const_iterator i (resultCache.find(key));
			if (i == resultCache.end()) {
				mapLock.unlock();
				return defaultValue;
			}
			ValueType result (i.value().get());
			mapLock.unlock();
			return result;
		}

        tracked<ValueType> lookupHopefully(const KeyType& key, const codeplace& cp) const
		{
			mapLock.lockForRead();
            typename QMap<KeyType, tracked<ValueType> >::const_iterator i (resultCache.find(key));
			if (i == resultCache.end()) {
				hopefullyNotReached(cp);
				mapLock.unlock();
				return i.value(); // this will crash but what else do I return?
				// TODO: revisit default value semantics, I don't want to force ValueType to be default constructible...
			}
            tracked<ValueType> result (i.value());
			mapLock.unlock();
			return result;
		}

	private:
		mutable QReadWriteLock mapLock;
        QMap<KeyType, tracked<ValueType> > resultCache;
		friend class mapped;
	};

public:
	mapped (const KeyType& key, const ValueType& value, manager& mgr, const codeplace& cp) :
		tracked< ValueType > (value, cp),
		mgr (mgr),
		key (key)
	{
		mgr.mapLock.lockForWrite();
		hopefully(not mgr.resultCache.contains(key), "mapped<> item already exists with key", cp);
		mgr.resultCache.insert(key, *static_cast< tracked< ValueType >* >(this));
		mgr.mapLock.unlock();
	}

	virtual ~mapped()
	{
		mgr.mapLock.lockForWrite();
		hopefully(mgr.resultCache.remove(key) == 1, HERE);
		mgr.mapLock.unlock();
	}

public:
	const KeyType& getKey() const
	{
		return key;
	}

	/* virtual */ void assign(const ValueType& newValue, const codeplace& cp)
	{
		mgr.mapLock.lockForWrite();
		hopefully(1 == mgr.resultCache.remove(key), cp);
		tracked< ValueType >::assign(newValue, cp);
		mgr.resultCache.insert(key, *static_cast< tracked< ValueType >* >(this));
		mgr.mapLock.unlock();
	}

private:
	manager& mgr;
	KeyType key;
};

// we moc this file, though whether there are any QObjects or not may vary
// this dummy object suppresses the warning "No relevant classes found" w/moc
class MAPPED_no_moc_warning : public QObject { Q_OBJECT };

} // end namespace hoist

#endif

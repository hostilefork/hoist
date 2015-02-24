//
// tracked.h - Tracked provides a template wrapper you can apply
//  to your data types that helps you keep track of where an
//  object was declared, as well as where the last assignment
//  was made.  The type also has its own assert-like methods
//  that produce something more informative than a boolean
//  failure (such as "hopefullyEqualTo" and "hopefullyInSet")
//  This means it is able to capture specific values of runtime
//  data instead ofthe source expression of the assert.  To use it,
//  the type must be copyable and also must have a
//  "QTextStream & operator<<" defined.
//
//          Copyright (c) 2009-2014 HostileFork.com
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)
//
// See http://hostilefork.com/hoist/ for documentation.
//

#ifndef HOIST_TRACKED_H
#define HOIST_TRACKED_H

#include "codeplace.h"
#include "hopefully.h"

namespace hoist {

template <class T>
class tracked
{
public:
    tracked (T const & value, codeplace const & cp) :
        _value (value),
        _constructLocation (cp),
        _lastAssignLocation (cp)
    {
    }

    tracked (T && value, codeplace const & cp) :
        _value (std::move(value)),
        _constructLocation (cp),
        _lastAssignLocation (cp)
    {
    }

    // you can copy construct a tracked type but obviously you won't
    // get the construction location where the copy happened... but where
    // the other object was constructed.  But we need this to put tracked
    // types into collection classes.
    //
    // REVIEW: There are some extra bits in codeplace at the moment.
    // might it be useful to have a "copy constructed" bit to document
    // this situation?
    tracked (const tracked<T>& other) :
        _value (other._value),
        _constructLocation (other._constructLocation),
        _lastAssignLocation (other._lastAssignLocation)
    {
    }

    virtual ~tracked()
        { }

public:
    // Basic accessors, the value has an implicit casting operator so it can
    // act like the value it's tracking for reads

    operator T const & () const {
        return _value;
    }

    T const & get () const {
        return _value;
    }

    T const & operator-> () const {
        return _value;
    }


    // Tracked types may eventually offer a longer value/location history
    // Perhaps another parameter to the template for the length?

    codeplace whereConstructed () const {
        return _constructLocation;
    }

    codeplace whereLastAssigned () const {
        return _lastAssignLocation;
    }


public:
    // Is there a better way to do this that doesn't involve the caller
    // generating some kind of costly set structure?  Variadic templates?
    //
    // https://github.com/hostilefork/hoist/issues/5

    bool hopefullyInSet (
        T const & v1, T const & v2, T const & v3, codeplace const & cp
    )
        const
    {
        const T* goodValues[3] = {&v1, &v2, &v3};
        return hopefullyInSetCore(static_cast<const T**>(goodValues), 3, cp);
    }

    bool hopefullyInSet (
        T const & v1,
        T const & v2,
        codeplace const & cp
    )
        const
    {
        const T* goodValues[2] = {&v1, &v2};
        return hopefullyInSetCore(static_cast<const T**>(goodValues), 2, cp);
    }

    bool hopefullyEqualTo(T const & value, codeplace const & cp) const
    {
        const T* goodValues[1] = {&value};
        return hopefullyInSetCore(static_cast<const T**>(goodValues), 1, cp);
    }

public:
    bool hopefullyNotInSet(
        T const & v1, T const & v2, T const & v3, codeplace const & cp
    )
        const
    {
        const T* badValues[3] = {&v1, &v2, &v3};
        return hopefullyNotInSetCore(static_cast<const T**>(badValues), 3, cp);
    }

    bool hopefullyNotInSet(
        T const & v1, T const & v2, codeplace const & cp
    )
        const
    {
        const T* badValues[2] = {&v1, &v2};
        return hopefullyNotInSetCore(static_cast<const T**>(badValues), 2, cp);
    }

    bool hopefullyNotEqualTo(T const & value, codeplace const & cp) const
    {
        const T* badValues[1] = {&value};
        return hopefullyNotInSetCore(static_cast<const T**>(badValues), 1, cp);
    }



//
// Operations for setting the value
//
public:
    virtual void assign (T const & newValue, codeplace const & cp)
    {
        _value = newValue;
        _lastAssignLocation = cp;
    }

    virtual void assign (T && newValue, codeplace const & cp)
    {
        _value = std::move(newValue);
        _lastAssignLocation = cp;
    }

    void guarantee (T const & newValue, codeplace const & cp)
    {
        if (_value != newValue)
            assign(newValue, cp);
    }

    // Should this return something other than boolean, and perhaps be called
    // alterHopefully?
    //
    // https://github.com/hostilefork/hoist/issues/7

    bool hopefullyAlter (T const & newValue, codeplace const & cp) {
        bool result = hopefullyNotEqualTo(newValue, cp);
        assign(newValue, cp);
        return result;
    }

    bool hopefullyAlter (T && newValue, codeplace const & cp) {
        bool result = hopefullyNotEqualTo(newValue, cp);
        assign(std::move(newValue), cp);
        return result;
    }

    bool hopefullyTransition (
        T const & oldValue,
        T const & newValue,
        codeplace const & cp
    ) {
        bool result = hopefullyEqualTo(oldValue, cp);
        assign(newValue, cp);
        return result;
    }

    bool hopefullyTransition (
        T const & oldValue,
        T && newValue,
        codeplace const & cp
    ) {
        bool result (hopefullyEqualTo(oldValue, cp));
        assign(std::move(newValue), cp);
        return result;
    }


protected:
    bool hopefullyInSetCore (
        T const * * goodValues,
        size_t const & numGoodValues,
        codeplace const & cp
    )
        const
    {
        hopefully(numGoodValues != 0, HERE);

        for (size_t index = 0; index < numGoodValues; index++) {
            if (_value == *(goodValues[index]))
                return true;
        }

        QString message;
        QTextStream ts (&message);
        if (numGoodValues == 1) {
            ts << "Expected value to be " << *(goodValues[0]);
        } else {
            ts << "Expected value to be in set [";
            for(size_t index = 0; index < numGoodValues; index++) {
                ts << *(goodValues[index]);
                if (index == numGoodValues - 1)
                    ts << "] ";
                else
                    ts << ",";
            }
        }
        ts << " and it was " << _value << endl;
        ts <<"Last assignment was at " << _lastAssignLocation.toString();
        hopefullyNotReached(message, cp);
        return false;
    }

    bool hopefullyNotInSetCore (
        T const * * badValues,
        size_t const & numBadValues,
        codeplace const & cp
    )
        const
    {
        hopefully(numBadValues != 0, HERE);

        for (size_t index = 0; index < numBadValues; index++) {
            if (_value == *(badValues[index])) {

                QString message;
                QTextStream ts (&message);
                if (numBadValues == 1) {
                    ts << "Didn't expect value to be "
                        << *badValues[0]
                        << '\n';
                } else {
                    ts << "Didn't expect value to be in set [";
                    for (size_t index = 0; index < numBadValues; index++) {
                        ts << *(badValues[index]);
                        if (index == numBadValues - 1)
                            ts << "] ";
                        else
                            ts << ",";
                    }
                    ts << " and it was " << _value << '\n';
                }
                ts <<"Last assignment at " << _lastAssignLocation.toString();
                hopefullyNotReached(message, cp);
                return false;
            }
        }

        return true;
    }

private:
    T _value;
    codeplace _constructLocation;
    codeplace _lastAssignLocation;
};

} // end namespace hoist

#endif

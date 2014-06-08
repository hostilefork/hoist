//
// tracked.h - Tracked provides a template wrapper you can apply
//	to your data types that helps you keep track of where an
//	object was declared, as well as where the last assignment
//	was made.  The type also has its own assert-like methods
//	that produce something more informative than a boolean
//	failure (such as "hopefullyEqualTo" and "hopefullyInSet")
//	This means it is able to capture specific values of runtime
//	data instead ofthe source expression of the assert.  To use it,
//	the type must be copyable and also must have a
//	"QTextStream& operator<<" defined.
//
//              Copyright (c) 2009 HostileFork.com
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

template<class T> class tracked
{
public:
    tracked (T const & value, const codeplace& cp) :
		value (value),
		constructLocation (cp),
		lastAssignLocation (cp)
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
		value (other.value),
		constructLocation (other.constructLocation),
		lastAssignLocation (other.lastAssignLocation)
	{
	}

	virtual ~tracked()
		{ }

public:
	// Basic accessors, the value has a cast operator so it acts like what it's tracking
    operator const T&() const
		{ return value; }
    const T& get() const
		{ return value; }
    const T& operator-> () const
		{ return value; }

	// Tracked types may eventually offer a longer value/location history
	// Perhaps another parameter to the template for the length?
	codeplace whereConstructed() const
		{ return constructLocation; }
	codeplace whereLastAssigned() const
		{ return lastAssignLocation; }

public:
	// Is there a better way to do this that doesn't involve the caller generating some kind
	// of costly set structure?  Variadic templates, perhaps?
	//
	// http://www.boostcon.com/community/wiki/show/Start/HandsOnCXX0x/VariadicTemplates/
    bool hopefullyInSet(const T& v1, const T& v2, const T& v3, const codeplace& cp) const
	{
        const T* goodValues[3] = {&v1, &v2, &v3};
        return hopefullyInSetCore(static_cast<const T**>(goodValues), 3, cp);
	}
    bool hopefullyInSet(const T& v1, const T& v2, const codeplace& cp) const
	{
        const T* goodValues[2] = {&v1, &v2};
        return hopefullyInSetCore(static_cast<const T**>(goodValues), 2, cp);
	}
    bool hopefullyEqualTo(const T& value, const codeplace& cp) const
	{
        const T* goodValues[1] = {&value};
        return hopefullyInSetCore(static_cast<const T**>(goodValues), 1, cp);
	}

public:
    bool hopefullyNotInSet(const T& v1, const T& v2, const T& v3, const codeplace& cp) const
	{
        const T* badValues[3] = {&v1, &v2, &v3};
        return hopefullyNotInSetCore(static_cast<const T**>(badValues), 3, cp);
	}
    bool hopefullyNotInSet(const T& v1, const T& v2, const codeplace& cp) const
	{
        const T* badValues[2] = {&v1, &v2};
        return hopefullyNotInSetCore(static_cast<const T**>(badValues), 2, cp);
	}
    bool hopefullyNotEqualTo(const T& value, const codeplace& cp) const
	{
        const T* badValues[1] = {&value};
        return hopefullyNotInSetCore(static_cast<const T**>(badValues), 1, cp);
	}

public:
	// Operations for setting the value
    virtual void assign(T const & newValue, const codeplace& cp)
	{
		value = newValue;
		lastAssignLocation = cp;
	}
    void guarantee(const T& newValue, const codeplace& cp)
	{
		if (value != newValue)
			assign(newValue, cp);
	}
	// TODO: The question of what these hopefully routines should return is up in
	// the air right now.  I would like it to be possible that a hopefully call would go
	// hit a database, and possibly say "that's one we can ignore...".  But when
	// I think about pointer constructs, like:
	//
	//	ptr1 = hopefullyNotNull(ptr2, HERE)
	//
	// I wonder if a boolean result might not be as good as returning the pointer
	// that was passed in.  For the moment I'm making all hopefullys return
	// true if their hope was met, and fase if it wasn't... but perhaps that
	// should be thought about.
    bool hopefullyAlter(T const & newValue, const codeplace& cp)
	{
		bool result (hopefullyNotEqualTo(newValue, cp));
		assign(newValue, cp);
		return result;
	}
    bool hopefullyTransition(T const & oldValue, T const & newValue, const codeplace& cp) {
		bool result (hopefullyEqualTo(oldValue, cp));
		assign(newValue, cp);
		return result;
	}

protected:
    bool hopefullyInSetCore(const T** goodValues, const size_t& numGoodValues, const codeplace& cp) const
	{
		hopefully(numGoodValues != 0, HERE);

		for(size_t index = 0; index < numGoodValues; index++) {
			if (value == *(goodValues[index]))
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
		ts << " and it was " << value << endl;
		ts <<"Last assignment was at " << lastAssignLocation.toString();
		hopefullyNotReached(message, cp);
		return false;
	}

    bool hopefullyNotInSetCore(const T** badValues, const size_t& numBadValues, const codeplace& cp) const
	{
		hopefully(numBadValues != 0, HERE);

		for(size_t index = 0; index < numBadValues; index++) {
			if (value == *(badValues[index])) {

				QString message;
				QTextStream ts (&message);
				if (numBadValues == 1) {
					ts << "Did not expect value to be " << *(badValues[0]) << endl;
				} else {
					ts << "Did not expect value to be in set [";
					for(size_t index = 0; index < numBadValues; index++) {
						ts << *(badValues[index]);
						if (index == numBadValues - 1)
							ts << "] ";
						else
							ts << ",";
					}
					ts << " and it was " << value << endl;
				}
				ts <<"Last assignment was at " << lastAssignLocation.toString();
				hopefullyNotReached(message, cp);
				return false;
			}
		}

		return true;
	}

private:
    T value;
	codeplace constructLocation;
	codeplace lastAssignLocation;
};

// we moc this file, though whether there are any QObjects or not may vary
// this dummy object suppresses the warning "No relevant classes found" w/moc
class TRACKED_no_moc_warning : public QObject { Q_OBJECT };

} // end namespace hoist

#endif

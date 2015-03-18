/*
								SuperEasyJSON
					http://www.sourceforge.net/p/supereasyjson
	
	The MIT License (MIT)

	Copyright (c) 2013 Jeff Weinstein (jeff.weinstein at gmail)

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in
	all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
	THE SOFTWARE.

	CHANGELOG:
	==========
 	2/8/2014:
 	--------- 
 	MAJOR BUG FIXES, all courtesy of Per Rovegård, Ph.D.
 	* Feature request: HasKey and HasKeys added to Value for convenience and
 		to avoid having to make a temporary object.
 	* Strings should now be properly unescaped. Previously, as an example, the
 		string "\/Date(1390431949211+0100)\/\" would be parsed as
 		\/Date(1390431949211+0100)\/. The string is now properly parsed as
 		/Date(1390431949211+0100)/.
 		As per http://www.json.org the other escape characters including
 		\u+4 hex digits will now be properly unescaped. So for example,
 		\u0061 now becomes "A".
 	* Serialize now supports serializing a toplevel array (which is valid JSON).
 		The parameter it takes is now a Value, but existing code doesn't
 		need to be changed.
 	* Fixed bug with checking for proper opening/closing sequence for braces/brackets.
 		Previously, this code: 
			const char *json = "{\"arr\":[{}}]}";
			auto val = json::Deserialize(json);
		worked fine with no errors. That's a bug. I did a major overhaul so that
 		now improperly formatted pairs will now correctly result in an error.
 	* Made internal deserialize methods static
 
 	1/30/2014:
 	----------
 	* Changed #pragma once to the standard #ifndef header guard style for
 		better compatibility.
 	* Added a [] operator for Value that takes a const char* as an argument
 		to avoid having to explicitly (and annoyingly) cast to std::string.
 		Thus, my_value["asdf"] = "a string" should now work fine.
 		The same has been added to the Object class.
 	* Added non-operator methods of casting a Value to int/string/bool/etc.
 		Implicitly casting a Value to a std::string doesn't work as per C++
 		rules. As such, previously to assign a Value to a std::string you
 		had to do:
 			my_std_string = (std::string)my_value;
 		You can now instead do:
 			my_std_string = my_value.ToString();
 		If you want more information on why this can't be done, please read
 		this topic for more details:
 		http://stackoverflow.com/questions/3518145/c-overloading-conversion-operator-for-custom-type-to-stdstring
 
 	1/27/2014
 	----------
 	* Deserialize will now return a NULLType Value instance if there was an
 		error instead of asserting. This way you can handle however you want to
 		invalid JSON being passed in. As a top level object must be either an
 		array or an object, a NULL value return indicates an invalid result.
 
 	1/11/2014
 	---------
 	* Major bug fix: Strings containing []{} characters could cause
 		parsing errors under certain conditions. I've just tested
 		the class parsing a 300KB JSON file with all manner of bizarre
 		characters and permutations and it worked, so hopefully this should
 		be the end of "major bug" fixes.
 
 	1/10/2014
 	---------
 	Bug fixes courtesy of Gerry Beauregard:
 	* Pretty big bug: was using wrong string paramter in ::Deserialize
 		and furthermore it wasn't being trimmed. 
 	* Object::HasKeys now casts the return value to avoid compiler warnings.
 	* Slight optimization to the Trim function
 	* Made asserts in ::Deserialize easier to read
 
 	1/9/2014
 	--------
 	* Major bug fix: for JSON strings containing \" (as in, two characters,
 		not the escaped " character), the lib would mess up and not parse
 		correctly.
 	* Major bug fix: I erroneously was assuming that all root JSON types
 		had to be an object. This was an oversight, as a root JSON
 		object can be an array. I have therefore changed the Deserialize
 		method to return a json::Value rather than a json::Object. This
 		will NOT impact any existing code you have, as a json::Value will
 		cast to a json::Object (if it is indeed an object). But for 
 		correctness, you should be using json::Value = Deserialize...
 		The Value type can be checked if it's an array (or any other type),
 		and furthermore can even be accessed with the [] operator for
 		convenience.
 	* I've made the NULL value type set numeric fields to 0 and bool to false.
 		This is for convenience for using the NULL type as a default return
 		value in your code.
 	* asserts added to casting (Gerry Beauregard)
 	* Added method HasKeys to json::Object which will check if all the keys
 		specified are in the object, returning the index of the first key
 		not found or -1 if all found (hoppe).
 
	1/4/2014
	--------
	* Fixed bug where booleans were being parsed as doubles (Gerry Beauregard).

	1/2/2014 v3
	------------
	* More missing headers added for VisualStudio 2012
	* Switched to snprintf instead of sprintf (or sprintf_s in MSVC)

	1/2/2014 v2
	-----------
	* Added yet more missing headers for compiling on GNU and Linux systems
	* Made Deserialize copy the passed in string so it won't mangle it

	1/2/2014
	--------
	* Fixed previous changelog years. Got ahead of myself and marked them
		as 2014 when they were in fact done in 2013.
	* Added const version of [] to Array/Object/Value
	* Removed C++11 requirements, should work with older compilers
		(thanks to Meng Wang for pointing that out)
	* Made ValueMap and ValueVector typedefs in Object/Value public
		so you can actually iterate over the class
	* Added HasKey and HasValue to Object/Array for convenience
		(note this could have been done comparing .find to .end)

	12/29/2013 v2
	-------------
	* Added .size() field to Value. Returns 1 for non Array/Object types,
		otherwise the number of elements contained.
	* Added .find() to Object to search for a key. Returns Object::end()
		if not found, otherwise the Value.
		Example: bool found = my_obj.find("some key") != my_obj.end();
	* Added .find() to Array to search for a value. Just a convenience
		wrapper for std::find(Array::begin(), Array::end(), Value)
	* Added ==, !=, <, >, <=, >= operators to Object/Array/Value.
		For Objects/Arrays, the operators function just like they do for a
		std::map and std::vector, respectively.
	* Added IsNumeric to Value to indicate if it's an int/float/double type.

	12/29/2013
	----------
	* Added the DoubleVal type which stores, you guessed it, double values.
	* Bug fix for floats with an exact integer value. Now, setting any numerical
		field will also set the fields for the other numerical types. So if you
		have obj["value"] = 12, then the int/float/double cast methods will
		return 12/12.0f/12.0. Previously, in the example above, only the int
		value was set, making a cast to float return 0.
	* Bug fix for deserializing JSON strings that contained large integer values.
		Now if the numerical value of a key in a JSON string contains a number
		less than INT_MIN or greater than INT_MAX it will be stored as a double.
		Note that as mentioned above, all numerical fields are set.
	* Should work fine with scientific notation values now.
	
	12/28/2013
	----------

	* Fixed a bug where if there were spaces around values or key names in a JSON
	string passed in to Deserialize, invalid results or asserts would occur.
	(Fix courtesy of Gerry Beauregard)

	* Added method named "Clear()" to Object/Array/Value to reset state

	* Added license to header file for easyness (totally valid word).
 */

#ifndef __SUPER_EASY_JSON_H__
#define __SUPER_EASY_JSON_H__

#include <vector>
#include <map>
#include <string>
#include <assert.h>

namespace json
{
	enum ValueType
	{
		NULLVal,
		StringVal,
		IntVal,
		FloatVal,
		DoubleVal,
		ObjectVal,
		ArrayVal,
		BoolVal
	};

	class Value;

	class Object
	{
		public:

			typedef std::map<std::string, Value> ValueMap;

		protected:

			ValueMap	mValues;

		public:

			Object();
			Object(const Object& obj);

			Object& operator =(const Object& obj);

			friend bool operator ==(const Object& lhs, const Object& rhs);
			inline friend bool operator !=(const Object& lhs, const Object& rhs) 	{return !(lhs == rhs);}
			friend bool operator <(const Object& lhs, const Object& rhs);
			inline friend bool operator >(const Object& lhs, const Object& rhs) 	{return operator<(rhs, lhs);}
			inline friend bool operator <=(const Object& lhs, const Object& rhs)	{return !operator>(lhs, rhs);}
			inline friend bool operator >=(const Object& lhs, const Object& rhs)	{return !operator<(lhs, rhs);}

			Value& operator [](const std::string& key);
			const Value& operator [](const std::string& key) const;
			Value& operator [](const char* key);
			const Value& operator [](const char* key) const;
		
			ValueMap::const_iterator begin() const;
			ValueMap::const_iterator end() const;
			ValueMap::iterator begin();
			ValueMap::iterator end();

			// Find will return end() if the key can't be found, just like std::map does.
			ValueMap::iterator find(const std::string& key);
			ValueMap::const_iterator find(const std::string& key) const;

			// Convenience wrapper to find to search for a key
			bool HasKey(const std::string& key) const;

			// Checks if the object contains all the keys in the array. If it does, returns -1.
			// If it doesn't, returns the index of the first key it couldn't find.
			int HasKeys(const std::vector<std::string>& keys) const;
			int HasKeys(const char* keys[], int key_count) const;

			// Removes all values and resets the state back to default
			void Clear();

			size_t size() const {return mValues.size();}

	};

	class Array
	{
		public:

			typedef std::vector<Value> ValueVector;

		protected:

			ValueVector				mValues;

		public:

			Array();
			Array(const Array& a);

			Array& operator =(const Array& a);

			friend bool operator ==(const Array& lhs, const Array& rhs);
			inline friend bool operator !=(const Array& lhs, const Array& rhs) {return !(lhs == rhs);}
			friend bool operator <(const Array& lhs, const Array& rhs);
			inline friend bool operator >(const Array& lhs, const Array& rhs) 	{return operator<(rhs, lhs);}
			inline friend bool operator <=(const Array& lhs, const Array& rhs)	{return !operator>(lhs, rhs);}
			inline friend bool operator >=(const Array& lhs, const Array& rhs)	{return !operator<(lhs, rhs);}

			Value& operator[] (size_t i);
			const Value& operator[] (size_t i) const;

			ValueVector::const_iterator begin() const;
			ValueVector::const_iterator end() const;
			ValueVector::iterator begin();
			ValueVector::iterator end();

			// Just a convenience wrapper for doing a std::find(Array::begin(), Array::end(), Value)
			ValueVector::iterator find(const Value& v);
			ValueVector::const_iterator find(const Value& v) const;

			// Convenience wrapper to check if a value is in the array
			bool HasValue(const Value& v) const;

			// Removes all values and resets the state back to default
			void Clear();

			void push_back(const Value& v);
			void insert(size_t index, const Value& v);
			size_t size() const;
	};

	class Value
	{
		protected:

			ValueType						mValueType;
			int								mIntVal;
			float							mFloatVal;
			double 							mDoubleVal;
			std::string						mStringVal;
			Object							mObjectVal;
			Array							mArrayVal;
			bool 							mBoolVal;

		public:

			Value() 					: mValueType(NULLVal), mIntVal(0), mFloatVal(0), mDoubleVal(0), mBoolVal(false) {}
			Value(int v)				: mValueType(IntVal), mIntVal(v), mFloatVal((float)v), mDoubleVal((double)v) {}
			Value(float v)				: mValueType(FloatVal), mFloatVal(v), mIntVal((int)v), mDoubleVal((double)v) {}
			Value(double v)				: mValueType(DoubleVal), mDoubleVal(v), mIntVal((int)v), mFloatVal((float)v) {}
			Value(const std::string& v)	: mValueType(StringVal), mStringVal(v) {}
			Value(const char* v)		: mValueType(StringVal), mStringVal(v) {}
			Value(const Object& v)		: mValueType(ObjectVal), mObjectVal(v) {}
			Value(const Array& v)		: mValueType(ArrayVal), mArrayVal(v) {}
			Value(const bool v)			: mValueType(BoolVal), mBoolVal(v) {}
			Value(const Value& v);

			ValueType GetType() const {return mValueType;}

			Value& operator =(const Value& v);

			friend bool operator ==(const Value& lhs, const Value& rhs);
			inline friend bool operator !=(const Value& lhs, const Value& rhs) 	{return !(lhs == rhs);}
			friend bool operator <(const Value& lhs, const Value& rhs);
			inline friend bool operator >(const Value& lhs, const Value& rhs) 	{return operator<(rhs, lhs);}
			inline friend bool operator <=(const Value& lhs, const Value& rhs)	{return !operator>(lhs, rhs);}
			inline friend bool operator >=(const Value& lhs, const Value& rhs)	{return !operator<(lhs, rhs);}


			// For use with Array/ObjectVal types, respectively
			Value& operator [](size_t idx);
			const Value& operator [](size_t idx) const;
			Value& operator [](const std::string& key);
			const Value& operator [](const std::string& key) const;
			Value& operator [](const char* key);
			const Value& operator [](const char* key) const;
		
			bool 		HasKey(const std::string& key) const;
			int 		HasKeys(const std::vector<std::string>& keys) const;
			int 		HasKeys(const char* keys[], int key_count) const;

		
			// non-operator versions
			int 		ToInt() const		{assert(IsNumeric()); return mIntVal;}
			float 		ToFloat() const		{assert(IsNumeric()); return mFloatVal;}
			double 		ToDouble() const	{assert(IsNumeric()); return mDoubleVal;}
			bool 		ToBool() const		{assert(mValueType == BoolVal); return mBoolVal;}
			std::string	ToString() const	{assert(mValueType == StringVal); return mStringVal;}
			Object 		ToObject() const	{assert(mValueType == ObjectVal); return mObjectVal;}
			Array 		ToArray() const		{assert(mValueType == ArrayVal); return mArrayVal;}
			
			// Please note that as per C++ rules, implicitly casting a Value to a std::string won't work.
			// This is because it could use the int/float/double/bool operators as well. So to assign a
			// Value to a std::string you can either do:
			// 		my_string = (std::string)my_value
			// Or you can now do:
			// 		my_string = my_value.ToString();
			//
			operator int() const 			{assert(IsNumeric()); return mIntVal;}
			operator float() const 			{assert(IsNumeric()); return mFloatVal;}
			operator double() const 		{assert(IsNumeric()); return mDoubleVal;}
			operator bool() const 			{assert(mValueType == BoolVal); return mBoolVal;}
			operator std::string() const 	{assert(mValueType == StringVal); return mStringVal;}
			operator Object() const 		{assert(mValueType == ObjectVal); return mObjectVal;}
			operator Array() const 			{assert(mValueType == ArrayVal); return mArrayVal;}

			bool IsNumeric() const 			{return (mValueType == IntVal) || (mValueType == DoubleVal) || (mValueType == FloatVal);}

			// Returns 1 for anything not an Array/ObjectVal
			size_t size() const;

			// Resets the state back to default, aka NULLVal
			void Clear();

	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Converts a JSON Object or Array instance into a JSON string representing it.
	std::string Serialize(const Value& obj);

	// If there is an error, Value will be NULLType
	Value 		Deserialize(const std::string& str);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	inline bool operator ==(const Object& lhs, const Object& rhs)
	{
		return lhs.mValues == rhs.mValues;
	}

	inline bool operator <(const Object& lhs, const Object& rhs)
	{
		return lhs.mValues < rhs.mValues;
	}

	inline bool operator ==(const Array& lhs, const Array& rhs)
	{
		return lhs.mValues == rhs.mValues;
	}

	inline bool operator <(const Array& lhs, const Array& rhs)
	{
		return lhs.mValues < rhs.mValues;
	}

	/* When comparing different numeric types, this method works the same as if you compared different numeric types
	 on your own. Thus it performs the same as if you, for example, did this:

	 	int a = 1;
	 	float b = 1.1f;
	 	bool equivalent = a == b;

		The same logic applies to the other comparison operators.
	 */
	inline bool operator ==(const Value& lhs, const Value& rhs)
	{
		if ((lhs.mValueType != rhs.mValueType) && !lhs.IsNumeric() && !rhs.IsNumeric())
			return false;

		switch (lhs.mValueType)
		{
			case StringVal		: 	return lhs.mStringVal == rhs.mStringVal;

			case IntVal			: 	if (rhs.GetType() == FloatVal)
										return lhs.mIntVal == rhs.mFloatVal;
									else if (rhs.GetType() == DoubleVal)
										return lhs.mIntVal == rhs.mDoubleVal;
									else if (rhs.GetType() == IntVal)
										return lhs.mIntVal == rhs.mIntVal;
									else
										return false;

			case FloatVal		: 	if (rhs.GetType() == FloatVal)
										return lhs.mFloatVal == rhs.mFloatVal;
									else if (rhs.GetType() == DoubleVal)
										return lhs.mFloatVal == rhs.mDoubleVal;
									else if (rhs.GetType() == IntVal)
										return lhs.mFloatVal == rhs.mIntVal;
									else
										return false;


			case DoubleVal		: 	if (rhs.GetType() == FloatVal)
										return lhs.mDoubleVal == rhs.mFloatVal;
									else if (rhs.GetType() == DoubleVal)
										return lhs.mDoubleVal == rhs.mDoubleVal;
									else if (rhs.GetType() == IntVal)
										return lhs.mDoubleVal == rhs.mIntVal;
									else
										return false;

			case BoolVal		: 	return lhs.mBoolVal == rhs.mBoolVal;

			case ObjectVal		: 	return lhs.mObjectVal == rhs.mObjectVal;

			case ArrayVal		: 	return lhs.mArrayVal == rhs.mArrayVal;

			default:
				return true;
		}
	}

	inline bool operator <(const Value& lhs, const Value& rhs)
	{
		if ((lhs.mValueType != rhs.mValueType) && !lhs.IsNumeric() && !rhs.IsNumeric())
			return false;

		switch (lhs.mValueType)
		{
			case StringVal		: 	return lhs.mStringVal < rhs.mStringVal;

			case IntVal			: 	if (rhs.GetType() == FloatVal)
										return lhs.mIntVal < rhs.mFloatVal;
									else if (rhs.GetType() == DoubleVal)
										return lhs.mIntVal < rhs.mDoubleVal;
									else if (rhs.GetType() == IntVal)
										return lhs.mIntVal < rhs.mIntVal;
									else
										return false;

			case FloatVal		: 	if (rhs.GetType() == FloatVal)
										return lhs.mFloatVal < rhs.mFloatVal;
									else if (rhs.GetType() == DoubleVal)
										return lhs.mFloatVal < rhs.mDoubleVal;
									else if (rhs.GetType() == IntVal)
										return lhs.mFloatVal < rhs.mIntVal;
									else
										return false;

			case DoubleVal		: 	if (rhs.GetType() == FloatVal)
										return lhs.mDoubleVal < rhs.mFloatVal;
									else if (rhs.GetType() == DoubleVal)
										return lhs.mDoubleVal < rhs.mDoubleVal;
									else if (rhs.GetType() == IntVal)
										return lhs.mDoubleVal < rhs.mIntVal;
									else
										return false;

			case BoolVal		: 	return lhs.mBoolVal < rhs.mBoolVal;

			case ObjectVal		: 	return lhs.mObjectVal < rhs.mObjectVal;

			case ArrayVal		: 	return lhs.mArrayVal < rhs.mArrayVal;

			default:
				return true;
		}
	}
}

#endif //__SUPER_EASY_JSON_H__
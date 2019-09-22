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

	8/31/2014:
	---------
	* Fixed bug from last update that broke false/true boolean usage. Courtesy of Vasi B.
	* Change postfix increment of iterators in Serialize to prefix, courtesy of Vasi B.
	* More improvements to validity checking of non string/object/array types. Should
		catch even more invalid usage types such as -1jE5, falsee, trueeeeeee
		{"key" : potato} (that should be {"key" : "potato"}), etc. 
	* Switched to strtol and strtod from atof/atoi in Serialize for better error handling.
	* Fix for GCC order of initialization warnings, courtsey of Vasi B.

	8/17/2014:
	----------
	* Better error handling (and bug fixing) for invalid JSON. Previously, something such as:
			{"def": j[{"a": 100}],"abc": 123}
		would result in at best, a crash, and at worst, nothing when this was passed to 
		the Deserialize method. Note that the "j" is invalid in this example. This led
		to further fixes for other invalid syntax:
		- Use of multiple 'e', for example: 1ee4 is not valid 
		- Use of '.' when not preceded by a digit is invalid. For example: .1 is
			incorrect, but 0.1 is fine.
		- Using 'e' when not preceded by a digit. For example, e4 isn't valid but 1e4 is.

		The deserialize method should properly handle these problems and when there's an
		error, it returns a Value object with the NULLVal type. Check this type to see
		if there's an error.

		Issue reported by Imre Pechan.

	7/21/2014:
	----------
	* All asserts removed and replaced with exceptions, as per request from many users.
		Instead of asserting, functions will throw a std::runtime_error with
		appropriate error message.
	* Added versions of the Value::To* functions that take a default parameter.
		In the event of an error (like calling Value::ToInt() when it's type is an Object),
		the default value you specified will be returned. Courtesy of PeterSvP 
	* Fixed type mismatch warning, courtesy of Per Rovegård
	* Initialized some variables in the various Value constructors to defaults for
		better support with full blast g++ warnings, courtesy of Mark Odell.
	* Changed Value::ToString to return a const std::string& instead of std::string
		to avoid unnecessary copying.
	* Improved some commenting
	* Fixed a bug where a capital E for scientific notation numbers wasn't
		recognized, only lowercase e.
	* VASTLY OVERHAULED AND IMPROVED THE README FILE, PLEASE CONSULT IT FOR
		IN DEPTH USAGE AND EXAMPLES.


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
 	* Deserialize will now return a NULLVal Value instance if there was an
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
#include <stdexcept>


// PLEASE SEE THE README FOR USAGE INFORMATION AND EXAMPLES. Comments will be kept to a minimum to reduce clutter.
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

	// Represents a JSON object which is of the form {string:value, string:value, ...} Where string is the "key" name and is
	// of the form "" or "characters". Value is either of: string, number, object, array, boolean, null
	class Object
	{
		public:

			// This is the type used to store key/value pairs. If you want to get an iterator for this class to iterate over its members,
			// use this. 
			// For example: Object::ValueMap::iterator my_iterator;
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

			// Just like a std::map, you can get the value for a key by using the index operator. You could also
			// use this to insert a value if it doesn't exist, or overwrite it if it does. Example:
			// Value my_val = my_object["some key name"];
			// my_object["some key name"] = "overwriting the value with this new string value";
			// my_object["new key name"] = "a new key being inserted";
			Value& operator [](const std::string& key);
			const Value& operator [](const std::string& key) const;
			Value& operator [](const char* key);
			const Value& operator [](const char* key) const;
		
			ValueMap::const_iterator begin() const;
			ValueMap::const_iterator end() const;
			ValueMap::iterator begin();
			ValueMap::iterator end();

			// Find will return end() if the key can't be found, just like std::map does. ->first will be the key (a std::string),
			// ->second will be the Value.
			ValueMap::iterator find(const std::string& key);
			ValueMap::const_iterator find(const std::string& key) const;

			// Convenience wrapper to search for a key
			bool HasKey(const std::string& key) const;

			// Checks if the object contains all the keys in the array. If it does, returns -1.
			// If it doesn't, returns the index of the first key it couldn't find.
			int HasKeys(const std::vector<std::string>& keys) const;
			int HasKeys(const char* keys[], int key_count) const;

			// Removes all values and resets the state back to default
			void Clear();

			size_t size() const {return mValues.size();}

	};

	// Represents a JSON Array which is of the form [value, value, ...] where value is either of: string, number, object, array, boolean, null
	class Array
	{
		public:

			// This is the type used to store values. If you want to get an iterator for this class to iterate over its members,
			// use this. 
			// For example: Array::ValueVector::iterator my_array_iterator;
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

	// Represents a JSON value which is either of: string, number, object, array, boolean, null
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
			Value(int v)				: mValueType(IntVal), mIntVal(v), mFloatVal((float)v), mDoubleVal((double)v), mBoolVal(false) {}
			Value(float v)				: mValueType(FloatVal), mIntVal((int)v), mFloatVal(v), mDoubleVal((double)v), mBoolVal(false) {}
			Value(double v)				: mValueType(DoubleVal), mIntVal((int)v), mFloatVal((float)v), mDoubleVal(v), mBoolVal(false) {}
			Value(const std::string& v) : mValueType(StringVal), mIntVal(), mFloatVal(), mDoubleVal(), mStringVal(v), mBoolVal(false) {}
			Value(const char* v)		: mValueType(StringVal), mIntVal(), mFloatVal(), mDoubleVal(), mStringVal(v), mBoolVal(false) {}
			Value(const Object& v)		: mValueType(ObjectVal), mIntVal(), mFloatVal(), mDoubleVal(), mObjectVal(v), mBoolVal(false) {}
			Value(const Array& v)		: mValueType(ArrayVal), mIntVal(), mFloatVal(), mDoubleVal(), mArrayVal(v), mBoolVal(false) {}
			Value(bool v)				: mValueType(BoolVal), mIntVal(), mFloatVal(), mDoubleVal(), mBoolVal(v) {}
			Value(const Value& v);

			// Use this to determine the underlying type that this Value class represents. It will be one of the
			// ValueType enums as defined at the top of this file.
			ValueType GetType() const {return mValueType;}

			// Convenience method that checks if this type is an int/double/float
			bool IsNumeric() const 			{return (mValueType == IntVal) || (mValueType == DoubleVal) || (mValueType == FloatVal);}

			Value& operator =(const Value& v);

			friend bool operator ==(const Value& lhs, const Value& rhs);
			inline friend bool operator !=(const Value& lhs, const Value& rhs) 	{return !(lhs == rhs);}
			friend bool operator <(const Value& lhs, const Value& rhs);
			inline friend bool operator >(const Value& lhs, const Value& rhs) 	{return operator<(rhs, lhs);}
			inline friend bool operator <=(const Value& lhs, const Value& rhs)	{return !operator>(lhs, rhs);}
			inline friend bool operator >=(const Value& lhs, const Value& rhs)	{return !operator<(lhs, rhs);}


			// If this value represents an object or array, you can use the [] indexing operator
			// just like you would with the native json::Array or json::Object classes. 
			// THROWS A std::runtime_error IF NOT AN ARRAY OR OBJECT.
			Value& operator [](size_t idx);
			const Value& operator [](size_t idx) const;
			Value& operator [](const std::string& key);
			const Value& operator [](const std::string& key) const;
			Value& operator [](const char* key);
			const Value& operator [](const char* key) const;
		
			// If this value represents an object, these methods let you check if a single key or an array of
			// keys is contained within it. 
			// THROWS A std::runtime_error IF NOT AN OBJECT.
			bool 		HasKey(const std::string& key) const;
			int 		HasKeys(const std::vector<std::string>& keys) const;
			int 		HasKeys(const char* keys[], int key_count) const;

		
			// non-operator versions, **will throw a std::runtime_error if invalid with an appropriate error message**
			int 				ToInt() const;
			float 				ToFloat() const;
			double 				ToDouble() const;
			bool 				ToBool() const;
			const std::string&	ToString() const;
			Object 				ToObject() const;
			Array 				ToArray() const;

			// These versions do the same as above but will return your specified default value in the event there's an error, and thus **don't** throw an exception.
			int					ToInt(int def) const					{return IsNumeric() ? mIntVal : def;}
			float				ToFloat(float def) const				{return IsNumeric() ? mFloatVal : def;}
			double				ToDouble(double def) const				{return IsNumeric() ? mDoubleVal : def;}
			bool				ToBool(bool def) const					{return (mValueType == BoolVal) ? mBoolVal : def;}
			const std::string&	ToString(const std::string& def) const	{return (mValueType == StringVal) ? mStringVal : def;}

			
			// Please note that as per C++ rules, implicitly casting a Value to a std::string won't work.
			// This is because it could use the int/float/double/bool operators as well. So to assign a
			// Value to a std::string you can either do:
			// 		my_string = (std::string)my_value
			// Or you can now do:
			// 		my_string = my_value.ToString();
			//
			operator int() const;
			operator float() const;
			operator double() const;
			operator bool() const;
			operator std::string() const;
			operator Object() const;
			operator Array() const;			

			// Returns 1 for anything not an Array/ObjectVal
			size_t size() const;

			// Resets the state back to default, aka NULLVal
			void Clear();

	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Converts a JSON Object or Array instance into a JSON string representing it. RETURNS EMPTY STRING ON ERROR. 
	// As per JSON specification, a JSON data structure must be an array or an object. Thus, you must either pass in a
	// json::Array, json::Object, or a json::Value that has an Array or Object as its underlying type. 
	std::string Serialize(const Value& obj);

	// If there is an error, Value will be NULLVal. Pass in a valid JSON string (such as one returned from Serialize, or obtained
	// elsewhere) to receive a Value in return that represents the JSON structure. Check the type of Value by calling GetType().
	// It will be ObjectVal or ArrayVal (or NULLVal if invalid JSON). The Value class contains the operator [] for indexing in the
	// case that the underlying type is an object or array. You may, if you prefer, create an object or array from the Value returned
	// by this method by simply passing it into the constructor.
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

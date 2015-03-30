/**
 * Construct a new type from an array, object or function.
 * Use instances of this class to build a schema graph.
 * The schema graph must not contain data other than arrays and instances of this class.
 * Use the check function to check if arbitrary JS objects matches the schema.
 * @constructor
 * @param {String} name - name for the new type
 * @param obj - array, object or function
 * array: array should contain one instance of class "Type"
 * object: object members can be arrays or instances of class "Type".
 *         Can also have child objects, but the leaf members have to be instances of class "Type"
 * function: a function which takes three parameters: log, other, stack
 *           must return true if other matches the type, can report errors using the function passed in log.
 */
function Type(name, obj)
{
	//var dbg = function(str){console.log(str);};
	var dbg = function(str){};

	this.getName = function(){
		return name;
	}
	this.isLeaf = function(){
		return typeof(obj) === "function";
	}
	this.getObj = function(){
		return obj;
	}
	this.check = function(log, other, stack)
	{
		if(typeof(obj) === "object")
		{
			stack.push("<"+name+">");
			var ok = _check(log, obj, other, stack);
			stack.pop;
			return ok;
		}
		if(typeof(obj) === "function")
			return obj(log, other, stack);
		log("FATAL Error: wrong usage of new Type(), second parameter should be an object or checker function");
		return false;
	}
	function _check(log, ref, other, stack)
	{
		dbg("_check");
		dbg("ref=" + ref);
		dbg("other=" + other);
		dbg("stack=[" + stack + "]");
		if(ref instanceof Array)
		{
			dbg("is instanceof Array");
			if(other instanceof Array)
			{
				if(other.length > 0)
				{
					return _check(log, ref[0], other[0], stack);
				}
				else
				{
					log("Warning: can't check array of length 0 in ["+stack+"]");
					return true;
				}
			}
			else
			{
				log("Error: not an Array ["+stack+"]");
				return false;
			}
		}
		if(ref instanceof Type)
		{
			dbg("is instanceof Type");
			return ref.check(log, other, stack);
		}
		else // Object, have to check all children
		{
			dbg("is Object");
			var ok = true;
			for(m in ref)
			{
				if(m in other)
				{
					stack.push(m);
					ok = ok && _check(log, ref[m], other[m], stack);
					stack.pop();
				}
				else
				{
					log("Error: missing member \""+m+"\" in ["+stack+"]");
					ok = false;
				}
			}
			// check for additionally undocumented members
			for(m in other)
			{
				if(!(m in ref))
				{
					log("Warning: found additional member \""+m+"\" in ["+stack+"]");
				}
			}
			return ok;
		}
		
	}
};

// basic data types
// - string
// - bool
// - any (placeholder for unknown type)

var string = new Type("string",
	function(log, other, stack)
	{
		if(typeof(other) !== "string")
		{
			log("Error: not a string ["+stack+"]");
			return false;
		}
		else
			return true;
	}
);
var bool = new Type("bool",
	function(log, other, stack)
	{
		if(typeof(other) !== "boolean")
		{
			log("Error: not a bool ["+stack+"]");
			return false;
		}
		else
			return true;
	}
);
var any = new Type("any",
	function(log, other, stack)
	{
		return true;
	}
);

exports.Type = Type;
exports.string = string;
exports.bool = bool;
exports.any = any;
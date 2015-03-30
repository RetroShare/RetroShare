var TypesMod = require("./Types.js");
var Type = TypesMod.Type;
var string = TypesMod.string;
var bool = TypesMod.bool;
var any = TypesMod.any;

if(require.main === module)
{
	var RsNodeHttpConnection = require("./RsNodeHttpConnection.js");
	debugger;
	var connection = new RsNodeHttpConnection();
	var RsApi = require("./RsApi.js");
	var RS = new RsApi(connection);

	var tests = [];
	PeersTest(tests);

	tests.map(function(test){
		test(RS);
	});
}

function PeersTest(tests, doc)
{
	// compound types
	var location = new Type("location",
	{
		avatar_address: string,
		groups: 	any,
		is_online: 	bool,
		location: 	string,
		peer_id: 	any,
	});
	var peer_info = new Type("peer_info",
	{
		name: 	string,
		pgp_id: any,
		locations: [location],
	});
	var peers_list = new Type("peers_list",[peer_info]);

	tests.push(function(RS){
		console.log("testing peers module...");
		console.log("expected schema is:")
		console.log(graphToText(peers_list));
		RS.request({path: "peers"}, function(resp){
			//console.log("got response:"+JSON.stringify(resp));
			var ok = peers_list.check(function(str){console.log(str);}, resp.data, [])
			if(ok)
				console.log("success");
			else
				console.log("fail");
		});
	});

	function graphToText(top_node)
	{
		//var dbg = function(str){console.log(str);};
		var dbg = function(str){};

		dbg("called graphToText with " + top_node);

		var res = "";

		_visit(top_node.getObj(), 0);

		return res;

		function _indent(count)
		{
			var str = "";
			for(var i = 0; i < count; i++)
			{
				str = str + "    ";
			}
			return str;
		}
		function _visit(node, indent)
		{
			dbg("_visit");
			if(node instanceof Array)
			{
				dbg("is instanceof Array");
				//res = res + "[";
				res = res + "array\n";
				_visit(node[0], indent);
				//res = res + _indent(indent) + "]\n";
			}
			else if(node instanceof Type && node.isLeaf())
			{
				dbg("is instanceof Type");
				res = res + node.getName() + "\n";
			}
			else // Object, have to check all children
			{
				dbg("is Object");
				//res = res + "{\n";
				for(m in node.getObj())
				{
					res = res + _indent(indent+1) + m + ": ";
					_visit(node.getObj()[m], indent+1);
				}
				//res = res + _indent(indent) + "}\n";
			}
			
		}
	}
}



// ************   below is OLD stuff, to be removed ************
	var Location =
	{
		avatar_address: String(),
		groups: undefined,
		is_online: Boolean(),
		location: String(),
		name: String(),
		peer_id: undefined,
		pgp_id: undefined,
	};
	var PeerInfo = 
	{
		name: String(),
		locations: [Location],
	};
	var PeersList = [PeerInfo];

	function checkIfMatch(ref, other)
	{
		var ok = true;

		// sets ok to false on error
		function check(subref, subother, path)
		{
			//console.log("checking");
			//console.log("path: " + path);
			//console.log("subref: " +subref);
			//console.log("subother: "+subother);
			if(subref instanceof Array)
			{
				//console.log("is Array: " + path);
				if(!(subother instanceof Array))
				{
					ok = false;
					console.log("Error: not an Array " + path);
					return;
				}
				if(subother.length == 0)
				{
					console.log("Warning: can't check Array of lentgh 0 " + path);
					return;
				}
				// check first array member
				check(subref[0], subother[0], path);
				return;
			}
			// else compare as dict
			for(m in subref)
			{
				if(!(m in subother))
				{
					ok = false;
					console.log("Error: missing member \"" + m + "\" in "+ path);
					continue;
				}
				if(subref[m] === undefined)
				{
					// undefined = don't care what it is
					continue;
				}
				if(typeof(subref[m]) == typeof(subother[m]))
				{
					if(typeof(subref[m]) == "object")
					{
						// make deep object inspection
						path.push(m);
						check(subref[m], subother[m], path);
						path.pop();
					}
					// else everthing is fine
				}
				else
				{
					ok = false;
					console.log("Error: member \"" + m + "\" has wrong type in "+ path);
				}
			}
			// TODO: check for additional members and print notice
		}

		check(ref, other, []);

		return ok;
	}

	function stringifyTypes(obj)
	{
		if(obj instanceof Array)
		{
			return [stringifyTypes(obj[0])];
		}
		var ret = {};
		for(m in obj)
		{
			if(typeof(obj[m]) === "object")
				ret[m] = stringifyTypes(obj[m]);
			else
				ret[m] = typeof obj[m];
		}
		return ret;
	}

	// trick to get multiline string constants: use comment as string constant
	var input = function(){/*
[{
		"locations": [{
			"avatar_address": "/5cfed435ebc24d2d0842f50c6443ec76/avatar_image",
			"groups": null,
			"is_online": true,
			"location": "",
			"name": "se2",
			"peer_id": "5cfed435ebc24d2d0842f50c6443ec76",
			"pgp_id": "985CAD914B19A212"
		}],
		"name": "se2"
	}]
*/}.toString().slice(14,-3);

// **************** end of old stuff ***************************
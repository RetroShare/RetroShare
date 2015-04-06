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
	var doc = {
		counter: 0,
		toc: [],
		content: [],
		header: function(h){
			this.toc.push(h);
			this.content.push("<a name=\""+this.counter+"\"><h1>"+h+"</h1></a>");
			this.counter += 1;
		},
		paragraph: function(p){
			this.content.push("<p>"+p+"</p>");
		},

	};
	PeersTest(tests, doc);

	var docstr = "<!DOCTYPE html><html><body>";
	docstr += "<h1>Table of Contents</h1>";
	docstr += "<ul>";
	for(var i in doc.toc)
	{
		docstr += "<li><a href=\"#"+i+"\">"+doc.toc[i]+"</a></li>";
	}
	docstr += "</ul>";
	for(var i in doc.content)
	{
		docstr += doc.content[i];
	}
	docstr += "</body></html>";

	var fs = require('fs');
	fs.writeFile("dist/api_documentation.html", docstr);

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

	doc.header("peers");
	doc.paragraph("<pre>"+graphToText(peers_list)+"</pre>");

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

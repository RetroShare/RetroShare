var http = require('http');

/**
 * Connection to the RS backend using http for running under node.js
 * Mainly for testing, but could also use it for general purpose scripting.
 * @constructor
 */
module.exports = function()
{
	var server_hostname = "localhost";
	var server_port = "9090";
	var api_root_path = "/api/v2/";

	this.request = function(request, callback)
	{
		var data;
		if(request.data)
			data = JSON.stringify(request.data);
		else
			data = "";

		// NODEJS specific
		var req = http.request({
			host: server_hostname,
			port: server_port,
			path: api_root_path + request.path,
			headers: {
				"Content-Type": "application/json",
				"Content-Length": data.length, // content length is required, else Wt will not provide the data (maybe WT does not like chunked encoding?)
			}
			//method: "POST",
		}, function(response){
			var databuffer = [];
			response.on("data", function(chunk){
				//console.log("got some data");
				databuffer = databuffer + chunk;
			})
			response.on("end", function(){
				//console.log("finished receiving data");
				//console.log("data:"+databuffer);
				callback(JSON.parse(databuffer));
			})
			
		});

		//console.log("uploading data:");
		//console.log(data);
		req.write(data);
		req.end();
	}
}
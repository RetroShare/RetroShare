New programming interface for Retroshare
========================================

* access to libretroshare for webinterfaces, QML and scripting
* client - server architecture.
* network friendly: transport messages over high latency and low bandwidth networks
* multiple clients: can use scripting and webinterface at the same time
* simple packet format: no special serialiser required
* simple protocol: one request creates one response. A requets does not depend on a previous request.
* automatic state change propagation: if a resource on the server changes, the clients will get notified
* no shared state: Client and server don't have to track what they send each other.
* works with all programming languages

How does it work?
-----------------

	- Client sends a request: adress of a resource and optional parameters encoded as JSON
	{
		"method": "get",
		"resource": ["peers"],
	}
	- Server sends a Response:
	{
		"returncode": "ok",
		"statetoken": "ASDF",
		"data": [...]
	}

	- Client asks if data is still valid
	{
		"method": "exec",
		"resource": "statetokenservice"
		"data": ["ASDF", "GHJK"]
	}
	- Server answers Client that statetoken "ASDF" expired
	{
		"returncode": "ok",
		"data": ["ASDF"]
	}

Transport
---------

A transport protocol transports requests and responses between client and server.

* tracks message boundaries, so messages don't get merged
* may be able to handle concurrent requests with out of order delivery of responses
* knows to which request a response belongs to

Transport could do encryption and authentication with a standardized protocol like SSH or TLS.

Ideas:

* request id + length + request data -> SSH -> TCP -> ...
* Websockets
* Retroshare Items
* Unix domain sockets

Currently only unencrypted http is implemented. libmicrohttpd (short MHD) is used as http server.
Can use a proxy to add TLS encryption.

Message encoding
----------------

Currently JSON, because it is already available in JavaScript and QML.
Other key-value encodings could be used as well.
Read more about basic data types of different languages (JavaScript, QML, Lua, C++) in ./ApiServer.cpp.

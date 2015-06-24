var connection = new RsXHRConnection(window.location.hostname, window.location.port);
var RS = new RsApi(connection);
RS.start();

var api_url = window.location.protocol + "//" + window.location.hostname + ":" + window.location.port + "/api/v2/";
var filestreamer_url = window.location.protocol + "//" +window.location.hostname + ":" + window.location.port + "/fstream/";
var upload_url = window.location.protocol + "//" +window.location.hostname + ":" + window.location.port + "/upload/";

// livereload
function start_live_reload()
{
	RS.request({path: "livereload"}, function(resp){
		RS.register_token_listener(function(){
			// Reload the current page, without using the cache
			document.location.reload(true);
		},resp.statetoken);
	});
}
start_live_reload();

// displays a notice if requests to the server failed
var ConnectionStatusWidget = React.createClass({
	// react component lifecycle callbacks
	getInitialState: function(){
		return {status: "unknown"};
	},
	componentWillMount: function()
	{
		connection.register_status_listener(this.onConnectionStatusChanged);
	},
	componentWillUnmount: function()
	{
		connection.unregister_status_listener(this.onConnectionStatusChanged);
	},
	onConnectionStatusChanged: function(state)
	{
		this.setState({status: state});
	},
	render: function()
	{
		if(this.state.status === "unknown")
		{
			return <div>connecting to server...</div>;
		}
		if(this.state.status === "connected")
		{
			return <div></div>;
		}
		if(this.state.status === "not_connected")
		{
			return <div>You are not connected to the server anymore. Is the Network connection ok? Is the server up?</div>;
		}
		return <div>Error: unexpected status value in ConnectionStatusWidget. Please Report this. this.state.status={this.state.status}</div>;
	},
});

// implements automatic update using the state token system
// components using this mixin should have a member "getPath()" to specify the resource
var AutoUpdateMixin = 
{
	// react component lifecycle callbacks
	componentDidMount: function()
	{
		this._aum_debug("AutoUpdateMixin did mount path="+this.getPath());
		this._aum_on_data_changed();
	},
	componentWillUnmount: function()
	{
		this._aum_debug("AutoUpdateMixin will unmount path="+this.getPath());
		RS.unregister_token_listener(this._aum_on_data_changed);
	},

	// private auto update mixin methods
	_aum_debug: function(msg)
	{
		//console.log(msg);
	},
	_aum_on_data_changed: function()
	{
		RS.request({path: this.getPath()}, this._aum_response_callback);
	},
	_aum_response_callback: function(resp)
	{
		this._aum_debug("Mixin received data: "+JSON.stringify(resp));
		// it is impossible to update the state of an unmounted component
		// but it may happen that the component is unmounted before a request finishes
		// if response is too late, we drop it
		if(!this.isMounted())
		{
			this._aum_debug("AutoUpdateMixin: component not mounted. Discarding response. path="+this.getPath());
			return;
		}
		var state = this.state;
		state.data = resp.data;
		this.setState(state);
		RS.unregister_token_listener(this._aum_on_data_changed);
		RS.register_token_listener(this._aum_on_data_changed, resp.statetoken);
	},
};

// the signlaSlotServer decouples event senders from event receivers
// senders just send their events
// the server will forwards them to all receivers
// receivers have to register/unregister at the server
var signalSlotServer =
{
	clients: [],
	/**
	 * Register a client which wants to participate
	 * in the signal slot system. Clients must provide a function
	 * onSignal(signal_name, parameters)
	 * where signal_name is a string and parameters and array or object
	 */
	registerClient: function(client)
    {
        this.clients.push(client);
    },
    /**
     * Unregister a previously registered client.
     */
    unregisterClient : function(client)
    {
    	var to_delete = [];
    	var clients = this.clients;
        for(var i=0; i<clients.length; i++){
            if(clients[i] === client){
                to_delete.push(i);
            }
        }
        for(var i=0; i<to_delete.length; i++){
            // copy the last element to the current index
            var index = to_delete[i];
            clients[index] = clients[clients.length-1];
            // remove last element
            clients.pop();
        }
    },
    /**
     * Emit the signale given by its name, with the optional parameters in params.
     */
    emitSignal: function(signal_name, parameters)
    {
    	var clients = this.clients;
        for(var i=0; i<clients.length; i++){
            clients[i].onSignal(signal_name, parameters);
        }
    },
}

var SignalSlotMixin = 
{
	conected_signals: [],
	componentDidMount: function()
	{
		signalSlotServer.registerClient(this);
	},
	componentWillUnmount: function()
	{
		signalSlotServer.unregisterClient(this);
	},

	/**
	 * emit a signal
	 */
	emit: function(signal_name, parameters)
	{
		signalSlotServer.emitSignal(signal_name, parameters);
	},
	/**
	 * connect the callback to the signal
	 * the connection is automatically destroyed when this component gets unmounted
	 */
	connect: function(signal_name, callback)
	{
		this.conected_signals.push({name: signal_name, callback: callback});
	},

	/**
	 * callback for signal server
	 */
	onSignal: function(signal_name, parameters)
	{
        for(var i=0; i<this.conected_signals.length; i++){
            if(this.conected_signals[i].name === signal_name){
                this.conected_signals[i].callback(parameters);
            }
        }
	},
};

// url hash handling
// - allows the backwards/forward button to work
// - restores same view for same url
// dataflow:
// emit change_url event -> event handler sets browser url hash
// -> browser sends hash changed event -> emit url_changed event
var prev_url = "";
function hashChanged(){
	var url = window.location.hash.slice(1); // remove #
	// prevent double work
	//if(url !== prev_url)
	if(true)
	{
		signalSlotServer.emitSignal("url_changed", {url: url});
		prev_url = url;
	}
}
// Listen on hash change:
window.addEventListener('hashchange', hashChanged);  
// Listen on page load:
// this does not work, because the components are not always mounted when the event fires
window.addEventListener('load', hashChanged);  

var changeUrlListener = {
	onSignal: function(signal_name, parameters){
		if(signal_name === "change_url")
		{
			console.log("changeUrlListener: change url to "+parameters.url);
			window.location.hash = parameters.url;
			// some browsers dont send an event, so trigger event by hand
			// the history does not work then
			hashChanged();
		}
	},
};
signalSlotServer.registerClient(changeUrlListener);

var Peers2 = React.createClass({
	mixins: [AutoUpdateMixin, SignalSlotMixin],
	getPath: function(){return "peers";},
	getInitialState: function(){
		return {data: []};
	},
	add_friend_handler: function(){
		this.emit("change_url", {url: "add_friend"});
	},
	render: function(){
		var component = this;
		var Peer = React.createClass({
			remove_peer_handler: function(){
				var yes = window.confirm("Remove "+this.props.data.name+" from friendslist?");
				if(yes){
					RS.request({path: component.getPath()+"/"+this.props.data.pgp_id+"/delete"});
				}
			},
			render: function(){
				var locations = this.props.data.locations.map(function(loc){
					var online_style = {
						width: "1em",
						height: "1em",
						borderRadius: "0.5em",

						backgroundColor: "grey",
					};
					if(loc.is_online)
						online_style.backgroundColor = "lime";
					return(<li key={loc.peer_id}>{loc.location} <div style={online_style}></div></li>);
				});
				// TODO: fix the url, should get the "../api/v2" prefix from a single variable
				var avatar_url = "";
				if(this.props.data.locations.length > 0 && this.props.data.locations[0].avatar_address !== "")
					avatar_url = api_url + component.getPath() + this.props.data.locations[0].avatar_address
				var remove_button_style = {
					color: "red",
					fontSize: "1.5em",
					padding: "0.2em",
					cursor: "pointer",
				};
				var remove_button = <div onClick={this.remove_peer_handler} style={remove_button_style}>X</div>;
				return(<tr><td><img src={avatar_url}/></td><td>{this.props.data.name}</td><td><ul>{locations}</ul></td><td>{remove_button}</td></tr>);
			}
		});
		return (
		<div>
			{/* span reduces width to only the text length, div does not */}
			<div onClick={this.add_friend_handler} className="btn2">&#43; add friend</div>
			<table>
				<tr><th>avatar</th><th> name </th><th> locations</th><th></th></tr>
				{this.state.data.map(function(peer){ return <Peer key={peer.name} data={peer}/>; })}
			</table>
		</div>);
	},
});

var Peers3 = React.createClass({
	mixins: [AutoUpdateMixin, SignalSlotMixin],
	getPath: function(){return "peers";},
	getInitialState: function(){
		return {data: []};
	},
	add_friend_handler: function(){
		this.emit("change_url", {url: "add_friend"});
	},
	render: function(){
		var component = this;
		var Peer = React.createClass({
			remove_peer_handler: function(){
				var yes = window.confirm("Remove "+this.props.data.name+" from friendslist?");
				if(yes){
					RS.request({path: component.getPath()+"/"+this.props.data.pgp_id+"/delete"});
				}
			},
			render: function(){
				var locations = this.props.data.locations.map(function(loc){
					var online_style = {
						width: "1em",
						height: "1em",
						borderRadius: "0.5em",
						backgroundColor: "grey",
						float:"left",
					};
					if(loc.is_online)
						online_style.backgroundColor = "lime";
					return(<div key={loc.peer_id} style={{color: loc.is_online? "lime": "grey"}}>{/*<div style={online_style}></div>*/}{loc.location}</div>);
				});
				var avatars = this.props.data.locations.map(function(loc){
					if(loc.is_online && (loc.avatar_address !== ""))
					{
						var avatar_url = api_url + component.getPath() + loc.avatar_address;
						return <img style={{borderRadius: "3mm", margin: "2mm"}} src={avatar_url}/>;
					}
					else return <span></span>;
				});
				var remove_button_style = {
					color: "red",
					//fontSize: "1.5em",
					fontSize: "10mm",
					padding: "0.2em",
					cursor: "pointer",
				};
				var remove_button = <div onClick={this.remove_peer_handler} style={remove_button_style}>X</div>;
				var is_online = false;
				for(var i in this.props.data.locations)
				{
					if(this.props.data.locations[i].is_online)
						is_online = true;
				}
				return(
				<div className="flexbox" style={{color: is_online? "lime": "grey"}}>
					<div>{avatars}</div>
					<div className="flexwidemember">
						<h1 style={{marginBottom: "1mm"}}>{this.props.data.name}</h1>
						<div>{locations}</div>
					</div>
					{remove_button}
				</div>
				);
			}
		});
		return (
		<div>
			{/* span reduces width to only the text length, div does not */}
			<div onClick={this.add_friend_handler} className="btn2">&#43; add friend</div>
			{this.state.data.map(function(peer){ return <Peer key={peer.pgp_id} data={peer}/>; })}
		</div>);
	},
});

var OwnCert = React.createClass({
	mixins: [AutoUpdateMixin, SignalSlotMixin],
	getPath: function(){return "peers/self/certificate";},
	getInitialState: function(){
		return {data: {cert_string: ""}};
	},
	render: function(){
		// use <pre> tag for correct new line behavior!
		return (
		<pre>
			{this.state.data.cert_string}
		</pre>);
	},
});

var AddPeerWidget = React.createClass({
	getInitialState: function(){
		return {page: "start"};

		// for testing
		//return {page: "peer", data: {name: "some_test_name"}};
	},
	add_friend_handler: function(){
		var cert_string = this.refs.cert.getDOMNode().value;
		if(cert_string != null){
			// global replae all carriage return, because rs does not like them in certstrings
			//cert_string = cert_string.replace(/\r/gm, "");
			RS.request({path: "peers/examine_cert", data: {cert_string: cert_string}}, this.examine_cert_callback);
			this.setState({page:"waiting", cert_string: cert_string});
		}
	},
	examine_cert_callback: function(resp){
		this.setState({page: "peer", data: resp.data});
	},
	final_add_handler: function(){
		this.setState({page: "start"});
		RS.request(
		{
			path: "peers",
			data: {
				cert_string: this.state.cert_string,
				flags:{
					allow_direct_download: this.refs.cb_direct_dl.getDOMNode().checked,
					allow_push: this.refs.cb_push.getDOMNode().checked,
					// set to false, until the webinterface supports managment of the blacklist/whitelist
					require_whitelist: false,
				}
			}
		});
	},
	render: function(){
		if(this.state.page === "start")
			return(
				<div>
					<p>Your own key, give it to your friends</p>
					<OwnCert/>
					<p>paste your friends key below</p>
			        <textarea ref="cert" cols="70" rows="16"></textarea><br/>
			        <input
			          type="button"
			          value="read key"
			          onClick={this.add_friend_handler}
			        />
				</div>
			);
		if(this.state.page === "waiting")
			return(
				<div>
			        waiting for response from server...
				</div>
			);
		if(this.state.page === "peer")
			return(
				<div>
			        <p>Do you want to add {this.state.data.name} to your friendslist?</p>
			        <input className="checkbox" type="checkbox" ref="cb_direct_dl"/> Allow direct downloads from this node<br/>
			        <input className="checkbox" type="checkbox" ref="cb_push"/> Auto download recommended files from this node<br/>
			        <div onClick={this.final_add_handler} className="btn2">add to friendslist</div>
				</div>
			);
	},
});

var ProgressBar = React.createClass({
	render: function(){
		return(
		<div style={{
			borderStyle: "solid",
			borderColor: "lime",
			borderRadius: "3mm",
			padding: "2mm",
			height: "10mm",
		}}>
			<div style={{backgroundColor: "lime", height: "100%", width: (this.props.progress*100)+"%",}}></div>
		</div>);
	}
});

function makeFriendlyUnit(bytes)
{
	if(bytes < 1e3)
		return bytes.toFixed(1) + "B";
	if(bytes < 1e6)
		return (bytes/1e3).toFixed(1) + "kB";
	if(bytes < 1e9)
		return (bytes/1e6).toFixed(1) + "MB";
	if(bytes < 1e12)
		return (bytes/1e9).toFixed(1) + "GB";
	return (bytes/1e12).toFixed(1) + "TB";
}


var DownloadsWidget = React.createClass({
	mixins: [AutoUpdateMixin, SignalSlotMixin],
	getPath: function(){ return "transfers/downloads";},
	getInitialState: function(){
		return {data: []};
	},
	render: function(){
		var widget = this;
		var DL = React.createClass({
			render: function()
			{
				var file = this.props.data;
				var startFn = function(){
					RS.request({
						path: "transfers/control_download",
						data: {
							action: "start",
							id: file.id,
						}
					}, function(){
						console.log("start dl callback");
					}
				)};
				var pauseFn = function(){
					RS.request({
						path: "transfers/control_download",
						data: {
							action: "pause",
							id: file.id,
						}
					}, function(){
						console.log("pause dl callback");
					}
				)};
				var cancelFn = function(){
					RS.request({
						path: "transfers/control_download",
						data: {
							action: "cancel",
							id: file.id,
						}
					}, function(){
						console.log("cancel dl callback");
					}
				)};
				var playFn = function(){
					widget.emit("play_file", {name: file.name, hash: file.hash})
				};
				var playBtn = <div></div>;
				if(file.name.slice(-3) === "mp3")
					playBtn = <div className="btn" onClick={playFn}>play</div>;

				var ctrlBtn = <div></div>;
				if(file.download_status==="paused")
				{
					ctrlBtn = <div className="btn" onClick={startFn}>start</div>;
				}
				else
				{
					ctrlBtn = <div className="btn" onClick={pauseFn}>pause</div>;
				}
				return(<tr>
					<td>{this.props.data.name}</td>
					<td>{makeFriendlyUnit(this.props.data.size)}</td>
					<td><ProgressBar progress={this.props.data.transfered / this.props.data.size}/></td>
					<td>{makeFriendlyUnit(this.props.data.transfer_rate*1e3)}/s</td>
					<td>{this.props.data.download_status}</td>
					<td>{ctrlBtn} <div className="btn" onClick={cancelFn}>cancel</div> {playBtn}</td>
					</tr>);
			}
		});
		return (
		<table>
			<tr>
				<th>name</th>
				<th>size</th>
				<th>completed</th>
				<th>transfer rate</th>
				<th>download status</th>
				<th>actions</th>
			</tr>
			{this.state.data.map(function(dl){ return <DL key={dl.hash} data={dl}/>; })}
		</table>);
	},
});

var SearchWidget = React.createClass({
	getInitialState: function(){
		return {search_id: undefined};
	},
	handleSearch: function(){
		console.log("searching for: "+this.refs.searchbox.getDOMNode().value);
		var search_string = this.refs.searchbox.getDOMNode().value;
		RS.request({path: "filesearch/create_search", data:{distant: true, search_string: search_string}}, this.onCreateSearchResponse);
	},
	onCreateSearchResponse: function(resp){
		if(this.isMounted()){
			this.setState({search_id: resp.data.search_id});
		}
	},
	render: function(){
		var ResultList = React.createClass({
			mixins: [AutoUpdateMixin],
			getPath: function(){ return "filesearch/"+this.props.id; },
			getInitialState: function(){
				return {data: []};
			},
			start_download: function(fileinfo){
				RS.request({
					path: "transfers/control_download",
					data:{
						action: "begin",
						name: fileinfo.name,
						size: fileinfo.size,
						hash: fileinfo.hash,
					},
				});
			},
			render: function(){
				var c2 = this;
				var File = React.createClass({
					render: function(){
						var file = this.props.data;
						return(
						<tr>
							<td>{file.name}</td>
							<td>{file.size}</td>
							<td><span onClick={function(){c2.start_download(file);}} className="btn">download</span></td>
						</tr>);
					},
				});
				return (
				<table>
					<tr><th>name</th><th>size</th><th></th></tr>
					{this.state.data.map(
						function(file){
							return <File key={file.id} data={file}/>;
						}
					)}
				</table>);
			}
		});

		var results = <div></div>;
		if(this.state.search_id !== undefined)
		{
			results = <ResultList id={this.state.search_id} />;
		}
		return(
		<div>
			<p>turtle file search</p>
		   	<div>
		        <input type="text" ref="searchbox" />
		        <input
		          type="button"
		          value="search"
		          onClick={this.handleSearch}
		        />
	      	</div>
	      	{results}
		</div>);
	},
});

var AudioPlayerWidget = React.createClass({
	mixins: [SignalSlotMixin],
	getInitialState: function(){
		return {file: undefined};
	},
	componentWillMount: function(){
		this.connect("play_file", this.play_file);
	},
	play_file: function(file){
		this.setState({file: file});
	},
	render: function(){
		if(this.state.file === undefined)
		{
			return(<div></div>);
		}
		else
		{
			return(
				<div>
					<p>{this.state.file.name}</p>
					<audio controls src={filestreamer_url+this.state.file.hash} type="audio/mpeg">
					</audio>
				</div>
			);
		}
	},
});

var PasswordWidget =  React.createClass({
	mixins: [AutoUpdateMixin],
	getInitialState: function(){
		return {data: {want_password: false}};
	},
	getPath: function(){
		return "control/password";
	},
	sendPassword: function(){
		RS.request({path: "control/password", data:{password: this.refs.password.getDOMNode().value}})
	},
	render: function(){
		if(this.state.data.want_password === false)
		{
			return <div></div>;
			//return(<p>PasswordWidget: nothing to do.</p>);
		}
		else
		{
			return(
			   	<div>
			   		<p>Enter password for key {this.state.data.key_name}</p>
			        <input type="password" ref="password" />
			        <input
			          type="button"
			          value="ok"
			          onClick={this.sendPassword}
			        />
		      	</div>
			);
		}
	},
});

var AccountSelectWidget =  React.createClass({
	mixins: [AutoUpdateMixin],
	getInitialState: function(){
		return {mode: "list", data: []};
	},
	getPath: function(){
		return "control/locations";
	},
	selectAccount: function(id){
		console.log("login with id="+id)
		RS.request({path: "control/login", data:{id: id}});
		var state = this.state;
		state.mode = "wait";
		this.setState(state);
	},
	render: function(){
		var component = this;
		if(this.state.mode === "wait")
			return <p>waiting...</p>;
		else
		return(
			<div>
				{
					this.state.data.map(function(location){
						return <div className="btn2" key={location.id} onClick ={function(){component.selectAccount(location.id);}}>login {location.name} ({location.location})</div>;
					})
				}
			</div>
		);
	},
});

var LoginWidget = React.createClass({
	mixins: [AutoUpdateMixin],
	getInitialState: function(){
		return {data: {runstate: "waiting_init"}};
	},
	getPath: function(){
		return "control/runstate";
	},
	shutdown: function(){
		RS.request({path: "control/shutdown"});
	},
	render: function(){
		if(this.state.data.runstate === "waiting_init")
		{
			return(<p>Retroshare is initialising...      please wait...</p>);
		}
		else if(this.state.data.runstate === "waiting_account_select")
		{
			return(<AccountSelectWidget/>);
		}
		else
		{
			return(
				<div>
					<p>runstate: {this.state.data.runstate}</p>
					<div onClick={this.shutdown} className="btn">shutdown Retroshare</div>
				</div>
			);
		}
	},
});

var LoginWidget2 = React.createClass({
	debug: function(msg){
		console.log(msg);
	},
	getInitialState: function(){
		return {display_data:[], state: "waiting", hidden_node: false, error: ""};
	},
	componentDidMount: function()
	{
		this.update();
		// FOR TESTING
		//this.setState({state: "create_location"});
	},
	// this component depends on three resources
	// have to fecth all, before can display anything
	data: {},
	clear_data: function()
	{
		this.debug("clear_data()");
		data = 
		{
			have_runstate: false,
			runstate: "",
			have_identities: false,
			identities: [],
			have_locations: false,
			locations: [],

			pgp_id: undefined,
			pgp_name: undefined,
		};
	},
	update: function()
	{
		this.debug("update()");
		var c = this;
		c.clear_data();
		function check_data()
		{
			if(c.data.have_runstate && c.data.have_locations && c.data.have_identities)
			{
				c.debug("update(): check_data(): have all data, will change to display mode");
				var data = [];
				var pgp_ids_with_loc = [];
				function pgp_id_has_location(pgp_id)
				{
					for(var i in pgp_ids_with_loc)
					{
						if(pgp_id === pgp_ids_with_loc[i])
							return true;
					}
					return false;
				}
				// collect all pgp ids with location
				// collect location data
				for(var i in c.data.locations)
				{
					if(!pgp_id_has_location(c.data.locations[i].pgp_id))
						pgp_ids_with_loc.push(c.data.locations[i].pgp_id);
					data.push(c.data.locations[i]);
				}
				// collect pgp data without location
				for(var i in c.data.identities)
				{
					if(!pgp_id_has_location(c.data.identities[i].pgp_id))
						data.push(c.data.identities[i]);
				}
				c.setState({
					display_data: data,
					state: "display",
				});
			}
		}
		RS.request({path:"control/runstate"}, function(resp){
			c.debug("update(): got control/runstate");
			if(resp.returncode === "ok"){
				c.data.have_runstate = true;
				c.data.runstate = resp.data.runstate;
				check_data();
			}
		});
		RS.request({path:"control/identities"}, function(resp){
			c.debug("update(): got control/identities");
			if(resp.returncode === "ok"){
				c.data.have_identities = true;
				c.data.identities = resp.data;
				check_data();
			}
		});
		RS.request({path:"control/locations"}, function(resp){
			c.debug("update(): got control/locations");
			if(resp.returncode === "ok"){
				c.data.have_locations = true;
				c.data.locations = resp.data;
				check_data();
			}
		});
	},
	shutdown: function(){
		RS.request({path: "control/shutdown"});
	},
	selectAccount: function(id){
		this.debug("login with id="+id);
		RS.request({path: "control/login", data:{id: id}});
		var state = this.state;
		state.mode = "wait";
		this.setState(state);
	},
	onDrop: function(event)
	{
		this.debug("onDrop()");
		this.debug(event.dataTransfer.files);
		event.preventDefault();

		var reader = new FileReader();  

		var widget = this;

		var c = this;
		reader.onload = function(evt) {
			c.debug("onDrop(): file loaded");
			RS.request({path:"control/import_pgp", data:{key_string:evt.target.result}}, c.importCallback);
		};
		reader.readAsText(event.dataTransfer.files[0]);
		this.setState({state:"waiting"});
	},
	importCallback: function(resp)
	{
		this.debug("importCallback()");
		console.log(resp);
		if(resp.returncode === "ok")
		{
			this.debug("import ok");
			this.setState({error: "import ok"});
		}
		else
		{
			this.setState({error: "import error"});
		}
		this.update();

	},
	selectAccount: function(id){
		console.log("login with id="+id)
		RS.request({path: "control/login", data:{id: id}});
		this.setState({state: "waiting"});
	},
	setupCreateLocation: function(pgp_id, pgp_name)
	{
		this.data.pgp_id = pgp_id;
		this.data.pgp_name = pgp_name;
		this.setState({state: "create_location"});
	},
	submitLoc: function()
	{
		var req = {
			path: "control/create_location",
			data: {
				ssl_name: "nogui-webui"
			}
		};
		if(this.data.pgp_id !== undefined)
		{
			req.data["pgp_password"] = this.refs.pwd1.getDOMNode().value;
			req.data["pgp_id"] = this.data.pgp_id;
		}
		else
		{
			var pgp_name = this.refs.pgp_name.getDOMNode().value
			var pwd1 = this.refs.pwd1.getDOMNode().value
			var pwd2 = this.refs.pwd2.getDOMNode().value
			if(pgp_name === "")
			{
				this.setState({error:"please fill in a name"});
				return;
			}
			if(pwd1 === "")
			{
				this.setState({error:"please fill in a password"});
				return;
			}
			if(pwd1 !== pwd2)
			{
				this.setState({error:"passwords do not match"});
				return;
			}
			req.data["pgp_name"] = pgp_name;
			req.data["pgp_password"] = pwd1;
		}
		if(this.refs.cb_hidden.getDOMNode().checked)
		{
			var addr = this.refs.tor_addr.getDOMNode().value
			var port = this.refs.tor_port.getDOMNode().value
			if(addr === "" || port === "")
			{
				this.setState({error:"please fill in hidden adress and hidden port"});
				return;
			}
			req.data["hidden_adress"] = addr;
			req.data["hidden_port"] = port;
		}
		var c = this;
		RS.request(req, function(resp){
			if(resp.returncode === "ok")
			{
				c.setState({state:"waiting_end", msg:"created account"});
			}
			else
			{
				c.setState({state:"display", error:"failed to create account: "+resp.debug_msg})
			}
		});
		this.setState({state:"waiting"});
	},
	render: function(){
		var c = this;
		if(this.state.state === "waiting")
		{
			return(<div>
				<p>please wait a second...</p>
				</div>);
			//return(<p>Retroshare is initialising...      please wait...</p>);
		}
		//else if(this.state.data.runstate === "waiting_account_select")
		else if(this.state.state === "display")
		{
			if(this.data.runstate === "waiting_account_select")
			{
				return(
				<div>
					<div>{this.state.error}</div>
					{
						this.state.display_data.map(function(loc){
							if(loc.peer_id)
								return <div className="btn2" key={loc.id} onClick={function(){c.selectAccount(loc.id);}}
										>{loc.name} ({loc.location})</div>;
							else
								return <div className="btn2" key={loc.id} onClick={function(){c.setupCreateLocation(loc.pgp_id, loc.name);}}
										>{loc.name}</div>;
						})
					}
					<div 
						onDragOver={function(event){/*important: block default event*/event.preventDefault();}}
						onDrop={this.onDrop}
						className="btn2"
					>drag and drop a profile file here</div>
					<div className="btn2" onClick={function(){c.setupCreateLocation();}}>create new profile</div>
					<div onClick={this.shutdown} className="btn2">shut down Retroshare</div>
				</div>);
			}
			else
			{
				<div>
					<p>This is the login page. It has no use at the moment, because Retroshare is aleady running.</p>
				</div>
			}
		}
		else if(this.state.state === "create_location")
		{
			return(
				<div>
					<div>Create a new node</div>
					{function(){
						if(c.data.pgp_id !== undefined)
							return <div>
							Create new nodw with pgp_id {c.data.pgp_id}
							<input type="password" ref="pwd1" placeholder="password"/>
							</div>
						else
							return <div>
							<input type="text" ref="pgp_name" placeholder="name"/>
							<input type="password" ref="pwd1" placeholder="password"/>
							<input type="password" ref="pwd2" placeholder="password (repeat)"/>
							</div>;
					}()}
					<input className="checkbox" type="checkbox" ref="cb_hidden"
						onClick={function(){c.setState({hidden_node:c.refs.cb_hidden.getDOMNode().checked})}}
					/>TOR hidden node<br/>
					{function(){if(c.state.hidden_node)
						return <div>
						<input type="text" ref="tor_addr" placeholder="tor address"/>
						<input type="text" ref="tor_port" placeholder="hidden service port"/>
						</div>;}()
					}
					<div>{this.state.error}</div>
					<div className="btn2" onClick={c.update}>cancel</div>
					<div className="btn2" onClick={c.submitLoc}>go</div>
				</div>
			);
		}
		else
		{
			return(
				<div>
					<div onClick={this.shutdown} className="btn">shutdown Retroshare</div>
				</div>
			);
		}
	},
});

var Menu = React.createClass({
	mixins: [SignalSlotMixin],
	getInitialState: function(){
		return {};
	},
	componentWillMount: function()
	{
	},
	render: function(){
		var outer = this;
		return (
				<div>
					<div className="btn2" onClick={function(){outer.emit("change_url", {url: "main"});}}>
						Start
					</div>
					{/*function(){
						if(outer.props.fullcontrol)
							return (<div className="btn2" onClick={function(){outer.emit("change_url", {url: "login"});}}>
								Login
							</div>);
						else return <div></div>;
					}()*/}
					<div className="btn2" onClick={function(){outer.emit("change_url", {url: "friends"});}}>
						Friends
					</div>
					<div className="btn2" onClick={function(){outer.emit("change_url", {url: "downloads"});}}>
						Downloads
					</div>
					<div className="btn2" onClick={function(){outer.emit("change_url", {url: "search"});}}>
						Search
					</div>
					{/*<div className="btn2" onClick={function(){outer.emit("change_url", {url: "testwidget"});}}>
						TestWidget
					</div>*/}
			</div>
		);
	},
});

var TestWidget = React.createClass({
	mixins: [SignalSlotMixin],
	getInitialState: function(){
		return {s:"one"};
	},
	componentWillMount: function()
	{
	},
	one: function(){
		this.setState({s:"one"});
	},
	two: function(){
		this.setState({s:"two"});
	},
	render: function(){
		var outer = this;
		var outercontainerstyle = {borderStyle: "solid", borderColor: "darksalmon", overflow: "hidden", width: "100%"};
		var transx = "0px";
		if(this.state.s === "two")
			transx = "-45%";
		var innercontainerstyle = {width: "200%", transform: "translatex("+transx+")", WebkitTransform: "translatex("+transx+")", transition: "all 0.5s ease-in-out", WebkitTransition: "all 0.5s ease-in-out"};
		var innerstyle = {float:"left", width: "45%"};
		var two = <div></div>;
		if(this.state.s === "two")
			two = <div style={innerstyle} className="btn2" onClick={function(){outer.one();}}>
					two
				</div>;
		return (
			<div style={outercontainerstyle}>
			<div style={innercontainerstyle}>
				<div style={innerstyle} className="btn2" onClick={function(){outer.two();}}>
					one
				</div>
				{two}
			</div>
			</div>
		);
	},
});

var FileUploadWidget = React.createClass({
	getInitialState: function(){
		// states:
		// waiting_user waiting_upload upload_ok upload_failed
		return {state: "waiting_user", file_name: "", file_id: 0};
	},
	onDrop: function(event)
	{
		console.log(event.dataTransfer.files);
		event.preventDefault();

		this.setState({state:"waiting_upload", file_name: event.dataTransfer.files[0].name});

		var reader = new FileReader();  
		var xhr = new XMLHttpRequest();

		var self = this;
		xhr.upload.addEventListener("progress", function(e) {
			if (e.lengthComputable) {
				var percentage = Math.round((e.loaded * 100) / e.total);
				console.log("progress:"+percentage);
			}
		}, false);

		var widget = this;
		xhr.onreadystatechange = function(){
			if (xhr.readyState === 4) {
				if(xhr.status !== 200)
				{
					console.log("upload failed status="+xhr.status);
					widget.setState({state: "upload_failed"});
					return;
				}
				//console.log("upload ok");
				//console.log(JSON.parse(xhr.responseText));
				var resp = JSON.parse(xhr.responseText);
				if(resp.ok)
				{
					widget.setState({state:"upload_ok", file_id: resp.id});
					// tell parent about successful upload
					if(widget.props.onUploadReady)
						widget.props.onUploadReady(resp.id);
				}
				else
					widget.setState({state:"upload_fail"});
			}
		};
		xhr.open("POST", upload_url);
		reader.onload = function(evt) {
			xhr.send(evt.target.result);
		};
		// must read as array buffer, to preserve binary data as it is
		reader.readAsArrayBuffer(event.dataTransfer.files[0]);
	},
	render: function(){
		var text = "bug-should not happen";
		if(this.state.state ==="waiting_user")
			text = "drop a file";
		if(this.state.state ==="waiting_upload")
			text = "File " + this.state.file_name + " is being uploaded";
		if(this.state.state ==="upload_ok")
			text = "File " + this.state.file_name + " uploaded ok. file_id=" + this.state.file_id;
		if(this.state.state ==="upload_fail")
			text = "Could not upload file" + this.state.file_name;

		return (
			<div 
				onDragOver={function(event){/*important: block default event*/event.preventDefault();}}
				onDrop={this.onDrop}
			>
				{text}
			</div>
		);
	},
});

var MainWidget = React.createClass({
	mixins: [SignalSlotMixin, AutoUpdateMixin],
	getPath: function(){
		return "control/runstate";
	},
	getInitialState: function(){
		// hack: get hash
		var url = window.location.hash.slice(1);
		if(url === "")
			url = "menu";
		return {page: url, data: {runstate: "waiting_for_server"}};
		//return {page: "login"};
	},
	componentWillMount: function()
	{
		var outer = this;
		this.connect("url_changed",
		function(params)
		{
			console.log("MainWidget received url_changed. url="+params.url);
			var url = params.url;
			if(url.length === 0)
				url = "menu";
			outer.setState({page: url});
		});
	},
	render: function(){
		var outer = this;
		var mainpage = <p>page not implemented: {this.state.page}</p>;

		if(this.state.data.runstate === "waiting_for_server")
		{
			mainpage = <div><p>waiting for reply from server...<br/>please wait...</p></div>;
		}
		if(this.state.data.runstate === "waiting_init" || this.state.data.runstate === "waiting_account_select")
		{
			//mainpage = <LoginWidget/>;
			mainpage = <LoginWidget2/>;
		}

		if(this.state.page === "testing")
		{
			//mainpage = <TestWidget/>;
			//mainpage = <FileUploadWidget/>;
			mainpage = <LoginWidget2/>;
		}

		if(this.state.data.runstate === "running_ok" || this.state.data.runstate ==="running_ok_no_full_control")
		{
			if(this.state.page === "main")
			{
				mainpage = <div><p>
				A new webinterface for Retroshare. Build with react.js.
				React allows to build a modern and user friendly single page application. 
				The component system makes this very simple.
				Updating the GUI is also very simple: one React mixin can handle updating for all components.
				</p>
				</div>;
			}
			if(this.state.page === "friends")
			{
				mainpage = <Peers3 />;
			}
			if(this.state.page === "downloads")
			{
				mainpage = <DownloadsWidget/>;
			}
			if(this.state.page === "search")
			{
				mainpage = <SearchWidget/>;
			}
			if(this.state.page === "add_friend")
			{
				mainpage = <AddPeerWidget/>;
			}
			if(this.state.page === "menu")
			{
				mainpage = <Menu fullcontrol = {this.state.data.runstate === "running_ok"}/>;
			}
		}

		var menubutton = <div onClick={function(){outer.emit("change_url", {url: "menu"});}} className="btn2">&lt;- menu</div>;
		if(this.state.page === "menu")
			menubutton = <div>Retroshare webinterface</div>;
		return (
			<div>
				{/*<div id="overlay"><div className="paddingbox"><div className="btn2">test</div></div></div>*/}
				<ConnectionStatusWidget/>
				<PasswordWidget/>
				<AudioPlayerWidget/>
				{menubutton}
				{mainpage}
				{/*<ProgressBar progress={0.7}/>*/}
			</div>
		);
	},
});

React.render(
	<MainWidget />,
	document.body
);
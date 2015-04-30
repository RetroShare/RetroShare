var connection = new RsXHRConnection(window.location.hostname, window.location.port);
var RS = new RsApi(connection);
RS.start();

var api_url = window.location.protocol + "//" + window.location.hostname + ":" + window.location.port + "/api/v2/";
var filestreamer_url = window.location.protocol + "//" +window.location.hostname + ":" + window.location.port + "/fstream/";

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

var Peers = React.createClass({
	mixins: [AutoUpdateMixin],
	getPath: function(){return "peers";},
	getInitialState: function(){
		return {data: []};
	},
	render: function(){
		var renderOne = function(f){
			console.log("make one");
			return <p>{f.name} <img src={api_url+f.locations[0].avatar_address} /></p>;
		};
		return <div>{this.state.data.map(renderOne)}</div>;
	},
 });

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

var AddPeerWidget = React.createClass({
	getInitialState: function(){
		return {page: "start"};
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
		RS.request({path: "peers", data: {cert_string: this.state.cert_string}});
	},
	render: function(){
		if(this.state.page === "start")
			return(
				<div>
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
			        <span onClick={this.final_add_handler} className="btn">yes</span>
				</div>
			);
	},
});

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
					<td>{this.props.data.size}</td>
					<td>{this.props.data.transfered / this.props.data.size}</td>
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
				<div><p>select a location to log in</p></div>
				{this.state.data.map(function(location){
					return <div className="btn2" key={location.id} onClick ={function(){component.selectAccount(location.id);}}>{location.name} ({location.location})</div>;
				})}
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
					{function(){
						if(outer.props.fullcontrol)
							return (<div className="btn2" onClick={function(){outer.emit("change_url", {url: "login"});}}>
								Login
							</div>);
						else return <div></div>;
					}()}
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
			mainpage = <LoginWidget/>;
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
			mainpage = <Peers2 />;
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
		if(this.state.page === "login")
		{
			mainpage = <LoginWidget/>;
		}
		if(this.state.page === "menu")
		{
			mainpage = <Menu fullcontrol = {this.state.data.runstate === "running_ok"}/>;
		}
		if(this.state.page === "testwidget")
		{
			mainpage = <TestWidget/>;
		}
		}

		var menubutton = <div onClick={function(){outer.emit("change_url", {url: "menu"});}} className="btn2">&lt;- menu</div>;
		if(this.state.page === "menu")
			menubutton = <div>Retroshare webinterface</div>;
		return (
			<div>
				<PasswordWidget/>
				<AudioPlayerWidget/>
				{menubutton}
				{mainpage}
			</div>
		);
	},
});

React.render(
	<MainWidget />,
	document.body
);
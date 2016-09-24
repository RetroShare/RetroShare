var rs=require("retroshare");
module.exports = {  nodes: [
	{
		name: "home",
		path: "/"
	},
	{
		name: "login",
		module: "accountselect",
		runstate: "waiting_account_select",
		counter: rs.counting("control/locations"),
	},
	{
	    name: "create login",
	    path: "/createlogin",
	    module: "createlogin",
	    runstate: "waiting_account_select",
	},
	{
		name: "peers",
		runstate: "running_ok.*",
		counter: rs.counting("peers", function(data){
		    var onlinecount = 0;
		    data.map(function(peer) {
		        var is_online = false;
		        peer.locations.map(function (location){
		            if (location.is_online) {
		                is_online=true;
		            }
		        });
		        if (is_online) {
		            onlinecount +=1;
		        }
		    });
		    return onlinecount + "/" + data.length;
		})
	},
	{
	    name: "addpeer",
	    runstate: "running_ok.*",
	    show: false,
	},
	{
	    name: "identities",
	    runstate: "running_ok.*",
	    counter: rs.counting("identity/own"),
	},
	{
	    name: "addidentity",
	    runstate: "running_ok.*",
	    show: false,
	},
	{
	    name:"searchresult",
	    path: "/search/:id",
	    runstate: "running_ok.*",
	},
	{
	    name: "search",
	    runstate: "running_ok.*",
	},
	{
		name: "downloads",
		runstate: "running_ok.*",
		counter: rs.counting("transfers/downloads")
	},
	{
		name: "adddownloads",
		runstate: "running_ok.*",
		path: "/downloads/add",
		show: false,
	},
	{
	    name: "forums",
	    runstate: "running_ok.*",
	},
	{
		name: "chat",
		runstate: "running_ok.*",
		counter: rs.counting2({
	        "peers": function(peer) {
	            var sum = 0;
	            peer.locations.map(function (loc) {
	                sum += parseInt(loc.unread_msgs);
	            });
	            return sum;
	        },
	        "chat/lobbies": function(lobby) {
	            return lobby.unread_msg_count;
	        }
		})
	},
	{
	    name:"settings",
	    runstate: "running_ok.*",
	},
	{
	    name:"servicecontrol",
	    runstate: "running_ok.*",
	    path:"/settings/servicecontrol",
	    show: false,
	},
	{
		name: "shutdown",
		runstate: "running_ok|waiting_account_select",
		action: function(m){
			rs.request("control/shutdown",null,function(){
				rs("control/runstate").runstate=null;
				rs.forceUpdate("control/runstate");
				m.redraw();
			});
		}
	},
	{
	    name: "waiting",
	    show: false,
	},
]
}

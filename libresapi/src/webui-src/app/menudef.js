module.exports = {  nodes: [
	{
		name: "home",
		path: "/"
	},
	{
		name: "login",
		module: "accountselect",
		runstate: "waiting_account_select",
	},
	{
		name: "peers",
		runstate: "running_ok.*",
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
	},
	{
		name: "chat",
		runstate: "running_ok.*",
	},
	{
		name: "shutdown",
		runstate: "running_ok|waiting_account_select",
		action: function(rs, m){
			rs.request("control/shutdown",null,function(){
				rs("control/runstate").runstate=null;
				rs.forceUpdate("control/runstate");
				m.redraw();
			});
		}
	},
]
}

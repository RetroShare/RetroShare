"use strict";

var m = require("mithril");
var rs = require("retroshare");
var menu =require("menu");

module.exports = {view: function(){
    var runstate = rs("control/runstate");
    console.log("runstate: " + (runstate === undefined ? runstate : runstate.runstate));
    if(runstate === undefined){
        return m("div", "waiting_server ... ");
    } else if (runstate.runstate == null){
        return m("div", "server down");
    } else {
        if (rs.content === undefined) {
            rs.content = null;
        }
        if (runstate.runstate == "waiting_init") {
            return m("div","server starting ...")
        } else if(runstate.runstate =="waiting_account_select") {
            return m("div", [
    	        m("div", menu.view()),
    	        m("hr"),
    	        m("div", require("accountselect").view())
            ]);
        } else if (runstate.runstate == "waiting_startup") {
	    return m("div","RetroShare starting ... please wait ...");
        } else if(runstate.runstate =="running_ok" || runstate.runstate=="running_ok_no_full_control") {
            return m("div", [
                m("div", menu.view()),
                m("hr"),
                m("div", rs.content == null ? null : rs.content.view())
            ]);
        } else {
            return m("div", "unknown runstate: " + runstate.runstate);
        }
    }
}
};

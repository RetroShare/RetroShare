"use strict";

var m = require("mithril");
var rs = require("retroshare");

function shutdown(){
    rs.request("control/shutdown",null,function(){
        rs("control/runstate").runstate=null;
        m.redraw();
    });

}

function goback(){
    rs.content=null;
    m.redraw();
}

function peers(){
    rs.content=require("peers");
}

function chat(){
    rs.content=require("chat");
}

module.exports = {view: function(){
    if (rs.content!=undefined && rs.content != null) {
    	return m("div", {onclick:goback}, "back");
    }
    var nodes = [];
    var runstate = rs("control/runstate");
    if (runstate.runstate == "running_ok_no_full_control"||runstate.runstate =="running_ok") {
        nodes.push(m("div",{onclick:function(){peers();}},"peers"));
        nodes.push(m("div",{onclick:function(){chat();}},"chat"));
    }
    if (runstate.runstate == "waiting_account_select"||runstate.runstate == "running_ok"){
    	nodes.push(m("div",{onclick:shutdown},"shutdown"));
    }
    return m("div", [
    	m("h2","menu:"),
    	m("hr"),
    	nodes
    ])
}
};

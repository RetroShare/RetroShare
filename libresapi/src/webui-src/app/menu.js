"use strict";

var m = require("mithril");
var rs = require("retroshare");
var mnodes = require("menudef");

function goback(){
    rs.content=null;
    m.redraw();
}

module.exports = {view: function(){
    if (m.route() != "/") {
    	return m("div", {onclick:function(){m.route("/");}}, "home");
    }
    var runstate = rs("control/runstate");
    if (runstate === undefined || runstate.runstate === undefined || runstate.runstate == null)
    	return m("h2","menu: waiting for server ...");
    return m("div", [
    	m("h2","menu:"),
    	m("hr"),
    	mnodes.nodes.map(function(menu){
            if (menu.runstate === undefined || runstate.runstate.match(menu.runstate)){
                if (menu.action === undefined) {
                    return m("div", {
                        onclick: function(){
                            m.route(
                                menu.path != undefined ? menu.path : "/" + menu.name
                            )
                        }
                    }, menu.name);
                } else {
                    return m("div", {onclick: function(){menu.action(rs,m)}}, menu.name);
                }
            }
        })
    ]);
}
};

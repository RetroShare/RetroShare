"use strict";

var m = require("mithril");
var rs = require("retroshare");
var mnodes = require("menudef");

function goback(){
    rs.content=null;
    m.redraw();
}

function buildmenu(menu, tagname, runstate, ignore){
    if (
        (menu.runstate === undefined
            || runstate.match("^(" + menu.runstate + ")$")!=null)
        && (!menu.name.match(ignore))
        && (menu.path === undefined || menu.path.match(":")==null)
        && (menu.show === undefined || menu.show)
    )  {
        var name = menu.name;
        if (menu.counter != undefined) {
            name += menu.counter();
        }
        if (menu.action === undefined) {
            return m(tagname , {
                onclick: function(){
                    m.route(
                        menu.path != undefined ? menu.path : "/" + menu.name
                    )
                }
            }, name);
        } else {
            return m(tagname, {onclick: function(){menu.action(m)}}, name);
        }
    }
}

module.exports = {view: function(){
    var runstate = rs("control/runstate");
    if (runstate === undefined
        || runstate.runstate === undefined
        || runstate.runstate == null)
    	return m("div.nav","menu: waiting for server ...");
    if (m.route() != "/")
    return m("span",[
            m("span"," | "),
        	mnodes.nodes.map(function(menu){
        	    var item = buildmenu(menu,"span.menu", runstate.runstate, "-");
        	    if (item != null){
        	        return [
            	        item,
            	        m("span"," | ")
            	    ]
            	}
            })
        ]);
    return m("div", [
    	m("h2","home"),
    	m("hr"),
    	mnodes.nodes.map(function(menu){
    	    return buildmenu(menu,"div.btn2", runstate.runstate, "home");
        })
    ]);
}
};

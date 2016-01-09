"use strict";

var m = require("mithril");
var rs = require("retroshare");
var menu =require("menu");
var currentpasswd = null;


function setPasswd(password) {
    currentpasswd = password
}

function sendPassword(data) {
    console.log("sending pwd for " + data.key_name + "...");
    rs.request("control/password", {password: currentpasswd}, function(){
        m.redraw();
    });
}

function Page(content, runst){
    this.view = function(){
        var runstate = rs("control/runstate");
        var needpasswd = rs("control/password");
        //console.log("runstate: " + (runstate === undefined ? runstate : runstate.runstate));
        if(runstate === undefined){
            return m("h2", "waiting_server ... ");
        } else if (runstate.runstate == null){
            return m("h2", "server down");
        } else if (needpasswd != undefined && needpasswd.want_password === true){
            return m("div",[
                m("h2","password required"),
                m("h3",needpasswd.key_name),
                m("input",{type:"password", onchange:m.withAttr("value", setPasswd)}),
                m("br"),
                m("input[type=button][value=send password]",{onclick: function(){sendPassword(needpasswd);}}),
            ]);
        } else {
            if ("waiting_init|waiting_startup".match(runstate.runstate)) {
                return m("h2","server starting ...")
            } else if("waiting_account_select|running_ok*".match(runstate.runstate)) {
                if (runst === undefined || runst.match(runstate.runstate)) {
                    return m("div", [
                        m("div", menu.view()),
        	            m("hr"),
                        m("div", content)
                    ]);
                } else {
                    // funktion currently not available
                    m.route("/");
                    return m("div", [
                        m("div", menu.view()),
        	            m("hr"),
                        m("div", require("home").view())
                    ]);
                };
            } else {
                return m("div", "unknown runstate: " + runstate.runstate);
            }
        }
    }
};


var me ={};

me.init = function(main){
    console.log("start init ...");
    var menudef = require("menudef");
    var maps = {};
    var m = require("mithril");

    menudef.nodes.map(function(menu){
        if (menu.action === undefined) {
            var path = menu.path != undefined ? menu.path : ("/" + menu.name);
            var module = menu.module != undefined ? menu.module : menu.name
            console.log("adding route " + menu.name + " for " + path + " with module " + module);
            maps[path] = new Page(require(module), menu.runstate);
        }
    });
    m.route.mode = "hash";
    m.route(main,"/",maps);
    console.log("init done.");
};

module.exports = me;


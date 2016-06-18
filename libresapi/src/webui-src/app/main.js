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

function Page(menu){
    this.menu = menu;
    this.module = (menu.module != undefined) ? menu.module : menu.name;
    this.path = menu.path != undefined ? menu.path : ("/" + menu.name);

    var runst = menu.runstate;
    var content = require(this.module);
    var mm = require("menu");

    this.view = function(){
        var runstate = rs("control/runstate");
        var needpasswd = rs("control/password");
        //console.log("runstate: " + (runstate === undefined ? runstate : runstate.runstate));
        if(runstate === undefined){
            return m("h2", "waiting_server ... ");
        } else if (runstate.runstate == null){
            // try clean reboot ...
            rs.clearCache();
            rs("control/runstate"); //reboot detector
            console.log("i'm down");
            return m("h2", "server down");
        } else if (needpasswd != undefined && needpasswd.want_password === true){
            m.initControl = "txtpasswd";
            return m("div",[
                m("h2","password required"),
                m("h3",needpasswd.key_name),
                m("input",{
                    id: "txtpasswd",
                    type:"password",
                    onchange:m.withAttr("value", setPasswd),
                    onkeydown: function(event){
                        if (event.keyCode == 13){
                            setPasswd(this.value);
                            sendPassword(needpasswd);
                        }
                    }
                }),
                m("br"),
                m("input[type=button][value=send password]",{
                    onclick: function(){
                        sendPassword(needpasswd);
                    }
                }),
            ]);
        } else {
            if (runstate.runstate.match("waiting_init|waiting_startup")) {
                return m("h2","server starting ...")
            } else if(runstate.runstate.match("waiting_account_select|running_ok.*")) {
                if (runst === undefined || runstate.runstate.match(runst)) {
                    return m("div", {
                        style: {
                            height: "100%",
                            "box-sizing": "border-box",
                            "padding-bottom": "40px"
                        }
                    }, [
                        m("div", mm.view()),
        	            m("hr"),
                        m("div", {
                            style: {
                                height: "100%",
                                "box-sizing": "border-box",
                                "padding-bottom":"40px"
                            }
                        }, content)
                    ]);
                } else {
                    // funktion currently not available
                    m.route("/");
                    return m("div", [
                        m("div", mm.view()),
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


module.exports = {
    init:function(main){
        console.log("start init ...");
        var menudef = require("menudef");
        var maps = {};
        var m = require("mithril");

        menudef.nodes.map(function(menu){
            if (menu.action === undefined) {
                var p = new Page(menu)
                console.log("adding route " + menu.name + " for " + p.path + " with module " + p.module);
                maps[p.path] = p;
            }
        });
        m.route.mode = "hash";
        m.route(main,"/",maps);
        console.log("init done.");
    }
};


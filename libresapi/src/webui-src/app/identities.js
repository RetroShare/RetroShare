"use strict";

var m = require("mithril");
var rs = require("retroshare");

module.exports = {view: function(){
    return m("div",[
        m("h2","identities"),
        m("hr"),
        m("div.btn2", {
            onclick: function (){
                m.route("/addidentity");
            }
        },"< create new identity >"),
        m("ul",
            rs.list("identity/own", function(item){
                return m("li",[m("h2",item.name)]);
            },
            rs.sort("name"))
        )
    ]);
}}

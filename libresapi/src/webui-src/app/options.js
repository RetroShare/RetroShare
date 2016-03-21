"use strict";

var m = require("mithril");
var rs = require("retroshare");

module.exports = {
    view: function(){
        return m("div", [
            m("h2","Options"),
            m("hr"),
            m("div.btn2",{
                onclick: function(){
                    m.route("/options/servicecontrol");
                },
            }, "rights")
        ]);
    }
}


"use strict";

var m = require("mithril");
var rs = require("retroshare");

module.exports = {view: function(){
    return m("div",[
        m("h2","forums"),
        m("p","(work in progress, currently only listing)"),
        m("hr"),
        /*
        m("div.btn2", {
            onclick: function (){
                m.route("/addforum");
            }
        },"< create new forum >"),
        */
        m("ul",
            rs.list("forums",
                function(item){
                    return m("li",[
                        m("h2",item.name),
                        m("div",{style:{margin:"10px"}},
                            [
                                item.description != ""
                                ? [
                                    m("span", "Description: "
                                     + item.description),
                                    m("br")]
                                : [],
                                m("span","messages visible: "
                                 + item.visible_msg_count),
                            ]
                        ),
                        /*
                        item.subscribed
                            ? [
                                m(
                                    "span.btn2",
                                    {style:{padding:"0px"}},
                                    "unsubscribe"
                                ),
                                " ",
                                m(
                                    "span.btn2",
                                    {style:{padding:"0px", margin:"10px"}},
                                    "enter"
                                ),
                                m("hr", {style: {color:"silver"}}),
                            ]
                            : [
                                m(
                                    "span.btn2",
                                    {style:{padding:"0px", margin:"10px"}},
                                    "subscribe"
                                ),
                            ]
                        */
                    ]);
                },
                rs.sort("name")
            )
        )
    ]);
}}

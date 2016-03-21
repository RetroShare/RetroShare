"use strict";

var m = require("mithril");
var rs = require("retroshare");

function setOption(id,value) {
    return function(){
        rs.request("servicecontrol", {
            service_id: id,
            default_allowed: value,
        });
        rs.forceUpdate("servicecontrol", true);
    }
}

module.exports = {
    view: function(){
        return m("div", [
            m("h2","Options / Rights"),
            m("div.btn2",{
                onclick: function(){
                    m.route("/options")
                },
            },"back to options"),
            m("hr"),
            m("ul", rs.list("servicecontrol", function(item){
                    //return m("li",item.service_name)
                    if (item.service_name.match("banlist")) {
                        console.log("banlist:" + item.default_allowed);
                    }
                    return m("li", {
                        style: {
                            margin: "2px",
                            color: item.default_allowed ? "lime" :"red",
                        }
                    }, [
                        m("div", {
                            onclick: setOption(
                                item.service_id,
                                !item.default_allowed
                            ),
                        }, [
                            m("div.menu", {
                                style: {
                                    float:"left",
                                    width:"30px",
                                    color: "#303030",
                                    textAlign: "center",
                                    backgroundColor: !item.default_allowed
                                        ? "black"
                                        : "lime",
                                }
                            }, "ON"),
                            m("div.menu",{
                                style: {
                                    float:"left",
                                    width:"30px",
                                    textAlign: "center",
                                    marginRight:"5px",
                                    color: "#303030",
                                    backgroundColor: item.default_allowed
                                        ? "black"
                                        : "lime",
                                }
                            }, "OFF"),
                        ]),
                        m("div", {
                            style: {
                                color: "lime",
                            }
                        }, item.service_name),
                        m("div", {
                            style: {
                                clear: "left"
                            }
                        }),
                    ]);
                })
            )
        ]);
    }
}


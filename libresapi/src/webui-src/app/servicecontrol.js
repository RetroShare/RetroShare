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

function setUserOption(serviceid, userid, value) {
    return function(){
        rs.request("servicecontrol/user", {
            service_id: serviceid,
            peer_id: userid,
            enabled: value
        }, function(){
            rs.forceUpdate("servicecontrol", true)
        });
    }
}

function createSwitch(isOn, width) {
    if (width === undefined) {
        width = "2.1em";
    }
    return [
        m("div.menu", {
            style: {
                float:"left",
                width: width,
                textAlign: "center",
                color: "#303030",
                borderColor: isOn
                    ? "lime"
                    : "red",
                backgroundColor: !isOn
                    ? "black"
                    : "lime",
            }
        }, "ON"),
        m("div.menu",{
            style: {
                float:"left",
                width: width,
                textAlign: "center",
                marginRight:"5px",
                color: "#303030",
                borderColor: isOn
                    ? "lime"
                    : "red",
                backgroundColor: isOn
                    ? "black"
                    : "red",
            }
        }, "OFF"),
    ];
}

function breadcrums(name, parts){
    var result = [];
    rs.for_key_in_obj(parts, function(partname,item){
        result.push(
            m("span.btn",{
                onclick: function(){
                    m.route(item)
                }
            },partname)
        );
        result.push(" / ");
    });
    result.push(name);
    return result;
}

function serviceView(serviceid) {
    var service, liste;
    service = rs.find(rs("servicecontrol"),"service_id",serviceid);
    if (service == null) {
        return m("h3","<please wait ... >");
    }
    liste = service.default_allowed
        ? service.peers_denied
        : service.peers_allowed;
    return m("div", [
        m("h2", breadcrums(service.service_name, {
            settings:"/settings",
            rights: "/settings/servicecontrol",
        })),
        m("hr"),
        m("h2",{
            style:{
                float:"left",
            }
        },[
            m("div",{
                style:{
                    float:"left",
                }
            },"user rights for: " + service.service_name + ", default: "),
            m("div", {
                onclick: setOption(
                    serviceid,
                    !service.default_allowed
                ),
                style: {
                    float:"left",
                    marginLeft: "0.4em",
                    marginRight: "0.4em",
                }
            },createSwitch(service.default_allowed)),
        ]),
        m("div", {
            style: {
                clear:"left",
            }
        }),
        m("ul", rs.list("peers",function(peer){
            var locs;
            locs = peer.locations;
            locs.sort(rs.sort("location"));
            return peer.locations.map(function(location){
                var isExcept, isOn;
                isExcept = liste != null
                    && liste.indexOf(location.peer_id)>=0;
                isOn = service.default_allowed ? !isExcept: isExcept;
                return m("li", {
                    style: {
                        margin: "5px",
                        color: isOn ? "lime" :"red",
                    }
                }, [
                    m("div"),
                    m("div", {
                        onclick: setUserOption(
                            serviceid,
                            location.peer_id,
                            !isOn
                        ),
                        style: {
                            float:"left",
                        },
                    },createSwitch(isOn)),
                    m("div",
                        {
                            style: {
                                //color: "lime",
                                float:"left",
                                marginLeft: "5px",
                                marginRight: "5px",
                                fontWeight: "bold",
                            }
                        },
                        peer.name + (location.location
                            ? " (" + location.location + ")"
                            : "")
                    ),
                    m("div", {
                        style: {
                            clear: "left"
                        }
                    }),
                ]);
            })
        }, rs.sort("name")))
    ]);
}


module.exports = {
    view: function(){
        if (m.route.param("service_id")) {
            return serviceView(m.route.param("service_id"));
        }
        return m("div", [
            m("h2", breadcrums("rights", {
                settings:"/settings",
            })),
            m("hr"),
            m("ul", rs.list("servicecontrol", function(item){
                    return m("li", {
                        style: {
                            margin: "5px",
                            color: item.default_allowed ? "lime" :"red",
                        }
                    }, [
                        m("div"),
                        m("div", {
                            onclick: setOption(
                                item.service_id,
                                !item.default_allowed
                            ),
                            style: {
                                float:"left",
                            }
                        },createSwitch(item.default_allowed)),
                        m("div.menu",
                            {
                                style: {
                                    // color: "lime",
                                    borderColor: item.default_allowed
                                        ? "lime"
                                        : "red",
                                    float: "left",
                                    marginLeft: "5px",
                                    marginRight: "5px",
                                    paddingLeft: "2px",
                                    paddingRight: "2px",
                                },
                                onclick: function(){
                                    m.route("/settings/servicecontrol/", {
                                        service_id: item.service_id,
                                    })
                                }
                            }, "more"
                        ),
                        m("div",
                            {
                                style: {
                                    // color: "lime",
                                    float:"left",
                                    marginLeft: "5px",
                                    marginRight: "5px",
                                    fontWeight: "bold",
                                }
                            },
                            item.service_name
                        ),
                        m("div",
                            {
                                style: {
                                    color: "lime",
                                    float:"left",
                                    marginLeft: "5px",
                                    marginRight: "5px",
                                }
                            },
                            (
                                item.default_allowed
                                ? ( item.peers_denied != null
                                    ? "(" + item.peers_denied.length + " denied)"
                                    : "")
                                : ( item.peers_allowed != null
                                    ? "(" + item.peers_allowed.length + " allowed)"
                                    : "")
                            )
                        ),
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


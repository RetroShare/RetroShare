"use strict";

var m = require("mithril");
var rs = require("retroshare");

module.exports = {view: function(){
    var peers = rs("peers");
    //console.log("peers:" + peers);

    //waiting for peerlist ...
    if(peers === undefined || peers == null){
        return m("div",[
            m("h2","peers"),
            m("h3","waiting_server"),
        ]);
    };
    peers = peers.sort(rs.sort("name"));

    //building peerlist (prebuild for counting)
    var online = 0;
    var peerlist = peers.map(function(peer){
        var isonline = false;
        var avatar_address ="";

        //building location list (prebuild for state + icon)
        var loclist = peer.locations.map(function(location){
            if (location.is_online && ! isonline){
                online +=1;
                isonline = true;
            }
            if (location.avatar_address != "" && avatar_address =="") {
                avatar_address=location.avatar_address;
            }
            return m("li",{
                style:"color:" + (location.is_online ? "lime": "grey")
                    + ";cursor:pointer",
                onclick: function(){
                    m.route("/chat?lobby=" + location.chat_id)
                }

            }, location.location);
        });

        //return friend (peer + locations)
        return m("div.flexbox[style=color:lime]",[
            // avatar-icon
            m("div", [
                avatar_address == "" ? "" : (
                    m("img",{
                        src: rs.apiurl("peers" + avatar_address),
                        style:"border-radius:3mm;margin:2mm;",
                    })
                )
            ]),
            //peername + locations
            m("div.flexwidemember",[
                m("h1[style=margin-bottom:1mm;]",
                    {style:"color:" + (isonline ? "lime": "grey")} ,
                    peer.name
                ),
                m("ul", loclist ),
            ]),
            //remove-button
            m("div", {
                style: "color:red;" +
                    "font-size:1.5em;" +
					"padding:0.2em;" +
					"cursor:pointer",
				onclick: function (){
				    var yes = window.confirm(
				        "Remove " + peer.name + " from friendslist?");
				    if(yes){
					    rs.request("peers/" + peer.pgp_id +"/delete");
				    }
				}
            }, "X")
        ]);
    });

    // return add-peer-button + peerlist
    return m("div",[
        m("div.btn2",{onclick: function(){m.route("/addpeer")}},"add new friend"),
        m("h2","peers (online: " + online  +" / " + peers.length + "):"),
        m("div", [
            peerlist,
        ]),
    ]);
}
};

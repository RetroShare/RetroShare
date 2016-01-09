"use strict";

var m = require("mithril");
var rs = require("retroshare");

module.exports = {view: function(){
    var peers = rs("peers");
    console.log("peers:" + peers);
    if(peers === undefined){
        return m("div",[
            m("h2","peers"),
            m("h3","waiting_server"),
        ]);
    };
    var online = 0;
    var peerlist = peers.map(function(peer){
        var isonline = false;
        var loclist = peer.locations.map(function(location){
            if (location.is_online && ! isonline){
                online +=1;
                isonline = true;
            }
            return m("li",
                {style:"color:" + (location.is_online ? "lime": "grey")},
                location.location);
        });
        return [
            m("h2", {style:"color:" + (isonline ? "lime": "grey")} ,peer.name),
            m("ul", loclist ),
        ];
    });

    return m("div",[
        m("div.btn2",{onclick: function(){m.route("/addpeer")}},"add new friend"),
        m("h2","peers ( online: " + online  +" / " + peers.length + "):"),
        m("table", [
            peerlist,
        ]),
    ]);
}
};

"use strict";

var m = require("mithril");
var rs = require("retroshare");

module.exports = {view: function(){
    var peers = rs("peers");
    console.log("peers:" + peers);
    if(peers === undefined){
        return m("div", "waiting_server");
    }
    return m("div", peers.map(function(peer){
        return m("div",peer.name)
    }));
}
};

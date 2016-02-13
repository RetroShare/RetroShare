"use strict";

var m = require("mithril");
var rs = require("retroshare");

function msg(from, when, text){
    return m(".chat.msg.container",[
        m(".chat.msg.from", from),
        m(".chat.msg.when", when),
        m(".chat.msg.text", text),
    ]);
}

function lobbies(){
    var lobs = rs("chat/lobbies");
    if (lobs === undefined) {
        return m("div","loading ...")
    };
    var dta = lobs.map(function (lobby){
        return m("div.btn",{
            title: "topic: " + lobby.topic + "\n"
                + "subscribed: " + lobby.subscribed,
            style: {
                backgroundColor: lobby.subscribed ? 'blue' : 'darkred',
            },
            onclick: function(){
                m.route("/chat?lobby=" + lobby.chat_id);
            }
                    },lobby.name + (lobby.unread_msg_count > 0 ? ("(" + lobby.unread_msg_count + ")") : ""));
    });
    return dta;

}

function getLobbyDetails(lobbyid){
    var lobs = rs("chat/lobbies");
    if (lobs === undefined) {
        return null;
    };
    for(var i = 0, l = lobs.length; i < l; ++i) {
        if (lobs[i].chat_id == lobbyid) {
            return lobs[i];
        }
    }
    return null;
}

function lobby(lobbyid){
    var lobby = getLobbyDetails(lobbyid);
    if (lobby == null) {
        return m("div","waiting ...");
    }
    var msgs = rs("chat/messages/" + lobbyid);
    var info = rs("chat/info/" + lobbyid);
    if (msgs === undefined || info === undefined) {
        return m("div","waiting ...");
    }
    var intro = [
            m("h2",lobby.name),
            m("p",lobby.topic),
            m("hr")
    ]
    if (!lobby.subscribed) {
        return [
            intro,
            m("b","select subscribe identity:"),
            m("p"),
            rs.list("identity/own", function(item){
                return m("div.btn2",{
                    onclick:function(){
                        console.log("subscribe - id:" + lobby.id +", "
                            + "gxs_id:" + item.gxs_id)
			            rs.request("chat/subscribe_lobby",{
			                id:lobby.id,
			                gxs_id:item.gxs_id
			            })
                    }
                },"subscribe as " + item.name);
            }),
        ];
    }
    return [
        intro,
        msgs.map(function(item){
            var d = new Date(new Number(item.send_time)*1000);
            return msg(
                item.author_name,
                d.toLocaleDateString() + " " + d.toLocaleTimeString(),
                item.msg
            );
        })
    ];
}

module.exports = {
    frame: function(content, right){
        return m(".chat.container", [
            m(".chat.header", "headerbar"),
            m(".chat.left", [
                m("div.chat.header[style=position:relative]","lobbies:"),
                m("br"),
                lobbies(),
            ]),
            m(".chat.right", right),
            m(".chat.middle", content),
            m(".chat.clear", ""),
        ]);
    },
    view: function(){
        var lobbyid = m.route.param("lobby");
        if (lobbyid != undefined ) {
            return this.frame(
                lobby(lobbyid),[]
            );
        };
        return this.frame(
                m("div", "please select lobby"),
                m("div","right"));
    }
}

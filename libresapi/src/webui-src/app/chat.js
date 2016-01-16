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
            onclick: function(){
                m.route("/chat?lobby=" + lobby.chat_id);
            }
                    },lobby.name + (lobby.unread_msg_count > 0 ? ("(" + lobby.unread_msg_count + ")") : ""));
    });
    return dta;

}

function lobby(lobbyid){
    var msgs = rs("chat/messages/" + lobbyid);
    var info = rs("chat/info/" + lobbyid);
    if (msgs === undefined || info === undefined) {
        return m("div","waiting ...");
    }
    return msgs.map(function(item){
        var d = new Date(new Number(item.send_time)*1000);
        return msg(item.author_name, d.toLocaleDateString() + " " + d.toLocaleTimeString(),item.msg);
    });
}

module.exports = {
    frame: function(content, right){
        return m(".chat.container", [
            m(".chat.header", "headerbar"),
            m(".chat.left", [
                m("div.chat.header","lobbies:"),
                m("hr"),
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

"use strict";

var m = require("mithril");
var rs = require("retroshare");

var msg = null;

function dspmsg(from, when, text){
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

function sendmsg(msgid){
    var txtmsg = document.getElementById("txtNewMsg");
	rs.request("chat/send_message", {
		chat_id: msgid,
		msg: txtmsg.value
	});
    txtmsg.value="";
}

function lobby(lobbyid){
    var msgs;
    var lobby = getLobbyDetails(lobbyid);
    var info = rs("chat/info/" + lobbyid);
    if (lobby == null || info === undefined) {
        return m("div","waiting ...");
    }

    var mem = rs.memory("chat/info/" + lobbyid);
    if (mem.msg === undefined) {
        mem.msg = [];
    };

    var reqData = {};
    if (mem.lastKnownMsg != undefined) {
        reqData.begin_after = mem.lastKnownMsg;
    }

    rs.request("chat/messages/" + lobbyid, reqData, function (data) {
        mem.msg = mem.msg.concat(data);
        if (mem.msg.length > 0) {
            mem.lastKnownMsg = mem.msg[mem.msg.length -1].id;
        }
        if (data.length > 0 ) {
            rs.request("chat/mark_chat_as_read/" + lobbyid,{}, null,
                {allow: "ok|not_set"});
        }
    }, {
        onmismatch: function (){},
        log:function(){} //no logging (pulling)
    });

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
    } else {
        msg = m(".chat.bottom",[
            m("div","enter new message:"),
            m("input",{
                id:"txtNewMsg",
                onkeydown: function(event){
                    if (event.keyCode == 13){
                        sendmsg(lobbyid);
                    }
                }
            }),
            m("div.btn2", {
                style: {textAlign:"center"},
                onclick: function(){
                    sendmsg(lobbyid);
                }
            },"submit")
        ]);
    }
    return [
        intro,
        mem.msg.map(function(item){
            var d = new Date(new Number(item.send_time)*1000);
            return dspmsg(
                item.author_name,
                d.toLocaleDateString() + " " + d.toLocaleTimeString(),
                item.msg
            );
        })
    ];
}

module.exports = {
    frame: function(content, right){
        return m("div", [
            m(".chat.container", [
                m(".chat.header", [
                    m(
                        "h2",
                        {style:{margin:"0px"}},
                        "chat"
                    )
                ]),
                m(".chat.left", [
                    m("div.chat.header[style=position:relative]","lobbies:"),
                    m("br"),
                    lobbies(),
                ]),
                m(".chat.right", right),
                m(".chat.middle", content),
                m(".chat.clear", ""),
            ]),
            msg != null
            ? msg
            : [],
        ]);
    },
    view: function(){
        var lobbyid = m.route.param("lobby");
        msg = null;
        if (lobbyid != undefined ) {
            return this.frame(
                lobby(lobbyid),
                []
            );
        };
        return this.frame(
            m(
                "div",
                {style: {margin:"10px"}},
                "please select lobby"
            ),
            m("div","right")
        );
    }
}

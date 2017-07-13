"use strict";

var m = require("mithril");
var rs = require("retroshare");

var msg = null;
var particips = [];

function dspmsg(from, when, text){
    return m(".chat.msg.container",[
        m(".chat.msg.from", from),
        m(".chat.msg.when", when),
        m(".chat.msg.text", text),
    ]);
}

function lobbies(){
    return [
        rs.list("chat/lobbies",function(lobby){
            return m("div.btn",{
                title: "topic: " + lobby.topic + "\n"
                    + "subscribed: " + lobby.subscribed,
                style: {
                    backgroundColor: lobby.subscribed ? 'blue' : 'darkred',
                },
                onclick: function(){
                    m.route("/chat?lobby=" + lobby.chat_id);
                }
            },
            lobby.name + (
                lobby.unread_msg_count > 0
                ? ("(" + lobby.unread_msg_count + ")")
                : "")
            );
        },
            rs.sort.bool("is_broadcast",
                rs.sort.bool("subscribed",
                    rs.sort("name")))
        ),
        m("br"),
        m("h3","peers:"),
        rs.list("peers",function(peer){
            return peer.locations.map(function(loc){
                if (loc.location == "") {
                    return [];
                };
                return m("div.btn",{
                    style: {
                        backgroundColor: loc.is_online ? 'blue' : 'darkred',
                    },
                    onclick: function(){
                        m.route("/chat?lobby=" + loc.chat_id);
                    }
                },
                peer.name + " / " + loc.location  + (
                    loc.unread_msgs > 0
                    ? ("(" + loc.unread_msgs + ")")
                    : "")
                );
            })
        })
    ];
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
    var peers = rs("peers");
    if (peers === undefined) {
        return null;
    };
    for(var i = 0, l = peers.length; i < l; ++i) {
        var peer = peers[i];
        for(var i1 = 0, l1 = peer.locations.length; i1 < l1; ++i1) {
            if (peer.locations[i1].chat_id == lobbyid) {
                return peer.locations[i1];
            }
        }
    }
    return null;
}


function find_next(searchValue){
    var els = document.getElementsByClassName("chat msg text");
    var middle = document.getElementsByClassName("chat middle")[0];
    var find_hidden = document.getElementById("LastWordPos");
    var start_index = Number(find_hidden.innerText);
    
	if(!Number.isInteger(start_index)){
        console.log(start_index + " is Not Integer");
        start_index = 0;
        }
	if(start_index >  els.length)
    	start_index = 0;
        
    console.log(start_index);
        
  for (var i = start_index; i < els.length; i++) {
  var start = els[i].innerHTML.indexOf(searchValue);
    if ( start > -1) {
      //match has been made   
      middle.scrollTop = els[i].parentElement.offsetTop - middle.clientHeight/2;
      var end = searchValue.length + start;
      setSelectionRange(els[i], start, end);
      find_hidden.innerText = i + 1;
      break;
    }
  }
}

function setSelectionRange(el, start, end) {
    if (document.createRange && window.getSelection) {
        var range = document.createRange();
        range.selectNodeContents(el);
        var textNodes = getTextNodesIn(el);
        var foundStart = false;
        var charCount = 0, endCharCount;

        for (var i = 0, textNode; textNode = textNodes[i++]; ) {
            endCharCount = charCount + textNode.length;
            if (!foundStart && start >= charCount && (start < endCharCount || (start == endCharCount && i <= textNodes.length))) {
                range.setStart(textNode, start - charCount);
                foundStart = true;
            }
            if (foundStart && end <= endCharCount) {
                range.setEnd(textNode, end - charCount);
                break;
            }
            charCount = endCharCount;
        }

        var sel = window.getSelection();
        sel.removeAllRanges();
        sel.addRange(range);
    } else if (document.selection && document.body.createTextRange) {
        var textRange = document.body.createTextRange();
        textRange.moveToElementText(el);
        textRange.collapse(true);
        textRange.moveEnd("character", end);
        textRange.moveStart("character", start);
        textRange.select();
    }
}

function getTextNodesIn(node) {
    var textNodes = [];
    if (node.nodeType == 3) {
        textNodes.push(node);
    } else {
        var children = node.childNodes;
        for (var i = 0, len = children.length; i < len; ++i) {
            textNodes.push.apply(textNodes, getTextNodesIn(children[i]));
        }
    }
    return textNodes;
}

function sendmsg(msgid){
    var txtmsg = document.getElementById("txtNewMsg");
    var remove_whitespace = txtmsg.value.replace(/(\r\n|\n|\r|\s)+/g,'');
	if( remove_whitespace == '')
		return;
	rs.request("chat/send_message", {
		chat_id: msgid,
		msg: txtmsg.value
	});
    txtmsg.value="";
}

function lobby(lobbyid){
    var msgs;
    var lobdt = getLobbyDetails(lobbyid);
    var info = rs("chat/info/" + lobbyid);
    if (lobdt == null || info === undefined) {
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
        if (data.length > 0 ) {
            mem.msg = mem.msg.concat(data);
            if (mem.msg.length > 0) {
                mem.lastKnownMsg = mem.msg[mem.msg.length -1].id;
            }
            rs.request("chat/mark_chat_as_read/" + lobbyid,{}, null,
                {allow: "ok|not_set"});
        } else {
            mem.msg = [];
        }
    }, {
        onmismatch: function (){},
        log:function(){} //no logging (pulling)
    });

    var intro = [
            //m("h2",lobdt.name),
            m("p",lobdt.topic ? lobdt.topic: lobdt.location),
            m("hr")
    ]
    if (lobdt.subscribed != undefined && !lobdt.subscribed) {
        return [
            intro,
            m("b","select subscribe identity:"),
            m("p"),
            rs.list("identity/own", function(item){
                return m("div.btn2",{
                    onclick:function(){
                        console.log("subscribe - id:" + lobdt.id +", "
                            + "gxs_id:" + item.gxs_id)
			            rs.request("chat/subscribe_lobby",{
			                id:lobdt.id,
			                gxs_id:item.gxs_id
			            })
                    }
                },"subscribe as " + item.name);
            }),
        ];
    } else {

		var el = document.getElementById("CharLobbyName");
        el.innerText = lobdt.name;
		
        msg = m(".chat.bottom",[
            m("div","enter new message, Ctrl+Enter to submit:"),
            m("textarea",{
                id:"txtNewMsg",
                onkeydown: function(event){
                    if (event.ctrlKey && event.keyCode == 13){
                        sendmsg(lobbyid);
                    }
                }
            }),
            m("table.noBorderTable", [
                m("tr",[
                    m("td.noBorderTD",[
                        m("div.btnSmall", {
                            style: {textAlign:"center"},
                            onclick: function(){
                               var els = document.getElementsByClassName("chat middle");
                               var el = els[0];
                                   el.scrollTop = el.scrollHeight - el.clientHeight;
                            }
                        },"bottom")
                    ]),
                    m("td.noBorderTD",[
                        m("div.btnSmall", {
                            style: {textAlign:"center"},
                            onclick: function(){
                                sendmsg(lobbyid);
                            }
                        },"submit")
                    ]),
					m("td.noBorderTD",[
						m("input",{
							id:"txtMsgKeyword"
						})
					]),
					m("td.noBorderTD", [
						m("div.btnSmall", {
							style: {textAlign:"center"},
							onclick: function(){
								var key = document.getElementById("txtMsgKeyword");
								var txtkeyword = key.value;
								find_next(txtkeyword);
							}
						},"Find")
					])
				])
            ]),
            m("div.hidden", {
                 id: "LastWordPos"
                },""
            ),
        ]);
    }
    if (lobdt.subscribed != undefined
        && lobdt.subscribed
        && !lobdt.is_broadcast
        ) {
        //set participants
        particips = [
            m("div.btn", {
                style: {
                    "text-align":"center"
                },
                onclick: function (){
                    rs.request("chat/unsubscribe_lobby",{
                        id:lobdt.id,
                    });
                    m.route("/chat");
                }
            },"unsubscribe"),
            m("div.btn", {
                style: {
                    "text-align":"center"
                },
                onclick: function (){
                    rs.request("chat/clear_lobby",{
                        id:lobdt.id,
                    });
                    m.route("/chat?lobby=" + lobbyid);
                }
            },"clear"),
            m("h3","participants:"),
            rs.list(
                "chat/lobby_participants/" + lobbyid,
                function(item) {
                    return m("div",item.identity.name);
                },
                function (a,b){
                    return rs.stringSort(a.identity.name,b.identity.name);
                }
            )
        ]
    } else {
        if (lobdt.subscribed != undefined
            && lobdt.subscribed
            && lobdt.is_broadcast
            ) {
            //set participants
            particips = [
                m("div.btn", {
                    style: {
                        "text-align":"center"
                    },
                    onclick: function (){
                        rs.request("chat/clear_lobby",{
                            lobbyid,
                        });
                        m.route("/chat?lobby=" + lobbyid);
                    }
                },"clear"),
            ]
        }
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
        return m("div", {
            style: {
                "height": "100%",
                "box-sizing": "border-box",
                "padding-bottom": "170px",
            }
        },[
            m(".chat.container", [
                m(".chat.header", [
                    m("table.noBorderTable",[
                        m("tr",[
                            m("td.noBorderTD",[
                                m(
                                    "h2",
                                    {style:{margin:"0px"}},
                                    "chat"
                                )
                            ]),
                            m("td.noBorderTD",[
                                m(
                                    "h2",
                                    {
                                        style:{margin:"0px"},
                                        id:"CharLobbyName"
                                    },
                                    "Lobby Name"
                                )
                            ])
                        ])
                    ])
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
            particips = [];
            return this.frame(
                lobby(lobbyid),
                particips
            );
        };
        return this.frame(
            m(
                "div",
                {style: {margin:"10px"}},
                "please select lobby"
            ),
            m("div","")
        );
    }
}

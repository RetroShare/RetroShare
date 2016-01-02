"use strict";

var m = require("mithril");

function msg(from, when, text){
    return m(".chat.msg.container",[
        m(".chat.msg.from", from),
        m(".chat.msg.when", when),
        m(".chat.msg.text", text),
    ]);
}

module.exports = {
    view: function(){
        return m(".chat.container", [
            m(".chat.header", "headerbar"),
            m(".chat.left", "left"),
            m(".chat.right", "right"),
            m(".chat.middle", [
                msg("Andi", "now", "Hallo"),
                msg("Test", "now", "Hallo back"),
                msg("Somebody", "now", "Hallo back, sfjhfu dsjkchsd wehfskf sdjksdf sjdnfkjsf sdjkfhjksdf jksdfjksdnf sjdefhsjkn cesjdfhsjk fskldcjhsklc ksdj"),
            ]),
            m(".chat.clear", ""),
        ]);
    }
}
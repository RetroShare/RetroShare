"use strict";

var m = require("mithril");
var rs = require("retroshare");

function cancel(){
    rs.memory("control/locations").curraccount=null;
    m.redraw();
}

function selAccount(account){
    rs.memory("control/locations").curraccount=account;
    m.redraw();
    rs.request("control/login", {id: account.id}, function(){
        console.log("login sent");
    });
}

function curraccount() {
    var mem;
    mem = rs.memory("control/locations");
    if (mem.curraccount === undefined) {
        return null;
    }
    return mem.curraccount;
}

module.exports = {view: function(){
    var accounts = rs("control/locations");
    if(accounts === undefined || accounts == null){
        return m("div", "accounts: waiting_server");
    }
    if (curraccount() == null) {
        return m("div", [
    	    m("h2","login:"),
            m("hr"),
    	    accounts.map(function(account){
    	    	return [
    	    	    m("div.btn2", {
    	    	        onclick: function(){
    	    	            selAccount(account)
    	    	        }
    	    	    },
    	    	    account.location + " (" + account.name + ")"),
    	    	    m("br")
    	    	]
            })
        ]);
    } else {
//        rs.untoken("control/password");
        return m("div", [
            m("div", [
                "logging in ...",
                m("br"),
                "(waiting for password-request)",
            ]),
            /*
            m("hr"),
            m(".btn2", {
                onclick: function() {
                    cancel();
                }
            },"Cancel " + curraccount().name + " login "),
            */
            ]);
    }
}
};

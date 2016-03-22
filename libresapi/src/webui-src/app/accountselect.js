"use strict";

var m = require("mithril");
var rs = require("retroshare");

var curraccount = null;
var accountMap = new Map();

function cancel(){
    curraccount=null;
    m.redraw();
}

function selAccount(account){
    curraccount=accountMap.get(account);
    m.redraw();
    rs.request("control/login", {id: curraccount.id}, function(){
        console.log("login sent");
        rs.clearCache();
    });
}

module.exports = {view: function(){
    var accounts = rs("control/locations");
    if(accounts === undefined || accounts == null){
        return m("div", "accounts: waiting_server");
    }
    if (curraccount == null) {
    	accountMap.clear();
        return m("div", [
    	    m("h2","login:"),
            m("hr"),
    	    accounts.map(function(account){
    	    	accountMap.set(account.id,account);
    	    	return [
    	    	    m("div.btn2", {
    	    	        onclick: m.withAttr("account", selAccount),
    	    	        account:account.id
    	    	    },
    	    	    account.location + " (" + account.name + ")"),
    	    	    m("br")
    	    	]
            })
        ]);
    } else {
        return m("div", "logging in ..." );
    }
}
};

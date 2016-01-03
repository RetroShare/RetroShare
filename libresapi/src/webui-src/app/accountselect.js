"use strict";

var m = require("mithril");
var rs = require("retroshare");

var curraccount = null;
var currentpasswd = null;
var accountMap = new Map();

function login(){
    alert("login:" + curraccount.location + "; passwort: " + currentpasswd);
}

function cancel(){
    curraccount=null;
    m.redraw();
}

function selAccount(account){
    curraccount=accountMap.get(account);
    m.redraw();
}

function setPasswd(password) {
    currentpasswd = password
}

module.exports = {view: function(){
    var accounts = rs("control/locations");
    console.log("accounts: " + accounts);
    if(accounts === undefined){
        return m("div", "accounts: waiting_server");
    }
    if (curraccount == null) {
    	accountMap.clear();
        return m("div", [
    	    m("h2","accounts:"),
            m("hr"),
    	    accounts.map(function(account){
    	    	console.log("adding: " + account.id);
    	    	accountMap.set(account.id,account);
    	    	return [
    	    	    m("div", {onclick: m.withAttr("account", selAccount), account:account.id }, account.location + " (" + account.name + ")"),
    	    	    m("br")
    	    	    ]
        })]);
    } else {
        return m("div", [
    	    m("h2","login:"),
            m("hr"),
    	    m("h2","account:" + curraccount.location  + " (" + curraccount.name + ")"),
    	    m("input",{type:"password", onchange:m.withAttr("value", setPasswd)}),
    	    m("br"),
    	    m("button",{onclick: function(){login();}},"login"),
    	    m("button",{onclick: function(){cancel();}},"cancel")
    	    ]);
    }
}
};

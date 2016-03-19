"use strict";

var m = require("mithril");
var rs = require("retroshare");

function createidentity(){
    var data = {
        name: document.getElementById("txtname").value,
        pgp_linked: false,
        //document.getElementById("chklinked").checked,
    };
    m.route("/waiting");
    rs.request("identity/create_identity",data, function(){
        m.route("/identities");
    })
}

module.exports = {view: function(){
    m.initControl = "txtname";
    return m("div",
        m("h2","create identity"),
        m("hr"),
        m("h3","name"),
        m("input", {
            type: "text",
            id: "txtname",
            /*
            onkeydown: function(event){
                if (event.keyCode == 13){
                    setPasswd(this.value);
                    sendPassword(needpasswd);
                }
            }
            */
        }),
        /*
        m("b","linked with pgp-id: "),
        m("input", {
            type: "checkbox",
            id: "chklinked",
            style: {
                fontweight:"bold",
                width: "0%",
            }
        }),
        */
        m("p"," "),
        m("input.btn2", {
            onclick: createidentity,
            type: "button",
            value: "create new identity",
        })
    )
}}

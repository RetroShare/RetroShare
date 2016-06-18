var m = require("mithril");
var rs = require("retroshare");

var locationName = "";
var password ="";
var ssl_name = "";
var newName = "";

function listprofiles(){
    var locations = rs("control/locations");
    var knownProfileIds = [];
    var result = [];
    if(locations === undefined || locations == null){
        return m("div", "profiles: waiting_server");
    }
    locations.map(function(location) {
        if (knownProfileIds.indexOf(location.pgp_id)<0){
            knownProfileIds.push(location.pgp_id);
            result.push(m(
                "div.btn2",{
                    onclick: function(){
                        m.route("/createlogin",{
                            id: location.pgp_id,
                            name: location.name,
                        })
                    }
                },
                location.name
            ));
        }
    });

    return result;
}

function setLocationName(location) {
    locationName = location;
}

function setPasswd(passwd) {
    password = passwd;
}

function setSslName(ssl) {
    ssl_name = ssl;
}

function setNewName(name) {
    newName = name;
}

function checkpasswd(){
    var status = "";
    var color = "red";
    var lbl = document.getElementById("lblpwdinfo");
    var passwd2 = document.getElementById("txtpasswd2").value;
    if (passwd2 == password && passwd2 != "") {
        color = "lime";
        status = "password ok";
    } else if (passwd2 == "") {
        color = "yellow";
        status = "password required";
    } else {
        color = "red";
        status = "passwords don't match";
    }
    lbl.textContent = status;
    lbl.style.color=color;
}


function createLocation() {
    var profile = m.route.param("id");
    var profname = m.route.param("name");

    var loc ={
        ssl_name: document.getElementById("txtlocation").value,
        pgp_password: password,
    };
    if (profile != undefined) {
        loc.pgp_id= profile;
    } else {
        loc.pgp_name = newName;
    };
    rs.request("control/create_location",loc,function(data){
        m.route("/accountselect", {});
    });
    m.route("/createlogin",{state:wait});
}

function certDrop(event)
{
	console.log("onDrop()");
	console.log(event.dataTransfer.files);
	event.preventDefault();

	var reader = new FileReader();

	var widget = this;

	reader.onload = function(evt) {
		console.log("onDrop(): file loaded");
		rs.request(
		    "control/import_pgp",{
		        key_string:evt.target.result,
		    }, importCallback);
	};
	reader.readAsText(event.dataTransfer.files[0]);
	m.route("/createlogin",{state:"waiting"});
}

function importCallback(resp)
{
	console.log("importCallback()" + resp);
	m.route("/createlogin",{
	    id:resp.pgp_id,
	    name:"",
	});
}


module.exports = {
    view: function(){
        var profile = m.route.param("id");
        var state = m.route.param("state");
        var profname = m.route.param("name");
        var hidden = m.route.param("hidden");
        if (state == "wait"){
            return m("div","waiting ...");
        } if (state == "newid"){
            m.initControl = "txtnewname";
            return m("div",[
                m("h2","create login - Step 2 / 2: create location"),
                m("h3","- for new profile "),
                m("hr"),
                m("h2","PGP-profile name:"),
                m("input",{
                    id: "txtnewname",
                    type:"text",
                    onchange:m.withAttr("value", setNewName),
                    onkeydown: function(event){
                        if (event.keyCode == 13){
                            document.getElementById("txtpasswd").focus();
                        }
                    },
                }),
                m("h2","enter password:"),
                m("input", {
                    id: "txtpasswd",
                    type:"password",
                    onchange: m.withAttr("value",setPasswd),
                    onkeydown: function(event){
                        if (event.keyCode == 13){
                            setPasswd(this.value);
                            document.getElementById("txtpasswd2").focus();
                        };
                        checkpasswd;
                    }
                }),
                m("h2", "re-enter password:"),
                m("input", {
                    id: "txtpasswd2",
                    type:"password",
                    onfocus: checkpasswd,
                    onchange: checkpasswd,
                    onkeyup: function(event){
                        if (event.keyCode == 13){
                            document.getElementById("txtlocation").focus();
                        }
                        checkpasswd();
                    }
                }),
                m("h3",{
                    id: "lblpwdinfo",
                    style:"color:yellow",
                }, "password required"),
                m("h2","location name:"),
                m("input",{
                    id: "txtlocation",
                    type:"text",
                    onchange:m.withAttr("value", setLocationName),
                    onkeydown: function(event){
                        if (event.keyCode == 13){
                            setSslName(this.value);
                            createLocation();
                        }
                    },
                }),
                m("br"),
                m("input",{
                    type: "button",
                    onclick: createLocation,
                    value: "create location",
                }),
            ]);
        } else if (profile != undefined) {
            m.initControl = "txtpasswd";
            return m("div",[
                m("h2","create login - Step 2 / 2: create location"),
                m("h3","- for " + profname + " (" +profile + ")"),
                m("hr"),
                m("h2","enter password:"),
                m("input", {
                    id: "txtpasswd",
                    type:"password",
                    onchange: m.withAttr("value",setPasswd),
                    onkeydown: function(event){
                        if (event.keyCode == 13){
                            setPasswd(this.value);
                            document.getElementById("txtlocation").focus();
                        }
                    }
                }),
                m("h2","location name:"),
                m("input",{
                    id: "txtlocation",
                    type:"text",
                    onchange:m.withAttr("value", setLocationName),
                    onkeydown: function(event){
                        if (event.keyCode == 13){
                            setSslName(this.value);
                            createLocation();
                        }
                    },
                }),
                m("br"),
                m("input",{
                    type: "button",
                    onclick: createLocation,
                    value: "create location",
                }),
            ]);
        } else {
            return m("div",[
                m("h2","create login - Step 1 / 2: select profile(PGP-ID)"),
                m("hr"),
                m("div.btn2",{
                    onclick: function(){
                        m.route("/createlogin", {state: "newid"});
                    },
                } ,"<create new profile>"),
                m("div.btn2",{
                	ondragover:function(event){
                	    /*important: block default event*/
                	    event.preventDefault();
                	},
					ondrop: certDrop,
                } ,"<import profile (drag and drop a profile here)>"),
                listprofiles()
            ]);
        };

    }
}

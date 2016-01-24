var m = require("mithril");
var rs = require("retroshare");

var locationName = "";
var password ="";

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


function createLocation(loc) {
    rs.request("control/create_location",loc,function(data){
        m.route("/accountselect", {});
    });
    m.route("/createlogin",{state:wait});
}


module.exports = {
    view: function(){
        var profile = m.route.param("id");
        var state = m.route.param("state");
        var profname = m.route.param("name");
        var hidden = m.route.param("hidden");
        if (state == "wait"){
            return m("div","waiting ...");
        } else if (profile === undefined) {
            return m("div",[
                m("h2","create login - Step 1 / 2: select profile(PGP-ID)"),
                m("hr"),
                m("div.btn2",{} ,"<create new profile>"),
                m("div.btn2",{} ,"<import profile (drag and drop a profile here)>"),
                listprofiles()
            ]);
        };
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
                        createLocation({
                            ssl_name: this.value,
                            pgp_id: profile,
                            pgp_password: password,
                        });
                    }
                },
            }),
            m("br"),
            m("input",{
                type: "button",
                onclick: function(){
                    createLocation(locationName);
                },
                value: "create location",
            }),
        ]);
    }
}

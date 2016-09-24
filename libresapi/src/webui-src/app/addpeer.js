"use strict";

var m = require("mithril");
var rs = require("retroshare");

var newkey = "";
var remote = "";

module.exports = {
    view: function(){
        var key = m.route.param("radix");
        var pgp = m.route.param("pgp_id");
        var peer_id =m.route.param("peer_id");

        if (key===undefined && pgp === undefined) {

            var owncert = rs("peers/self/certificate");
            if (owncert === undefined ) {
                owncert = {cert_string:"< waiting for server ... >"}
            }
            return m("div", [
                m("h2","add new friend (Step 1/3)"),
                m("p","Your own key, give it to your friends"),
                m("pre", owncert.cert_string),
                m("p","paste your friends key below"),
			    m("textarea", {
                    ref:"cert",
			        cols:"70",
			        rows:"16",
			        onchange: m.withAttr("value", function(value){newkey=value;})
			    }),
			    m("br"),
			    m("input.btn2",{
			        type:"button",
			        value:"read",
			        onclick: function (){
			            m.route("/addpeer",{radix:newkey})
			        },
			    })
            ]);
        } else if (pgp === undefined) {
            rs.request("peers/examine_cert",{cert_string:key},
                function(data,responsetoken){
                    data.radix=key;
                    m.route("/addpeer", data);
                }
            );
            return m("div", [
                m("h2","add new friend (Step 2/3)"),
                m("div", "analyse cert, please wait for server ...")
                // { data: null, debug_msg: "failed to load certificate ", returncode: "fail" }
            ]);
        } else {
            var result = {
                cert_string:key ,
                flags:{
                    allow_direct_download:false,
                    allow_push:false,
					// set to false, until the webinterface supports managment of the blacklist/whitelist
					require_whitelist: false,
                }
            };
            return m("div",[
                m("h2","add new friend (Step 3/3)"),
                m("p","Do you want to add "
                    + m.route.param("name")
                    + " (" + m.route.param("location") + ")"
                    + " to your friendslist?"),
                m("input.checkbox[type=checkbox]", {
                    onchange: m.withAttr("checked", function(checked){
                        result.flags.allow_direct_download=checked;
                    })
                }), "Allow direct downloads from this node",
                m("br"),
                m("input.checkbox[type=checkbox]", {
                    onchange: m.withAttr("checked", function(checked){
                        result.flags.allow_push=checked;
                    })
                }), "Auto download recommended files from this node",
                m("div.btn2",{
                    onclick: function(){
                        m.route("/waiting");
                        rs.request("peers",result,function(data, responsetoken){
                            m.route("/peers");
                        })
                    }
                },"add to friendslist")

            ])
        }
    }
};

/*

    return "peers/self/certificate"



		RS.request({path: "peers/examine_cert", data: {cert_string: cert_string}}, this.examine_cert_callback);
		this.setState({page:"waiting", cert_string: cert_string});


		RS.request(
		{
			path: "peers",
			data: {
				cert_string: this.state.cert_string,
				flags:{
					allow_direct_download: this.refs.cb_direct_dl.getDOMNode().checked,
					allow_push: this.refs.cb_push.getDOMNode().checked,
					// set to false, until the webinterface supports managment of the blacklist/whitelist
					require_whitelist: false,
				}
			}
		});


	render: function(){
		if(this.state.page === "start")
			return(
				<div>
					<p>Your own key, give it to your friends</p>
					<OwnCert/>
					<p>paste your friends key below</p>
			        <textarea ref="cert" cols="70" rows="16"></textarea><br/>
			        <input
			          type="button"
			          value="read key"
			          onClick={this.add_friend_handler}
			        />
				</div>
			);
		if(this.state.page === "waiting")
			return(
				<div>
			        waiting for response from server...
				</div>
			);
		if(this.state.page === "peer")
			return(
				<div>
			        <p>Do you want to add {this.state.data.name} to your friendslist?</p>
			        <input className="checkbox" type="checkbox" ref="cb_direct_dl"/> Allow direct downloads from this node<br/>
			        <input className="checkbox" type="checkbox" ref="cb_push"/> Auto download recommended files from this node<br/>
			        <div onClick={this.final_add_handler} className="btn2">add to friendslist</div>
				</div>
			);
	},

	*/

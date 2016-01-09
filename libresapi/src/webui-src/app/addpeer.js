"use strict";

var m = require("mithril");
var rs = require("retroshare");


module.exports = {
    view: function(){
        return m("h2","add new friend");
    }
}

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

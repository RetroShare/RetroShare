var m = require("mithril");
var rs = require("retroshare");

var state = {};
var searchText = "";

function updateText(newText) {
    searchText = newText;
}

function dosearch(){
	console.log("searching for: "+searchText);
	rs.request(
	    "filesearch/create_search", {
	        distant: true,
	        search_string: searchText
	    },
        function(resp){
            m.route("/search/" + resp.search_id);
        }
	);
}

module.exports = {
    view: function(){
        var results = rs("filesearch");
        if (results === undefined||results == null) {
            results = [];
        };
        return m("div",[
            m("h2","turtle file search"),
            m("div", [
                m("input[type=text]", {onchange:m.withAttr("value", updateText)}),
                m("input[type=button][value=search]",{onclick:dosearch})
            ]),
            m("hr"),
            m("h2","previous searches:"),
            m("div", [
                results.map(function(item){
                    var res = rs("filesearch/" + item.id,{},null,{allow:"not_set|ok"});
                    if (res === undefined) {
                        res =[];
                    };
                    return m("div.btn2",{
                        onclick:function(){
		                    m.route("/search/" + item.id);
		                    }
                    }, item.search_string + " (" + res.length + ")");
                })
            ])
        ])
    }
}


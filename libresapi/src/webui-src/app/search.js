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
	    onCreateSearchResponse
	);
}

function onCreateSearchResponse(resp){
    state={search_id: resp.search_id};
    //tracking searchresults
    rs("filesearch/"+state.search_id,{},null,{allow:"not_set|ok"});
    //todo: route search/id, caching search-resultlists
}

module.exports = {
    view: function(){
        var results = [];
        if (state.search_id != undefined){
            var searchresult = rs("filesearch/"+state.search_id);
            if (searchresult != undefined) {
                results = searchresult;
            }
        }
        return m("div",[
            m("p","turtle file search"),
            m("div", [
                m("input[type=text]", {onchange:m.withAttr("value", updateText)}),
                m("input[type=button][value=search]",{onclick:dosearch})
            ]),
            m("table", [
                m("tr" ,[
                    m("th","name"),
                    m("th","size"),
                    m("th",""),
                ]),
                results.map(function(file){
                    return m("tr",[
                        m("th",file.name),
                        m("th",file.size),
                        m("th",[
                            m("span.btn", {
                                onclick:function(){
				                    rs.request("transfers/control_download", {
					                    action: "begin",
					                    name: file.name,
					                    size: file.size,
					                    hash: file.hash,
				                    });
                                }
                            },
                            "download")
                        ]),
                    ])
                })
            ])
        ])
    }
}


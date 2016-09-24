var m = require("mithril");
var rs = require("retroshare");

module.exports = {
    view: function(){
        var id=m.route.param("id");
        var results = rs("filesearch/" + id ,{},null,{allow:"not_set|ok"});
        if (results === undefined || results.length == undefined) {
            results = [];
        }
        var searches = rs("filesearch");
        var searchdetail = "<unknown>";
        if (!(searches === undefined) && !(searches.length === undefined)) {
            searches.forEach(function(s){
                if (s.id == id) {
                    searchdetail = s.search_string;
                }
            });
        }

        var dl_ids = [];

        var downloads =rs("transfers/downloads");
        if (downloads !== undefined) {
            downloads.map(function(item){
                dl_ids.push(item.hash);
            })
        }

        return m("div",[
            m("h2","turtle file search results"),
            m("h3", "searchtext: " + searchdetail + " (" + results.length + ")"),
            m("hr"),
            m("table", [
                m("tr" ,[
                    m("th","name"),
                    m("th","size"),
                    m("th",""),
                ]),
                results.map(function(file){
                    if (dl_ids.indexOf(file.hash)>=0) {
                        file.state="in download queue"
                    }
                    return m("tr",[
                        m("th",file.name),
                        m("th",file.size),
                        m("th",[
                            file.state === undefined
                            ? m("span.btn", {
                                onclick:function(){
				                    rs.request("transfers/control_download", {
					                    action: "begin",
					                    name: file.name,
					                    size: file.size,
					                    hash: file.hash,
				                    }, function(){
				                      result="added";
				                    });
                                    m.startComputation();
                                    m.endComputation();
                                }
                            },  "download")
                            : file.state
                        ]),
                    ])
                })
            ])
        ])
    }
}


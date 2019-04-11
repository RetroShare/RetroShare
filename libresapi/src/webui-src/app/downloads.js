var m = require("mithril");
var rs = require("retroshare");

function makeFriendlyUnit(bytes)
{
	if(bytes < 1e3)
		return bytes.toFixed(1) + "B";
	if(bytes < 1e6)
		return (bytes/1e3).toFixed(1) + "kB";
	if(bytes < 1e9)
		return (bytes/1e6).toFixed(1) + "MB";
	if(bytes < 1e12)
		return (bytes/1e9).toFixed(1) + "GB";
	return (bytes/1e12).toFixed(1) + "TB";
}

function progressBar(file){
    return m("div[style=border:5px solid lime;"
        + 'border-radius:3mm;'
        + 'padding:2mm;'
        + 'height:5mm'
	+ "]", [
	m("div[style="
	    + 'background-color:lime;'
	    + 'height:100%;'
	    + 'width:' + (file.transferred  /  file.size * 100)+'%'
	    + ']'
	,"")
	]);
};

function cntrlBtn(file, act) {
    return(
        m("div.btn",{
            onclick: function(){
                rs.request("transfers/control_download",{action: act, hash: file.hash});
            }
        },
        act)
    )
}


module.exports = {
    view: function(){
        var paths = rs("transfers/downloads");
        var filestreamer_url = "/fstream/";
        if (paths === undefined) {
            return m("div", "Downloads ... please wait ...");
        }
        return m("div", [
            m("h2","Downloads (" + paths.length +")"),
            m("div.btn2", {
                onclick: function(){
                    m.route("/downloads/add");
                }
            }, "add retrohare downloads"),
            m("hr"),
            m('table', [
                m("tr",[
                    m("th","name"),
                    m("th","size"),
                    m("th","progress"),
                    m("th","transfer rate"),
                    m("th","status"),
                    m("th","progress"),
                    m("th","action")
                ]),
            	paths.map(function (file){
            	    var ctrlBtn = m("div","");
                    var progress = file.transferred  /  file.size * 100;
            	    return m("tr",[
            	        m("td",[
            	            m("a.filelink",
            	                {
                        	        target: "blank",
                        	        href: filestreamer_url + file.hash + "/" + encodeURIComponent(file.name)
                	            },
            	                file.name
            	            )
            	        ]),
            	        m("td", makeFriendlyUnit(file.size)),
            	        m("td", progress.toPrecision(3) + "%"),
            	        m("td", makeFriendlyUnit(file.transfer_rate*1e3)+"/s"),
            	        m("td", file.download_status),
            	        m("td", progressBar(file)),
            	        m("td", [
            	            cntrlBtn(file, file.download_status==="paused"?"start":"pause"),
            	            cntrlBtn(file, "cancel")]
            	        )
            	    ])
                })
	    ])
        ]);
    }
};

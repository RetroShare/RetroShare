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
	    + 'width:' + (file.transfered  /  file.size * 100)+'%'
	    + ']'
	,"")
	]);
};

function cntrlBtn(file, act) {
    return(
        m("button",{
            onclick: function(){
                rs.request("transfers/control_download",{action: act, id: file.id});
            }
        },
        act)
    )
}


module.exports = {
    view: function(){
        var paths = rs("transfers/downloads");
        if (paths === undefined) {
            return m("div", "Downloads ... please wait ...");
        }
        return m("div", [
            m("div","Downloads (" + paths.length +")"),
            m("hr[style=color:silver]"),
            m('table[cellspacing=2][style=padding:2px !important;margin:2px !important;border-spacing: 2px]', [
                m("tr[style=font-weight:bold]",[
                    m("th[style=padding:2px]","name"),
                    m("th[style=padding:2px]","size"),
                    m("th[style=padding:2px]","progress"),
                    m("th[style=padding:2px]","transfer rate"),
                    m("th[style=padding:2px]","status"),
                    m("th[style=padding:2px]","progress"),
                    m("th[style=padding:2px]","action")
                ]),
            	paths.map(function (file){
            	    var ctrlBtn = m("div","");
                    var progress = file.transfered  /  file.size * 100;
            	    return m("tr",[
            	        m("td[style=padding:2px;style=padding: 0.3em;border-style: solid;border-width: 0.1em;border-color: #0F0]", file.name),
            	        m("td[style=padding:2px;style=padding: 0.3em;border-style: solid;border-width: 0.1em;border-color: #0F0]", makeFriendlyUnit(file.size)),
            	        m("td[style=text-align:right;padding:2px;style=padding: 0.3em;border-style: solid;border-width: 0.1em;border-color: #0F0]", progress.toPrecision(3) + "%"),
            	        m("td[style=text-align:right;padding:2px;style=padding: 0.3em;border-style: solid;border-width: 0.1em;border-color: #0F0]", makeFriendlyUnit(file.transfer_rate*1e3)+"/s"),
            	        m("td[style=padding:2px;style=padding: 0.3em;border-style: solid;border-width: 0.1em;border-color: #0F0]", file.download_status),
            	        m("td[style=padding:2px;style=padding: 0.3em;border-style: solid;border-width: 0.1em;border-color: #0F0]", progressBar(file)),
            	        m("td[style=padding:2px;style=padding: 0.3em;border-style: solid;border-width: 0.1em;border-color: #0F0]", [
            	            cntrlBtn(file, file.download_status==="paused"?"start":"pause"),
            	            m("span"," "),
            	            cntrlBtn(file, "cancel")]
            	        )
            	    ])
                })
	    ])
        ]);
    }
};

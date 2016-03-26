var m = require("mithril");
var rs = require("retroshare");

var me = {
    toParse: [], // links to parse ( = pasted content)
    toConfirm: [], // links to confirm
    toAdd: [], // links to add
    toResult: [], // Result to show
    index: 0,
    view: function(){
        return m("div", {
            style: {
                height:"100%",
                boxSizing: "border-box",
                paddingBottom: "130px",
            }
        },[
            m("h2","add downloads"),
            m("hr"),
            this.toParse.length
            ? step2()
            : this.toConfirm.length
            ? step3()
            : this.toAdd.length
            ? step4()
            : this.toResult.length
            ? step5()
            : step1()
            ,
        ]);
    },
    parseOne: function(){
        if (me.index == null) {
            return null;
        }
        var startindex = me.index;
        while (me.toParse.length  > me.index && me.index - startindex < 10) {
            var src = me.toParse[me.index].split("?",2);
            console.log(src);
            if (src[0] == "retroshare://file" && src.length == 2) {
                var target = {action: "begin"};
                var errText = "Error: link missing name and/or hash"
                src[1].split("&").map(function(parm){
                    var pos = parm.indexOf("=");
                    if (pos >0){
                        if (parm.substr(0,pos) == "name") {
                            var sname=decodeURIComponent(parm.substr(pos+1))
                            if (sname.match("[\\\\/]")) {
                                errText="name contains illegal char "
                                    + sname.match("[\\\\/]");
                            } else {
                                target.name=sname;
                            }
                        } else if (parm.substr(0,pos) == "size") {
                            target.size=parseFloat(parm.substr(pos+1));
                        } else if (parm.substr(0,pos) == "hash") {
                            target.hash=parm.substr(pos+1);
                        }
                    }
                });
                if (target['name'] && target['hash']){
                    me.toConfirm.push({
                        text: target.name,
                        target: target,
                        confirmed: true,
                    });
                } else {
                    me.toConfirm.push({
                        text:errText,
                    });
                }
            } else {
                me.toConfirm.push({ text: "Error: no Retroshare-file link"})
            }
            me.index++;
        }
        if (me.toParse.length > me.index) {
            window.setTimeout("require(\"adddownloads\").parseOne()",1);
        } else {
            me.toParse = [];
            console.log(me.toConfirm.length);
        }
        refresh();
    },
    addOne: function(){
        if (me.index == null) {
            cancel();
        } else if (me.index >= me.toAdd.length) {
            me.toResult=me.toAdd;
            me.toAdd=[];
            refresh();
        } else {
            console.log([
                me.toAdd[me.index].action,
                me.toAdd[me.index].name,
                me.toAdd[me.index].size,
                me.toAdd[me.index].hash,
            ]);
            refresh();
            rs.request("transfers/control_download", me.toAdd[me.index],
                function(data,statetoken){
                    if (me.index != null) {
                        me.toAdd[me.index].ok=true;
                        me.index++;
                        me.addOne();
                    }
                }, {
                    onfail: function(value){
                        me.toAdd[me.index].ok=false;
                        me.toAdd[me.index].debug_msg=value;
                        me.index++;
                        me.addOne();
                    },
                    onmismatch: function(response){
                        me.toAdd[me.index].ok=false;
                        me.toAdd[me.index].debug_msg=response.debug_msg;
                        me.index++;
                        me.addOne();
                    },
                }
            );
        }
    }
};

function cancel() {
    me.toAdd=[];
    me.toConfirm=[];
    me.toParse=[];
    me.toResult=[];
    me.index=null;
    refresh();
}

function parseDownloads(){
    me.toParse = document.getElementById("txtInput").value.replace("\r\n","\n").split("\n");
    var pos;
    while ((pos=me.toParse.indexOf(""))>=0) {
        me.toParse.splice(pos,1);
    }
    var parser = document.createElement('a');
    me.toConfirm = [];
    me.index = 0;
    if (me.toParse.length  > me.index){
        window.setTimeout("require(\"adddownloads\").parseOne()",1);
    }
}

function addDownloads(){
    me.toConfirm.map(function(item){
        if (item.confirmed) {
            item.debug_msg="";
            me.toAdd.push(item.target);
        }
    });
    me.toConfirm=[];
    if (me.toAdd.length > 0){
        me.index=0;
        window.setTimeout("require(\"adddownloads\").addOne()",1);
    } else {
        cancel();
    }
    refresh();
}

function refresh(){
    m.startComputation();
    m.endComputation();
}

function cancelBtn(){
    return m("div.btn2", {
        style:{
            textAlign: "center",
            color: "red",
            borderColor: "red",
        },
        onclick:cancel,
    },"cancel");
}

// paste links
function step1(){
    m.initControl = "txtInput";
    return [
        m("h3","step 1 / 5: paste retroshare-links:"),
        m("textarea[id=txtInput]", {
            style: {
                height:"100%",
            },
            onkeydown: function(event){
                if (event.keyCode == 13){
                    parseDownloads();
                }
            }
        }),
        m("div.btn2", {
            style:{
                textAlign:"center",
            },
            onclick:parseDownloads,
        },"add downloads")
    ]
}

// parsing links
function step2(){
    return [
        m("h3","step 2 / 5: parsing input ..."),
        m("p",
            "parsing " + (me.index)  + " / " + me.toParse.length),
        cancelBtn(),
    ]
}

// parsing confirm
function step3(){
    return [
        m("h3","step 3 / 5: confirm-links:"),
        m("ul",
            me.toConfirm.map(function(item){
                return m("li", {
                    style:{
                        color: item.confirmed
                            ? "lime"
                            : "red"
                    },
                }, item.text);
            })
        ),
        m("div.btn2", {
            style:{
                textAlign:"center",
            },
            onclick:addDownloads,
        },"add green listed downloads"),
        cancelBtn(),
    ]
}

// adding links
function step4(){
    return [
        m("h3","step 4 / 5: adding downloads:"),
        m("p",
            "adding " + (me.index)  + " / " + me.toParse.length),
        m("ul",
            me.toAdd.map(function(item){
                return m("li", {
                    style:{
                        color: item.ok === undefined
                            ? "white"
                            : item.ok == null
                            ? "grey"
                            : item.ok
                            ? "lime"
                            : "red"
                    },
                }, (item.debug_msg ? item.debug_msg + ": " : "") + item.name
                    + " " + item.size + " " + item.hash);
            })
        ),
        cancelBtn(),
    ]
}

// show result
function step5(){
    return [
        m("h3","step 5 / 5: Result:"),
        m("p",
            "verarbeitet: " + me.toResult.length),
        m("ul",
            me.toResult.map(function(item){
                return m("li", {
                    style:{
                        color: item.ok === undefined
                            ? "white"
                            : item.ok == null
                            ? "grey"
                            : item.ok
                            ? "lime"
                            : "red"
                    },
                }, (item.debug_msg ? item.debug_msg + ": " : "") + item.name);
            })
        ),
        m("div.btn2", {
            style:{
                textAlign: "center",
            },
            onclick: cancel,
        },"ok"),
    ]
}



module.exports = me;

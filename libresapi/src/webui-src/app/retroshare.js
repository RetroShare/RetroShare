/*
var rs = requires("rs");
var m = require("mithril");

function main(){
    var state = rs("runstate");
    if(state=== undefined){
        return m("div", "waiting for server");
    }
    if(state === "waiting_login"){
        return require("login")();
    }
    if(state === "running_ok"){
        return require("mainwindow")();
    }
}
*/

/*
idea: statetokenservice could just send the date instead of the token
*/

"use strict";

var m = require("mithril");

var api_url = window.location.protocol + "//" + window.location.hostname + ":" + window.location.port + "/api/v2/";
var filestreamer_url = window.location.protocol + "//" +window.location.hostname + ":" + window.location.port + "/fstream/";
var upload_url = window.location.protocol + "//" + window.location.hostname + ":" + window.location.port + "/upload/";

function for_key_in_obj(obj, callback){
    var key;
    for(key in obj){
        callback(key);
    }
}

var cache = {};
var last_update_ts = 0;

function check_for_changes(){
    var tokens = [];
//    console.log("start-check");
    for_key_in_obj(cache, function(path){
        var token = cache[path].statetoken;
        if(token !== undefined){
            tokens.push(token);
        }
    });
//    console.log("tokens found: " + tokens);
    var req = m.request({
        method: "POST",
        url: api_url + "statetokenservice",
        background: true,
        data: tokens,
    });
    
    req.then(function handle_statetoken_response(response){
//        console.log("checking result " + response);
        var paths_to_fetch = [];
        for_key_in_obj(cache, function(path){
            var found = false;
            for(var i = 0; i < response.data.length; i++){
                if(response.data[i] === cache[path].statetoken){
                    found = true;
                }
            }
            if(found){
                paths_to_fetch.push(path);
            }
        });
//        console.log("generating Results for " + paths_to_fetch.length + " paths");
        var requests = [];
        paths_to_fetch.map(function request_it(path){
            var req2 = m.request({
                method: "GET",
                url: api_url + path,
                background: true,
            });
            req2 = req2.then(function fill_in_result(response){
                cache[path].data = response.data;
                cache[path].statetoken = response.statetoken;
            });
            requests.push(req2);
        });
        if(requests.length > 0){
//            console.log("requesting " + requests.length + " requests");
            m.sync(requests).then(function trigger_render(){
                m.startComputation();
                m.endComputation();
                checkFocus();
                setTimeout(check_for_changes, 500);
            });
        }
        else{
//            console.log("no requests");
            setTimeout(check_for_changes, 500);
        }
    }, function errhandling(value){
//        console.log("server disconnected " + value);
        setTimeout(check_for_changes, 500);
    });
}

check_for_changes();

var update_scheduled = false;
function schedule_request_missing(){
    if(update_scheduled)
        return;
    update_scheduled = true;
    // place update logic outside of render loop, this way we can fetch multiple things at once
    // (because after the render loop everything we should fetch is in the list)
    // if we fetch multiple things at once, we can delay a re-rende runtil everything is done
    // so we need only one re-render for multiple updates
    setTimeout(function request_missing(){
        update_scheduled = false;
        var requests = [];
        for_key_in_obj(cache, function(path){
            if(!cache[path].requested){
                var req = m.request({
                    method: "GET",
                    url: api_url + path,
                    background: true,
                });

                req.then(function fill_data(response){
                    // TODO: add errorhandling
                    cache[path].data = response.data;
                    cache[path].statetoken = response.statetoken;
                    if (cache[path].then != undefined && cache[path].then != null) {
                        try {
                            cache[path].then(response);
                        } catch (ex) {
                            if (cache[path].errorCallback != undefined && cache[path].errorCallback != null) {
                                cache[path].errorCallback(ex);
                            };
                        }
                    };
                }, function errhandling(value){
                    if (cache[path].errorCallback != undefined && cache[path].errorCallback != null) {
                        cache[path].errorCallback(value);
                    }
                });
                requests.push(req);
            }
            cache[path].requested = true;
        });
        m.sync(requests).then(function trigger_render(){
            m.startComputation();
            m.endComputation();
            checkFocus();
        });
    });
}

function checkFocus(){
	if (m.initControl != undefined) {
	    var ctrl = document.getElementById(m.initControl);
	    if (ctrl!= null) {
		    ctrl.focus();
		    m.initControl = undefined;
	    } else {
	        console.log("focus-control '" + m.initControl + "' not found!");
	        m.initControl = undefined;
	    }
	}
}

// called every time, rs or rs.request failed, only response or value is set
function requestFail(path, response, value) {
    rs.error = "error on " + path;
    console.log("Error on " + path +
        (response == null ? ", value: " + value : (", response: " +
            (response.debug_msg === undefined ? response : response.debug_msg)
        ))
    );
}

function rs(path, args, callback, options){
    if(cache[path] === undefined){
        options=optionsPrep(options,path);
        var req = {
            data: args,
            statetoken: undefined,
            requested: false,
            allow: options.allow,
            then: function(response){
                options.log(path + ": response: " + response.returncode);
                if (!this.allow.match(response.returncode)) {
                    options.onmismatch(response);
                } else if (callback != undefined && callback != null) {
                    callback(response.data, response.statetoken);
                }
            },
            errorCallback: options.onfail
        };
        cache[path] = req;
        schedule_request_missing();
    }
    return cache[path].data;
}

module.exports = rs;

// single request for action
rs.request=function(path, args, callback, options){
    options = optionsPrep(options, path);
    var req = m.request({
        method: options.method === undefined ? "POST" : options.method,
        url: api_url + path,
        data: args,
        background: true
    });
    req.then(function checkResponseAndCallback(response){
        options.log(path + ": response: " + response.returncode);
        if (!options.allow.match(response.returncode)) {
            options.onmismatch(response);
        } else if (callback != undefined && callback != null) {
            callback(response.data, response.statetoken);
        }
    }, options.onfail);
    return req;
};

//set default-values for shared options in rs() and rs.request()
function optionsPrep(options, path) {
    if (options === undefined) {
        options = {};
    }

    if (options.onfail === undefined) {
        options.onfail = function errhandling(value){
            requestFail(path, null, value);
        }
    };
    if (options.onmismatch === undefined) {
        options.onmismatch = function errhandling(response){
            requestFail(path, response,null);
        }
    };

    if (options.log === undefined)  {
        options.log = function(message) {
            console.log(message);
        }
    }

    if (options.allow === undefined) {
        options.allow = "ok";
    };
    return options;
}

// force reload for path
rs.forceUpdate = function(path){
   cache[path].requested=false;
}

//return api-path
rs.apiurl = function(path) {
    if (path === undefined) {
        path="";
    }
    if (path.length > 0 && "^\\\\|\\/".match(path)) {
        path=path.substr(1);
    }
    return api_url + path;
}

// counting in menu
rs.counting = function(path, counterfnkt) {
    return function () {
        var data=rs(path);
        if (data != undefined) {
            if (counterfnkt === undefined) {
                return " (" + data.length + ")";
            }
            return " (" + counterfnkt(data) + ")";
        }
        return "";
    }
};

// counting in menu
rs.counting2 = function(targets) {
    return function () {
        var sum = 0;
        for (var path in targets) {
            var data=rs(path);
            if (data != undefined) {
                data.map(function(item){
                    sum += parseInt(targets[path](item));
                });
            };
        };
        if (sum > 0) {
            return " (" + sum + ")";
        }
        return "";
    }
};

// listing data-elements
rs.list = function(path, buildfktn, sortfktn){
    var list = rs(path);
    if (list === undefined|| list == null) {
        return "< waiting for server ... >"
    };
    if (sortfktn != undefined && sortfktn != null) {
        list=list.sort(sortfktn);
    }
    return list.map(buildfktn);
};

//remember additional data (feature of last resort)
rs.memory = function(path, args){
    var item = cache[path];
    if (item === undefined) {
        rs(path, args);
        item =  cache[path];
    }
    if (item.memory === undefined) {
        item.memory = {};
    }
    return item.memory;
};

// Sortierfunktion für Texte von Objekten,
// falls einfache Namen nicht funktionieren
rs.stringSort = function(textA,textB, innersort, objectA, objectB){
        if (textA.toLowerCase() == textB.toLowerCase()) {
            if (innersort === undefined) {
                return 0
            }
            return innersort(objectA,objectB);
        } else if (textA.toLowerCase() < textB.toLowerCase()) {
            return -1
        } else {
            return 1
        }
    }


//return sorting-function for string, based on property name
//using: list.sort(rs.sort("name"));
// -----
//innersort: cascading sorting - using:
//list.sort(rs.sort("type",rs.sort("name")))
rs.sort = function(name, innersort){
    return function(a,b) {
        return rs.stringSort(a[name],b[name],innersort,a,b);
    }
}

//return sorting-function for boolean, based on property name
rs.sort.bool = function(name, innersort){
    return function(a,b){
        if (a[name] == b[name]) {
            if (innersort === undefined) {
                return 0
            }
            return innersort(a,b);
        } else if (a[name]) {
            return -1
        } else {
            return 1
        }
    }
}

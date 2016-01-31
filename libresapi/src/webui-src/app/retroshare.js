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
    console.log("Error on " + path + (response == null ? ", value " + value : ", response " + response));
}

function rs(path, args, callback, options){
    if(cache[path] === undefined){
        if (options === undefined){
            options = {};
        }
        var req = {
            data: args,
            statetoken: undefined,
            requested: false,
            allow: options.allow === undefined ? "ok" : options.allow,
            then: function(response){
                console.log(path + ": response: " + response.returncode);
                if (!this.allow.match(response.returncode)) {
                    requestFail(path, response, null);
                } else if (callback != undefined && callback != null) {
                    callback(response.data, response.statetoken);
                }
            },
            errorCallback: function(value){
                requestFail(path, null, value);
            }
        };
        cache[path] = req;
        schedule_request_missing();
    }
    return cache[path].data;
}

module.exports = rs;

rs.request=function(path, args, callback, options){
    if (options === undefined) {
        options = {};
    }
    var req = m.request({
        method: options.method === undefined ? "POST" : options.method,
        url: api_url + path,
        data: args,
        background: true
    });
    req.then(function checkResponseAndCallback(response){
        console.log(path + ": response: " + response.returncode);
        if (response.returncode != "ok") {
            requestFail(path, response, null);
        } else if (callback != undefined && callback != null) {
            callback(response.data, response.statetoken);
        }
    }, function errhandling(value){
        requestFail(path, null, value);
    });
    return req;
};

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

rs.counting = function(path, counterfnkt) {
    return function () {
        var data=rs(path);
        if (data != undefined) {
            return " (" + counterfnkt(data) + ")";
        }
        return "";
    }
}


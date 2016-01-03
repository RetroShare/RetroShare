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
    for_key_in_obj(cache, function(path){
        var token = cache[path].statetoken;
        if(token !== undefined){
            tokens.push(token);
        }
    });
    
    var req = m.request({
        method: "POST",
        url: api_url + "statetokenservice",
        background: true,
        data: tokens,
    });
    
    req.then(function handle_statetoken_response(response){
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
            m.sync(requests).then(function trigger_render(){
                m.startComputation();
                m.endComputation();
                setTimeout(check_for_changes, 500);
            });
        }
        else{
            setTimeout(check_for_changes, 500);
        }
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
                req = req.then(function fill_data(response){
                    // TODO: add errorhandling
                    cache[path].data = response.data;
                    cache[path].statetoken = response.statetoken;
                });
                requests.push(req);
            }
            cache[path].requested = true;
        });
        m.sync(requests).then(function trigger_render(){
            m.startComputation();
            m.endComputation();
        });
    });
}

function rs(path, optionen){
    if(optionen === undefined) {
        if(cache[path] === undefined){
            cache[path] = {
                data: undefined,
                statetoken: undefined,
                requested: false,
            };
            schedule_request_missing();
        }
        return cache[path].data;
    } else if (optionen.args != undefined) {
        var req = m.request({
                             method: "GET",

                        url: api_url + path,

                        args: optionen.args,

                        background: false,

                             callback: {
                                 }

    });

    }

}

module.exports = rs;
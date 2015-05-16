/**
 * Connection to the RS backend using XHR
 * (could add other connections later, for example WebSockets)
 * @constructor
 */
function RsXHRConnection(server_hostname, server_port)
{
    var debug;
    //debug = function(str){console.log(str);};
    debug = function(str){};

    //server_hostname = "localhost";
    //server_port = "9090";
    var api_root_path = "/api/v2/";

    var status_listeners = [];

    function notify_status(status)
    {
        for(var i = 0; i < status_listeners.length; i++)
        {
            status_listeners[i](status);
        }
    }

    /**
     *  Register a callback to be called when the state of the connection changes.
     *  @param {function} cb - callback which receives a single argument. The arguments value is "connected" or "not_connected".
     */
    this.register_status_listener = function(cb)
    {
        status_listeners.push(cb);
    };

    /**
     * Unregister a status callback function.
     * @param {function} cb - a privously registered callback function
     */
     this.unregister_status_listener = function(cb)
     {
        var to_delete = [];
        for(var i = 0; i < status_listeners.length; i++)
        {
            if(status_listeners[i] === cb){
                to_delete.push(i);
            }
        }
        for(var i = 0; i < to_delete.length; i++)
        {
            // copy the last element to the current index
            var index = to_delete[i];
            status_listeners[i] = status_listeners[status_listeners.length-1];
            // remove the last element
            status_listeners.pop();
        }
     };

    /**
     * Send a request to the backend
     * automatically encodes the request as JSON before sending it to the server
     * @param {object}  req - the request to send to the server
     * @param {function} cb - callback function to be called to handle the response. The callback takes one object as parameter. Can be left undefined.
     * @param {function} err_cb - callback function to signal a failed request. Can be undefined.
     */
    this.request = function(req, cb, err_cb)
    {
        //var xhr = window.XMLHttpRequest ? new XMLHttpRequest() : new ActiveXObject("Microsoft.XMLHTTP");
        // TODO: window is not available in QML
        var xhr = new XMLHttpRequest();
        xhr.onreadystatechange = function(){
            //console.log("onreadystatechanged state"+xhr.readyState);
            // TODO: figure out how to catch errors like connection refused
            // maybe want to have to set a state variable like ok=false
            // the gui could then display: "no connection to server"
            if (xhr.readyState === 4) {
                if(xhr.status !== 200)
                {
                    console.log("RsXHRConnection: request failed with status: "+xhr.status);
                    console.log("request was:");
                    console.log(req);
                    notify_status("not_connected");
                    if(err_cb !== undefined)
                        err_cb();
                    return;
                }
                // received response
                notify_status("connected");
                debug("RsXHRConnection received response:");
                debug(xhr.responseText);
                if(false)//if(xhr.responseText === "")
                {
                    debug("Warning: response is empty");
                    return;
                }
                try
                {
                    var respObj = JSON.parse(xhr.responseText);
                }
                catch(e)
                {
                    debug("Exception during response handling: "+e);
                }
                if(cb === undefined)
                    debug("No callback function specified");
                else
                    cb(respObj);
            }
        }
        // post is required for sending data
        var method;
        if(req.data){
            method = "POST";
        } else {
            method = "GET";
        }
        xhr.open(method, "http://"+server_hostname+":"+server_port+api_root_path+req.path);
        var data = JSON.stringify(req.data);
        debug("RsXHRConnection sending data:");
        debug(data);
        xhr.setRequestHeader('Content-Type', 'application/json');
        xhr.send(data);
    };
};
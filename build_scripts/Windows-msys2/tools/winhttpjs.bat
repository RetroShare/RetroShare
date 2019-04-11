@if (@X) == (@Y) @end /* JScript comment 
        @echo off 
        
        rem :: the first argument is the script name as it will be used for proper help message 
        cscript //E:JScript //nologo "%~f0" "%~nx0" %* 

        exit /b %errorlevel% 
        
@if (@X)==(@Y) @end JScript comment */

// used resources

// update 12.10.15 
// osvikvi(https://github.com/osvikvi) has nodited that the -password option is not set , so this is fixed

//https://msdn.microsoft.com/en-us/library/windows/desktop/aa384058(v=vs.85).aspx 
//https://msdn.microsoft.com/en-us/library/windows/desktop/aa384055(v=vs.85).aspx 
//https://msdn.microsoft.com/en-us/library/windows/desktop/aa384059(v=vs.85).aspx 

// global variables and constants 


// ---------------------------------- 
// -- asynch requests not included -- 
// ---------------------------------- 


//todo - save responceStream instead of responceBody !! 
//todo - set all winthttp options ->//https://msdn.microsoft.com/en-us/library/windows/desktop/aa384108(v=vs.85).aspx 
//todo - log all options 
//todo - improve help message . eventual verbose option 


var ARGS = WScript.Arguments;
var scriptName = ARGS.Item(0);

var url = "";
var saveTo = "";

var user = 0;
var pass = 0;

var proxy = 0;
var bypass = 0;
var proxy_user = 0;
var proxy_pass = 0;

var certificate = 0;

var force = true;

var body = "";

//ActiveX objects 
var WinHTTPObj = new ActiveXObject("WinHttp.WinHttpRequest.5.1");
var FileSystemObj = new ActiveXObject("Scripting.FileSystemObject");
var AdoDBObj = new ActiveXObject("ADODB.Stream");

// HttpRequest SetCredentials flags. 
var proxy_settings = 0;

// 
HTTPREQUEST_SETCREDENTIALS_FOR_SERVER = 0;
HTTPREQUEST_SETCREDENTIALS_FOR_PROXY = 1;

//timeouts and their default values 
var RESOLVE_TIMEOUT = 0;
var CONNECT_TIMEOUT = 90000;
var SEND_TIMEOUT = 90000;
var RECEIVE_TIMEOUT = 90000;

//HttpRequestMethod 
var http_method = 'GET';

//header 
var header_file = "";

//report 
var reportfile = "";

//test-this: 
var use_stream = false;

//autologon policy 
var autologon_policy = 1; //0,1,2 


//headers will be stored as multi-dimensional array 
var headers = [];

//user-agent 
var ua = "";

//escape URL 
var escape = false;

function printHelp() {
    WScript.Echo(scriptName + " - sends HTTP request and saves the request body as a file and/or a report of the sent request");
    WScript.Echo(scriptName + " url  [-force yes|no] [-user username -password password] [-proxy proxyserver:port] [-bypass bypass_list]");
    WScript.Echo("                                                        [-proxyuser proxy_username -proxypassword proxy_password] [-certificate certificateString]");
    WScript.Echo("                                                        [-method GET|POST|PATCH|DELETE|HEAD|OPTIONS|CONNECT]");
    WScript.Echo("                                                        [-saveTo file] - to print response to console use con");

    WScript.Echo("                                                        [-sendTimeout int(milliseconds)]");
    WScript.Echo("                                                        [-resolveTimeout int(milliseconds)]");
    WScript.Echo("                                                        [-connectTimeout int(milliseconds)]");
    WScript.Echo("                                                        [-receiveTimeout int(milliseconds)]");

    WScript.Echo("                                                        [-autologonPolicy 1|2|3]");
    WScript.Echo("                                                        [-proxySettings 1|2|3] (https://msdn.microsoft.com/en-us/library/windows/desktop/aa384059(v=vs.85).aspx)");

    //header 
    WScript.Echo("                                                        [-headers-file header_file]");
    //reportfile 
    WScript.Echo("                                                        [-reportfile reportfile]");
    WScript.Echo("                                                        [-ua user-agent]");
    WScript.Echo("                                                        [-ua-file user-agent-file]");

    WScript.Echo("                                                        [-escape yes|no]");

    WScript.Echo("                                                        [-body body-string]");
    WScript.Echo("                                                        [-body-file body-file]");

    WScript.Echo("-force  - decide to not or to overwrite if the local files exists");

    WScript.Echo("proxyserver:port - the proxy server");
    WScript.Echo("bypass- bypass list");
    WScript.Echo("proxy_user , proxy_password - credentials for proxy server");
    WScript.Echo("user , password - credentials for the server");
    WScript.Echo("certificate - location of SSL certificate");
    WScript.Echo("method - what HTTP method will be used.Default is GET");
    WScript.Echo("saveTo - save the responce as binary file");
    WScript.Echo(" ");
    WScript.Echo("Header file should contain key:value pairs.Lines starting with \"#\" will be ignored.");
    WScript.Echo("value should NOT be enclosed with quotes");
    WScript.Echo(" ");
    WScript.Echo("Examples:");

    WScript.Echo(scriptName + " http://somelink.com/somefile.zip -saveTo c:\\somefile.zip -certificate \"LOCAL_MACHINE\\Personal\\My Middle-Tier Certificate\"");
    WScript.Echo(scriptName + " http://somelink.com/something.html  -method POST  -certificate \"LOCAL_MACHINE\\Personal\\My Middle-Tier Certificate\" -header c:\\header_file -reportfile c:\\reportfile.txt");
    WScript.Echo(scriptName + "\"http://somelink\"  -method POST   -header hdrs.txt -reportfile reportfile2.txt -saveTo responsefile2 -ua \"Mozilla/5.0 (Windows NT 6.1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/41.0.2228.0 Safari/537.36\"  -body-file some.json");

}

function parseArgs() {
    // 
    if (ARGS.Length < 2) {
        WScript.Echo("insufficient arguments");
        printHelp();
        WScript.Quit(43);
    }
    // !!! 
    url = ARGS.Item(1);
    // !!! 
    if (ARGS.Length % 2 != 0) {
        WScript.Echo("illegal arguments");
        printHelp();
        WScript.Quit(44);
    }

    for (var i = 2; i < ARGS.Length - 1; i = i + 2) {
        var arg = ARGS.Item(i).toLowerCase();
        var next = ARGS.Item(i + 1);


        try {
            switch (arg) { // the try-catch is set mainly because of the parseInts 
                case "-force":
                    if (next == "no") {
                        force = false;
                    }
                    break;
                case "-escape":
                    if (next == "yes") {
                        escape = true;
                    }
                    break;
                case "-saveto":
                    saveTo = next;
                    break;
                case "-user":
                case "-u":
                    user = next;
                    break;
                case "-pass":
                case "-password":
                case "-p":
                    pass = next;
                    break;
                case "-proxy":
                    proxy = next;
                    break;
                case "-bypass":
                    bypass = next;
                    break;
                case "-proxyuser":
                case "-pu":
                    proxy_user = next;
                    break;
                case "-proxypassword":
                case "-pp":
                    proxy_pass = next;
                    break;
                case "-ua":
                    ua = next;
                    break;
                case "-ua-file":
                    ua = readFile(next);
                    break;
                case "-body":
                    body = next;
                    break;
                case "-usestream":
                    //WScript.Echo("~~"); 
                    if (next.toLowerCase() === "yes") {
                        use_stream = true
                    };
                    break;
                case "-body-file":
                    body = readFile(next);
                    break;
                case "-certificate":
                    certificate = next;
                    break;
                case "-method":
                    switch (next.toLowerCase()) {
                        case "post":
                            http_method = 'POST';
                            break;
                        case "get":
                            http_method = 'GET';
                            break;
                        case "head":
                            http_method = 'HEAD';
                            break;
                        case "put":
                            http_method = 'PUT';
                            break;
                        case "options":
                            http_method = 'OPTIONS';
                            break;
                        case "connect":
                            http_method = 'CONNECT';
                            break;
                        case "patch":
                            http_method = 'PATCH';
                            break;
                        case "delete":
                            http_method = 'DELETE';
                            break;
                        default:
                            WScript.Echo("Invalid http method passed " + next);
                            WScript.Echo("possible values are GET,POST,DELETE,PUT,CONNECT,PATCH,HEAD,OPTIONS");
                            WScript.Quit(1326);
                            break;
                    }
                    break;
                case "-headers-file":
                case "-header":
                    headers = readPropFile(next);
                    break;
                case "-reportfile":
                    reportfile = next;
                    break;
                    //timeouts 
                case "-sendtimeout":
                    SEND_TIMEOUT = parseInt(next);
                    break;
                case "-connecttimeout":
                    CONNECT_TIMEOUT = parseint(next);
                    break;
                case "-resolvetimeout":
                    RESOLVE_TIMEOUT = parseInt(next);
                    break;
                case "-receivetimeout":
                    RECEIVE_TIMEOUT = parseInt(next);
                    break;

                case "-autologonpolicy":
                    autologon_policy = parseInt(next);
                    if (autologon_policy > 2 || autologon_policy < 0) {
                        WScript.Echo("out of autologon policy range");
                        WScript.Quit(87);
                    };
                    break;
                case "-proxysettings":
                    proxy_settings = parseInt(next);
                    if (proxy_settings > 2 || proxy_settings < 0) {
                        WScript.Echo("out of proxy settings range");
                        WScript.Quit(87);
                    };
                    break;
                default:
                    WScript.Echo("Invalid  command line switch: " + arg);
                    WScript.Quit(1405);
                    break;
            }
        } catch (err) {
            WScript.Echo(err.message);
            WScript.Quit(1348);
        }
    }
}

stripTrailingSlash = function(path) {
    while (path.substr(path.length - 1, path.length) == '\\') {
        path = path.substr(0, path.length - 1);
    }
    return path;
}

function existsItem(path) {
    return FileSystemObj.FolderExists(path) || FileSystemObj.FileExists(path);
}

function deleteItem(path) {
    if (FileSystemObj.FileExists(path)) {
        FileSystemObj.DeleteFile(path);
        return true;
    } else if (FileSystemObj.FolderExists(path)) {
        FileSystemObj.DeleteFolder(stripTrailingSlash(path));
        return true;
    } else {
        return false;
    }
}

//------------------------------- 
//---------------------- 
//---------- 
//----- 
//-- 
function request(url) {

    try {

        WinHTTPObj.Open(http_method, url, false);
        if (proxy != 0 && bypass != 0) {
            WinHTTPObj.SetProxy(proxy_settings, proxy, bypass);
        }

        if (proxy != 0) {
            WinHTTPObj.SetProxy(proxy_settings, proxy);
        }

        if (user != 0 && pass != 0) {
            WinHTTPObj.SetCredentials(user, pass, HTTPREQUEST_SETCREDENTIALS_FOR_SERVER);
        }

        if (proxy_user != 0 && proxy_pass != 0) {
            WinHTTPObj.SetCredentials(proxy_user, proxy_pass, HTTPREQUEST_SETCREDENTIALS_FOR_PROXY);
        }

        if (certificate != 0) {
            WinHTTPObj.SetClientCertificate(certificate);
        }

        //set autologin policy 
        WinHTTPObj.SetAutoLogonPolicy(autologon_policy);
        //set timeouts 
        WinHTTPObj.SetTimeouts(RESOLVE_TIMEOUT, CONNECT_TIMEOUT, SEND_TIMEOUT, RECEIVE_TIMEOUT);

        if (headers.length !== 0) {
            WScript.Echo("Sending with headers:");
            for (var i = 0; i < headers.length; i++) {
                WinHTTPObj.SetRequestHeader(headers[i][0], headers[i][1]);
                WScript.Echo(headers[i][0] + ":" + headers[i][1]);
            }
            WScript.Echo("");
        }

        if (ua !== "") {
            //user-agent option from: 
            //WinHttpRequestOption enumeration 
            // other options can be added like bellow 
            //https://msdn.microsoft.com/en-us/library/windows/desktop/aa384108(v=vs.85).aspx 
            WinHTTPObj.Option(0) = ua;
        }
        if (escape) {
            WinHTTPObj.Option(3) = true;
        }
        if (trim(body) === "") {
            WinHTTPObj.Send();
        } else {
            WinHTTPObj.Send(body);
        }

        var status = WinHTTPObj.Status
    } catch (err) {
        WScript.Echo(err.message);
        WScript.Quit(666);
    }

    //////////////////////// 
    //     report         // 
    //////////////////////// 

    if (reportfile != "") {

        //var report_string=""; 
        var n = "\r\n";
        var report_string = "Status:" + n;
        report_string = report_string + "      " + WinHTTPObj.Status;
        report_string = report_string + "      " + WinHTTPObj.StatusText + n;
        report_string = report_string + "      " + n;
        report_string = report_string + "Response:" + n;
        report_string = report_string + WinHTTPObj.ResponseText + n;
        report_string = report_string + "      " + n;
        report_string = report_string + "Headers:" + n;
        report_string = report_string + WinHTTPObj.GetAllResponseHeaders() + n;

        WinHttpRequestOption_UserAgentString = 0; // Name of the user agent 
        WinHttpRequestOption_URL = 1; // Current URL 
        WinHttpRequestOption_URLCodePage = 2; // Code page 
        WinHttpRequestOption_EscapePercentInURL = 3; // Convert percents 
        // in the URL 
        // rest of the options can be seen and eventually added using this as reference 
        // https://msdn.microsoft.com/en-us/library/windows/desktop/aa384108(v=vs.85).aspx 

        report_string = report_string + "URL:" + n;
        report_string = report_string + WinHTTPObj.Option(WinHttpRequestOption_URL) + n;

        report_string = report_string + "URL Code Page:" + n;
        report_string = report_string + WinHTTPObj.Option(WinHttpRequestOption_URLCodePage) + n;

        report_string = report_string + "User Agent:" + n;
        report_string = report_string + WinHTTPObj.Option(WinHttpRequestOption_UserAgentString) + n;

        report_string = report_string + "Escapped URL:" + n;
        report_string = report_string + WinHTTPObj.Option(WinHttpRequestOption_EscapePercentInURL) + n;

        prepareateFile(force, reportfile);

        WScript.Echo("Writing report to " + reportfile);

        writeFile(reportfile, report_string);

    }

    switch (status) {
        case 200:
            WScript.Echo("Status: 200 OK");
            break;
        default:
            WScript.Echo("Status: " + status);
            WScript.Echo("Status was not OK. More info -> https://en.wikipedia.org/wiki/List_of_HTTP_status_codes");
    }

    //if as binary 
    if (saveTo.toLowerCase() === "con") {
        WScript.Echo(WinHTTPObj.ResponseText);
    }
    if (saveTo !== "" && saveTo.toLowerCase() !== "con") {
        prepareateFile(force, saveTo);
        try {

            if (use_stream) {
                writeBinFile(saveTo, WinHTTPObj.ResponseStream);
            } else {
                writeBinFile(saveTo, WinHTTPObj.ResponseBody);
            }

        } catch (err) {
            WScript.Echo("Failed to save the file as binary.Attempt to save it as text");
            AdoDBObj.Close();
            prepareateFile(true, saveTo);
            writeFile(saveTo, WinHTTPObj.ResponseText);
        }
    }
    WScript.Quit(status);
}

//-- 
//----- 
//---------- 
//---------------------- 
//------------------------------- 

function prepareateFile(force, file) {
    if (force && existsItem(file)) {
        if (!deleteItem(file)) {
            WScript.Echo("Unable to delete " + file);
            WScript.Quit(8);
        }
    } else if (existsItem(file)) {
        WScript.Echo("Item " + file + " already exist");
        WScript.Quit(9);
    }
}

function writeBinFile(fileName, data) {
    AdoDBObj.Type = 1;
    AdoDBObj.Open();
    AdoDBObj.Position = 0;
    AdoDBObj.Write(data);
    AdoDBObj.SaveToFile(fileName, 2);
    AdoDBObj.Close();
}

function writeFile(fileName, data) {
    AdoDBObj.Type = 2;
    AdoDBObj.CharSet = "iso-8859-1";
    AdoDBObj.Open();
    AdoDBObj.Position = 0;
    AdoDBObj.WriteText(data);
    AdoDBObj.SaveToFile(fileName, 2);
    AdoDBObj.Close();
}


function readFile(fileName) {
    //check existence 
    try {
        if (!FileSystemObj.FileExists(fileName)) {
            WScript.Echo("file " + fileName + " does not exist!");
            WScript.Quit(13);
        }
        if (FileSystemObj.GetFile(fileName).Size === 0) {
            return "";
        }
        var fileR = FileSystemObj.OpenTextFile(fileName, 1);
        var content = fileR.ReadAll();
        fileR.Close();
        return content;
    } catch (err) {
        WScript.Echo("Error while reading file: " + fileName);
        WScript.Echo(err.message);
        WScript.Echo("Will return empty string");
        return "";
    }
}

function readPropFile(fileName) {
    //check existence 
    resultArray = [];
    if (!FileSystemObj.FileExists(fileName)) {
        WScript.Echo("(headers)file " + fileName + " does not exist!");
        WScript.Quit(15);
    }
    if (FileSystemObj.GetFile(fileName).Size === 0) {
        return resultArray;
    }
    var fileR = FileSystemObj.OpenTextFile(fileName, 1);
    var line = "";
    var k = "";
    var v = "";
    var lineN = 0;
    var index = 0;
    try {
        WScript.Echo("parsing headers form " + fileName + " property file ");
        while (!fileR.AtEndOfStream) {
            line = fileR.ReadLine();
            lineN++;
            index = line.indexOf(":");
            if (line.indexOf("#") === 0 || trim(line) === "") {
                continue;
            }
            if (index === -1 || index === line.length - 1 || index === 0) {
                WScript.Echo("Invalid line " + lineN);
                WScript.Quit(93);
            }
            k = trim(line.substring(0, index));
            v = trim(line.substring(index + 1, line.length));
            resultArray.push([k, v]);
        }
        fileR.Close();
        return resultArray;
    } catch (err) {
        WScript.Echo("Error while reading headers file: " + fileName);
        WScript.Echo(err.message);
        WScript.Echo("Will return empty array");
        return resultArray;
    }
}

function trim(str) {
    return str.replace(/^\s+/, '').replace(/\s+$/, '');
}

function main() {
    parseArgs();
    request(url);
}
main();

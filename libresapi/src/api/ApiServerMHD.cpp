/*******************************************************************************
 * libresapi/api/ApiServerMHD.cpp                                              *
 *                                                                             *
 * LibResAPI: API for local socket server                                      *
 *                                                                             *
 * Copyright 2018 by Retroshare Team <retroshare.project@gmail.com>            *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
#include "ApiServerMHD.h"

#include <iostream>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstdio>
#include <algorithm>

#include <util/rsdir.h>
#include "util/ContentTypes.h"

// for filestreamer
#include <retroshare/rsfiles.h>

// to determine default docroot
#include <retroshare/rsinit.h>

#include "JsonStream.h"
#include "ApiServer.h"

#if MHD_VERSION < 0x00090000
    // very old version, probably v0.4.x on old debian/ubuntu
    #define OLD_04_MHD_FIX
#endif

// filestreamer only works if a content reader callback is allowed to return 0
// this is allowed since libmicrohttpd revision 30402
#if MHD_VERSION >= 0x00093101 // 0.9.31-1
    #define ENABLE_FILESTREAMER
#else
    #warning libmicrohttpd is too old to support file streaming. upgrade to a newer version.
#endif

#ifdef OLD_04_MHD_FIX
#define MHD_CONTENT_READER_END_OF_STREAM ((size_t) -1LL)
/**
 * Create a response object. The response object can be extended with
 * header information and then be used any number of times.
 *
 * @param size size of the data portion of the response
 * @param fd file descriptor referring to a file on disk with the
 * data; will be closed when response is destroyed;
 * fd should be in 'blocking' mode
 * @return NULL on error (i.e. invalid arguments, out of memory)
 * @ingroup response
 */
struct MHD_Response * MHD_create_response_from_fd(size_t size, int fd)
{
	unsigned char *buf = (unsigned char *)malloc(size) ;

    if(buf == 0)
    {
        std::cerr << "replacement MHD_create_response_from_fd: malloc failed, size was " << size << std::endl;
        close(fd);
        return NULL ;
    }
    if(read(fd,buf,size) != size)
	{
        std::cerr << "replacement MHD_create_response_from_fd: READ error in file descriptor " << fd <<  " requested read size was " << size << std::endl;
        close(fd);
		free(buf) ;
		return NULL ;
	}
	else
    {
        close(fd);
        return MHD_create_response_from_data(size, buf,1,0) ;
    }
}
#endif

namespace resource_api{

std::string getDefaultDocroot()
{
    return RsAccounts::systemDataDirectory(false) + "/webui";
}

const char* API_ENTRY_PATH = "/api/v2";
const char* FILESTREAMER_ENTRY_PATH = "/fstream/";
const char* STATIC_FILES_ENTRY_PATH = "/static/";
const char* UPLOAD_ENTRY_PATH = "/upload/";

static void secure_queue_response(MHD_Connection *connection, unsigned int status_code, struct MHD_Response* response);
static void sendMessage(MHD_Connection *connection, unsigned int status, std::string message);

// interface for request handler classes
class MHDHandlerBase
{
public:
    virtual ~MHDHandlerBase(){}
    // return MHD_NO to terminate connection
    // return MHD_YES otherwise
    // this function will get called by MHD until a response was queued
    virtual int handleRequest(  struct MHD_Connection *connection,
                                const char *url, const char *method, const char *version,
                                const char *upload_data, size_t *upload_data_size) = 0;
};

// handles calls to the resource_api
class MHDUploadHandler: public MHDHandlerBase
{
public:
    MHDUploadHandler(ApiServer* s): mState(BEGIN), mApiServer(s){}
    virtual ~MHDUploadHandler(){}
    // return MHD_NO or MHD_YES
    virtual int handleRequest(  struct MHD_Connection *connection,
                                const char */*url*/, const char *method, const char */*version*/,
                                const char *upload_data, size_t *upload_data_size)
    {
        // new request
        if(mState == BEGIN)
        {
            if(strcmp(method, "POST") == 0)
            {
                mState = WAITING_DATA;
                // first time there is no data, do nothing and return
                return MHD_YES;
            }
        }
        if(mState == WAITING_DATA)
        {
            if(upload_data && *upload_data_size)
            {
                mRequesString += std::string(upload_data, *upload_data_size);
                *upload_data_size = 0;
                return MHD_YES;
            }
        }

        std::vector<uint8_t> bytes(mRequesString.begin(), mRequesString.end());

        int id = mApiServer->getTmpBlobStore()->storeBlob(bytes);

        resource_api::JsonStream responseStream;
        if(id)
            responseStream << makeKeyValue("ok", true);
        else
            responseStream << makeKeyValue("ok", false);

        responseStream << makeKeyValueReference("id", id);

        std::string result = responseStream.getJsonString();

        struct MHD_Response* resp = MHD_create_response_from_data(result.size(), (void*)result.data(), 0, 1);

        MHD_add_response_header(resp, "Content-Type", "application/json");

        secure_queue_response(connection, MHD_HTTP_OK, resp);
        MHD_destroy_response(resp);
        return MHD_YES;
    }
    enum State {BEGIN, WAITING_DATA};
    State mState;
    std::string mRequesString;
    ApiServer* mApiServer;
};

// handles calls to the resource_api
class MHDApiHandler: public MHDHandlerBase
{
public:
    MHDApiHandler(ApiServer* s): mState(BEGIN), mApiServer(s){}
    virtual ~MHDApiHandler(){}
    // return MHD_NO or MHD_YES
    virtual int handleRequest(  struct MHD_Connection *connection,
                                const char *url, const char *method, const char */*version*/,
                                const char *upload_data, size_t *upload_data_size)
    {
        // new request
        if(mState == BEGIN)
        {
            if(strcmp(method, "POST") == 0)
            {
                mState = WAITING_DATA;
                // first time there is no data, do nothing and return
                return MHD_YES;
            }
        }
        if(mState == WAITING_DATA)
        {
            if(upload_data && *upload_data_size)
            {
                mRequesString += std::string(upload_data, *upload_data_size);
                *upload_data_size = 0;
                return MHD_YES;
            }
        }

        if(strstr(url, API_ENTRY_PATH) != url)
        {
            std::cerr << "FATAL ERROR in MHDApiHandler::handleRequest(): url does not start with api entry path, which is \"" << API_ENTRY_PATH << "\"" << std::endl;
            return MHD_NO;
        }
        std::string path2 = (url + strlen(API_ENTRY_PATH));

        resource_api::JsonStream instream;
        instream.setJsonString(mRequesString);
        resource_api::Request req(instream);

        if(strcmp(method, "GET") == 0)
        {
            req.mMethod = resource_api::Request::GET;
        }
        else if(strcmp(method, "POST") == 0)
        {
            req.mMethod = resource_api::Request::PUT;
        }
        else if(strcmp(method, "DELETE") == 0)
        {
            req.mMethod = resource_api::Request::DELETE_AA;
        }

		req.setPath(path2);

        std::string result = mApiServer->handleRequest(req);

        struct MHD_Response* resp = MHD_create_response_from_data(result.size(), (void*)result.data(), 0, 1);

        // EVIL HACK remove
        if(result[0] != '{')
            MHD_add_response_header(resp, "Content-Type", "image/png");
        else
            MHD_add_response_header(resp, "Content-Type", "application/json");

        secure_queue_response(connection, MHD_HTTP_OK, resp);
        MHD_destroy_response(resp);
        return MHD_YES;
    }
    enum State {BEGIN, WAITING_DATA};
    State mState;
    std::string mRequesString;
    ApiServer* mApiServer;
};

#ifdef ENABLE_FILESTREAMER
class MHDFilestreamerHandler: public MHDHandlerBase
{
public:
    MHDFilestreamerHandler(): mSize(0){}
    virtual ~MHDFilestreamerHandler(){}

    RsFileHash mHash;
    uint64_t mSize;

    // return MHD_NO or MHD_YES
    virtual int handleRequest(  struct MHD_Connection *connection,
                                const char *url, const char */*method*/, const char */*version*/,
                                const char */*upload_data*/, size_t */*upload_data_size*/)
    {
        if(rsFiles == 0)
        {
            sendMessage(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "Error: rsFiles is null. Retroshare is probably not yet started.");
            return MHD_YES;
        }		
		std::string urls(url);
		urls = urls.substr(strlen(FILESTREAMER_ENTRY_PATH));
		size_t perpos = urls.find('/');
		if(perpos == std::string::npos){
			mHash = RsFileHash(urls);
		}else{
			mHash = RsFileHash(urls.substr(0, perpos));
		}
		if(urls.empty() || mHash.isNull())
		{
			sendMessage(connection, MHD_HTTP_NOT_FOUND, "Error: URL is not a valid file hash");
			return MHD_YES;
		}

        FileInfo info;
        std::list<RsFileHash> dls;
        rsFiles->FileDownloads(dls);
        if(!(rsFiles->alreadyHaveFile(mHash, info) || std::find(dls.begin(), dls.end(), mHash) != dls.end()))
        {
            sendMessage(connection, MHD_HTTP_NOT_FOUND, "Error: file not existing on local peer and not downloading. Start the download before streaming it.");
            return MHD_YES;
        }
        mSize = info.size;

        struct MHD_Response* resp = MHD_create_response_from_callback(
                    mSize, 1024*1024, &contentReadercallback, this, NULL);

		// get content-type from extension
		std::string ext = "";
        std::string::size_type i = info.fname.rfind('.');
		if(i != std::string::npos)
			ext = info.fname.substr(i+1);
		MHD_add_response_header(resp, "Content-Type", ContentTypes::cTypeFromExt(ext).c_str());

        secure_queue_response(connection, MHD_HTTP_OK, resp);
        MHD_destroy_response(resp);
        return MHD_YES;
    }

    static ssize_t contentReadercallback(void *cls, uint64_t pos, char *buf, size_t max)
    {
        MHDFilestreamerHandler* handler = (MHDFilestreamerHandler*)cls;
        if(pos >= handler->mSize)
            return MHD_CONTENT_READER_END_OF_STREAM;
        uint32_t size_to_send = max;
        if(!rsFiles->getFileData(handler->mHash, pos, size_to_send, (uint8_t*)buf))
            return 0;
        return size_to_send;
    }
};
#endif // ENABLE_FILESTREAMER

// MHD will call this for each element of the http header
static int _extract_host_header_it_cb(void *cls,
                         enum MHD_ValueKind kind,
                         const char *key,
                         const char *value)
{
    if(kind == MHD_HEADER_KIND)
    {
        // check if key is host
        const char* h = "host";
        while(*key && *h)
        {
            if(tolower(*key) != *h)
                return MHD_YES;
            key++;
            h++;
        }
        // strings have same length and content
        if(*key == 0 && *h == 0)
        {
            *((std::string*)cls) = value;
        }
    }
    return MHD_YES;
}

// add security related headers and send the response on the given connection
// the reference counter is not touched
// this function is a wrapper around MHD_queue_response
// MHD_queue_response should be replaced with this function
static void secure_queue_response(MHD_Connection *connection, unsigned int status_code, struct MHD_Response* response)
{
    // TODO: protect againts handling untrusted content to the browser
    // see:
    // http://www.dotnetnoob.com/2012/09/security-through-http-response-headers.html
    // http://www.w3.org/TR/CSP2/
    // https://code.google.com/p/doctype-mirror/wiki/ArticleContentSniffing

    // check content type
    // don't server when no type or no whitelisted type is given
    // TODO sending invalid mime types is as bad as not sending them TODO
    /*
    std::vector<std::string> allowed_types;
    allowed_types.push_back("text/html");
    allowed_types.push_back("application/json");
    allowed_types.push_back("image/png");
    */
    const char* type = MHD_get_response_header(response, "Content-Type");
    if(type == 0 /*|| std::find(allowed_types.begin(), allowed_types.end(), std::string(type)) == allowed_types.end()*/)
    {
        std::string page;
        if(type == 0)
            page = "<html><body><p>Fatal Error: no content type was set on this response. This is a bug.</p></body></html>";
        else
            page = "<html><body><p>Fatal Error: this content type is not allowed. This is a bug.<br/> Content-Type: "+std::string(type)+"</p></body></html>";
        struct MHD_Response* resp = MHD_create_response_from_data(page.size(), (void*)page.data(), 0, 1);
        MHD_add_response_header(resp, "Content-Type", "text/html");
        MHD_queue_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, resp);
        MHD_destroy_response(resp);
    }

    // tell Internet Explorer to not do content sniffing
    MHD_add_response_header(response, "X-Content-Type-Options", "nosniff");

    // Prevent clickjacking attacks (also prevented by CSP, but not in all browsers, including FireFox)
    MHD_add_response_header(response, "X-Frame-Options", "SAMEORIGIN");

    // Content security policy header, its a new technology and not implemented everywhere

    // get own host name as the browser sees it
    std::string host;
    MHD_get_connection_values(connection, MHD_HEADER_KIND, _extract_host_header_it_cb, (void*)&host);

    std::string csp;
    csp += "default-src 'none';";
    csp += "script-src '"+host+STATIC_FILES_ENTRY_PATH+"';";
    csp += "font-src '"+host+STATIC_FILES_ENTRY_PATH+"';";
    csp += "img-src 'self';"; // allow images from all paths on this server
    csp += "media-src 'self';"; // allow media files from all paths on this server

    MHD_add_response_header(response, "X-Content-Security-Policy", csp.c_str());

    MHD_queue_response(connection, status_code, response);
}

// wraps the given string in a html page and sends it as response with the given status code
static void sendMessage(MHD_Connection *connection, unsigned int status, std::string message)
{
    std::string page = "<html><body><p>"+message+"</p></body></html>";
    struct MHD_Response* resp = MHD_create_response_from_data(page.size(), (void*)page.data(), 0, 1);
    MHD_add_response_header(resp, "Content-Type", "text/html");
    secure_queue_response(connection, status, resp);
    MHD_destroy_response(resp);
}

// convert all character to hex html entities
static std::string escape_html(std::string in)
{
    std::string out;
    for(uint32_t i = 0; i < in.size(); i++)
    {
        char a = (in[i]&0xF0)>>4;
        a = a < 10? a+'0': a-10+'A';
        char b = (in[i]&0x0F);
        b = b < 10? b+'0': b-10+'A';
        out += std::string("&#x")+a+b+";";
    }
    return out;
}

ApiServerMHD::ApiServerMHD(ApiServer *server):
    mConfigOk(false), mDaemon(0), mApiServer(server)
{
    memset(&mListenAddr, 0, sizeof(mListenAddr));
}

ApiServerMHD::~ApiServerMHD()
{
    stop();
}

bool ApiServerMHD::configure(std::string docroot, uint16_t port, std::string /*bind_address*/, bool allow_from_all)
{
    mRootDir = docroot;
    // make sure the docroot dir ends with a slash
    if(mRootDir.empty())
        mRootDir = "./";
    else if (mRootDir[mRootDir.size()-1] != '/' && mRootDir[mRootDir.size()-1] != '\\')
        mRootDir += "/";

    mListenAddr.sin_family = AF_INET;
    mListenAddr.sin_port = htons(port);

    // untested
    /*
    if(!bind_address.empty())
    {
        if(!inet_pton(AF_INET6, bind_address.c_str(), &mListenAddr.sin6_addr))
        {
            std::cerr << "ApiServerMHD::configure() invalid bind address: \"" << bind_address << "\"" << std::endl;
            return false;
        }
    }
    else*/ if(allow_from_all)
    {
        mListenAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        std::cerr << "ApiServerMHD::configure(): will serve the webinterface to ALL IP adresses." << std::endl;
    }
    else
    {
        mListenAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        std::cerr << "ApiServerMHD::configure(): will serve the webinterface to LOCALHOST only." << std::endl;
    }

    mConfigOk = true;
    return true;
}

bool ApiServerMHD::start()
{
    if(!mConfigOk)
    {
        std::cerr << "ApiServerMHD::start() ERROR: server not configured. You have to call configure() first." << std::endl;
        return false;
    }
    if(mDaemon)
    {
        std::cerr << "ApiServerMHD::start() ERROR: server already started. You have to call stop() first." << std::endl;
        return false;
    }
    mDaemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, 9999, // port will be overwritten by MHD_OPTION_SOCK_ADDR
                               &static_acceptPolicyCallback, this,
                               &static_accessHandlerCallback, this,
                               MHD_OPTION_NOTIFY_COMPLETED, &static_requestCompletedCallback, this,
                               MHD_OPTION_SOCK_ADDR, &mListenAddr,
                               MHD_OPTION_END);
    if(mDaemon)
    {
        std::cerr << "ApiServerMHD::start() SUCCESS. Started server on port " << ntohs(mListenAddr.sin_port) << ". Serving files from \"" << mRootDir << "\" at " << STATIC_FILES_ENTRY_PATH << std::endl;
        return true;
    }
    else
    {
        std::cerr << "ApiServerMHD::start() ERROR: starting the server failed. Maybe port " << ntohs(mListenAddr.sin_port) << " is already in use?" << std::endl;
        return false;
    }
}

void ApiServerMHD::stop()
{
    if(mDaemon == 0)
        return;
    MHD_stop_daemon(mDaemon);
    mDaemon = 0;
}

int ApiServerMHD::static_acceptPolicyCallback(void *cls, const sockaddr *addr, socklen_t addrlen)
{
    return ((ApiServerMHD*)cls)->acceptPolicyCallback(addr, addrlen);
}

int ApiServerMHD::static_accessHandlerCallback(void* cls, struct MHD_Connection * connection,
                                              const char *url, const char *method, const char *version,
                                              const char *upload_data, size_t *upload_data_size,
                                              void **con_cls)
{
    return ((ApiServerMHD*)cls)->accessHandlerCallback(connection, url, method, version,
                                               upload_data, upload_data_size, con_cls);
}

void ApiServerMHD::static_requestCompletedCallback(void *cls, MHD_Connection* connection,
                                                   void **con_cls, MHD_RequestTerminationCode toe)
{
    ((ApiServerMHD*)cls)->requestCompletedCallback(connection, con_cls, toe);
}


int ApiServerMHD::acceptPolicyCallback(const sockaddr* /*addr*/, socklen_t /*addrlen*/)
{
    // accept all connetions
    return MHD_YES;
}

int ApiServerMHD::accessHandlerCallback(MHD_Connection *connection,
                                       const char *url, const char *method, const char *version,
                                       const char *upload_data, size_t *upload_data_size,
                                       void **con_cls)
{
    // is this call a continuation for an existing request?
    if(*con_cls)
    {
        return ((MHDHandlerBase*)(*con_cls))->handleRequest(connection, url, method, version, upload_data, upload_data_size);
    }

    // these characters are not allowed in the url, raise an error if they occur
    // reason: don't want to serve files outside the current document root
    const char *double_dots = "..";
    if(strstr(url, double_dots))
    {
        const char *error = "<html><body><p>Fatal error: found double dots (\"..\") in the url. This is not allowed</p></body></html>";
        struct MHD_Response* resp = MHD_create_response_from_data(strlen(error), (void*)error, 0, 1);
        MHD_add_response_header(resp, "Content-Type", "text/html");
        secure_queue_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, resp);
        MHD_destroy_response(resp);
        return MHD_YES;
    }

    // if no path is given, redirect to index.html in static files directory
    if(strlen(url) == 1 && url[0] == '/')
    {
        std::string location = std::string(STATIC_FILES_ENTRY_PATH) + "index.html";
        std::string errstr = "<html><body><p>Webinterface is at <a href=\""+location+"\">"+location+"</a></p></body></html>";
        const char *error = errstr.c_str();
        struct MHD_Response* resp = MHD_create_response_from_data(strlen(error), (void*)error, 0, 1);
        MHD_add_response_header(resp, "Content-Type", "text/html");
        MHD_add_response_header(resp, "Location", location.c_str());
        secure_queue_response(connection, MHD_HTTP_FOUND, resp);
        MHD_destroy_response(resp);
        return MHD_YES;
    }
    // is it a call to the resource api?
    if(strstr(url, API_ENTRY_PATH) == url)
    {
        // create a new handler and store it in con_cls
        MHDHandlerBase* handler = new MHDApiHandler(mApiServer);
        *con_cls = (void*) handler;
        return handler->handleRequest(connection, url, method, version, upload_data, upload_data_size);
    }
    // is it a call to the filestreamer?
    if(strstr(url, FILESTREAMER_ENTRY_PATH) == url)
    {
#ifdef ENABLE_FILESTREAMER
        // create a new handler and store it in con_cls
        MHDHandlerBase* handler = new MHDFilestreamerHandler();
        *con_cls = (void*) handler;
        return handler->handleRequest(connection, url, method, version, upload_data, upload_data_size);
#else
        sendMessage(connection, MHD_HTTP_NOT_FOUND, "The filestreamer is not available, because this executable was compiled with a too old version of libmicrohttpd.");
        return MHD_YES;
#endif
    } 
    // is it a path to the static files?
    if(strstr(url, STATIC_FILES_ENTRY_PATH) == url)
    {
        url = url + strlen(STATIC_FILES_ENTRY_PATH);
        // else server static files
        std::string filename = mRootDir + url;
        // important: binary open mode (windows)
        // else libmicrohttpd will replace crlf with lf and add garbage at the end of the file
#ifdef O_BINARY
        int fd = open(filename.c_str(), O_RDONLY | O_BINARY);
#else
        int fd = open(filename.c_str(), O_RDONLY);
#endif
        if(fd == -1)
        {
            std::string direxists;
            if(RsDirUtil::checkDirectory(mRootDir))
                direxists = "directory &quot;"+mRootDir+"&quot; exists";
            else
                direxists = "directory &quot;"+mRootDir+"&quot; does not exist!";
            std::string msg = "<html><body><p>Error: can't open the requested file. path=&quot;"+escape_html(filename)+"&quot;</p><p>"+direxists+"</p></body></html>";
            sendMessage(connection, MHD_HTTP_NOT_FOUND, msg);
            return MHD_YES;
        }

        struct stat s;
        if(fstat(fd, &s) == -1)
        {
            close(fd);
            const char *error = "<html><body><p>Error: file was opened but stat failed.</p></body></html>";
            struct MHD_Response* resp = MHD_create_response_from_data(strlen(error), (void*)error, 0, 1);
            MHD_add_response_header(resp, "Content-Type", "text/html");
            secure_queue_response(connection, MHD_HTTP_NOT_FOUND, resp);
            MHD_destroy_response(resp);
            return MHD_YES;
        }

        // find the file extension and the content type
        std::string extension;
        int i = filename.size()-1;
        while(i >= 0 && filename[i] != '.')
        {
            extension = filename[i] + extension;
            i--;
        };

        struct MHD_Response* resp = MHD_create_response_from_fd(s.st_size, fd);
		MHD_add_response_header(resp, "Content-Type", ContentTypes::cTypeFromExt(extension).c_str());
        secure_queue_response(connection, MHD_HTTP_OK, resp);
        MHD_destroy_response(resp);
        return MHD_YES;
    }

    if(strstr(url, UPLOAD_ENTRY_PATH) == url)
    {
        // create a new handler and store it in con_cls
        MHDHandlerBase* handler = new MHDUploadHandler(mApiServer);
        *con_cls = (void*) handler;
        return handler->handleRequest(connection, url, method, version, upload_data, upload_data_size);
    }

    // if url is not a valid path, then serve a help page
    sendMessage(connection, MHD_HTTP_NOT_FOUND,
                "This address is invalid. Try one of the adresses below:<br/>"
                "<ul>"
                "<li>/ <br/>Retroshare webinterface</li>"
                "<li>"+std::string(API_ENTRY_PATH)+" <br/>JSON over http api</li>"
                "<li>"+std::string(FILESTREAMER_ENTRY_PATH)+" <br/>file streamer</li>"
                "<li>"+std::string(STATIC_FILES_ENTRY_PATH)+" <br/>static files</li>"
                "</ul>"
                );
    return MHD_YES;
}

void ApiServerMHD::requestCompletedCallback(struct MHD_Connection */*connection*/,
                                            void **con_cls, MHD_RequestTerminationCode /*toe*/)
{
    if(*con_cls)
    {
        delete (MHDHandlerBase*)(*con_cls);
    }
}

} // namespace resource_api

#include "ApiServerMHD.h"

#include <iostream>
#include <sys/stat.h>
#include <cstdio>
#include <algorithm>

// for filestreamer
#include <retroshare/rsfiles.h>

#include "api/JsonStream.h"

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

    if(buf == 0 || read(fd,buf,size) != size)
	{
        std::cerr << "malloc failed or READ error in file descriptor " << fd <<  " requested read size was " << size << std::endl;
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

const char* API_ENTRY_PATH = "/api/v2";
const char* FILESTREAMER_ENTRY_PATH = "/fstream/";

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
class MHDApiHandler: public MHDHandlerBase
{
public:
    MHDApiHandler(ApiServer* s): mState(BEGIN), mApiServer(s){}
    virtual ~MHDApiHandler(){}
    // return MHD_NO or MHD_YES
    virtual int handleRequest(  struct MHD_Connection *connection,
                                const char *url, const char *method, const char *version,
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

        std::stack<std::string> stack;
        std::string str;
        for(std::string::reverse_iterator sit = path2.rbegin(); sit != path2.rend(); sit++)
        {
            if((*sit) != '/')
            {
                // add to front because we are traveling in reverse order
                str = *sit + str;
            }
            else
            {
                if(str != "")
                {
                    stack.push(str);
                    str.clear();
                }
            }
        }
        if(str != "")
        {
            stack.push(str);
        }
        req.mPath = stack;
        req.mFullPath = path2;

        std::string result = mApiServer->handleRequest(req);

        struct MHD_Response* resp = MHD_create_response_from_data(result.size(), (void*)result.data(), 0, 1);

        // EVIL HACK remove
        if(result[0] != '{')
            MHD_add_response_header(resp, "Content-Type", "image/png");
        else
            MHD_add_response_header(resp, "Content-Type", "text/plain");

        MHD_queue_response(connection, MHD_HTTP_OK, resp);
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
                                const char *url, const char *method, const char *version,
                                const char *upload_data, size_t *upload_data_size)
    {
        if(rsFiles == 0)
        {
            ApiServerMHD::sendMessage(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "Error: rsFiles is null. Retroshare is probably not yet started.");
            return MHD_YES;
        }
        if(url[0] == 0 || (mHash=RsFileHash(url+strlen(FILESTREAMER_ENTRY_PATH))).isNull())
        {
            ApiServerMHD::sendMessage(connection, MHD_HTTP_NOT_FOUND, "Error: URL is not a valid file hash");
            return MHD_YES;
        }
        FileInfo info;
        std::list<RsFileHash> dls;
        rsFiles->FileDownloads(dls);
        if(!(rsFiles->alreadyHaveFile(mHash, info) || std::find(dls.begin(), dls.end(), mHash) != dls.end()))
        {
            ApiServerMHD::sendMessage(connection, MHD_HTTP_NOT_FOUND, "Error: file not existing on local peer and not downloading. Start the download before streaming it.");
            return MHD_YES;
        }
        mSize = info.size;

        struct MHD_Response* resp = MHD_create_response_from_callback(
                    mSize, 1024*1024, &contentReadercallback, this, NULL);
        MHD_queue_response(connection, MHD_HTTP_OK, resp);
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

ApiServerMHD::ApiServerMHD():
    mConfigOk(false), mDaemon(0)
{
}

ApiServerMHD::~ApiServerMHD()
{
    stop();
}

bool ApiServerMHD::configure(std::string docroot, uint16_t port, std::string bind_address, bool allow_from_all)
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
        mListenAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
        std::cerr << "ApiServerMHD::configure(): will serve the webinterface to ALL IP adresses." << std::endl;
    }
    else
    {
        mListenAddr.sin_addr.S_un.S_addr = htonl(INADDR_LOOPBACK);
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
        std::cerr << "ApiServerMHD::start() SUCCESS. Started server on port " << ntohs(mListenAddr.sin_port) << ". Serving files from \"" << mRootDir << "\" at /" << std::endl;
        return true;
    }
    else
    {
        std::cerr << "ApiServerMHD::start() ERROR: starting the server failed." << std::endl;
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

/*static*/ void ApiServerMHD::sendMessage(MHD_Connection *connection, unsigned int status, std::string message)
{
    std::string page = "<html><body><p>"+message+"</p></body></html>";
    struct MHD_Response* resp = MHD_create_response_from_data(page.size(), (void*)page.data(), 0, 1);
    MHD_add_response_header(resp, "Content-Type", "text/html");
    MHD_queue_response(connection, status, resp);
    MHD_destroy_response(resp);
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

    // these characters are not allowe in the url, raise an error if they occour
    // reason: don't want to serve files outside the current document root
    const char *double_dots = "..";
    if(strstr(url, double_dots))
    {
        const char *error = "<html><body><p>Fatal error: found double dots (\"..\") in the url. This is not allowed</p></body></html>";
        struct MHD_Response* resp = MHD_create_response_from_data(strlen(error), (void*)error, 0, 1);
        MHD_add_response_header(resp, "Content-Type", "text/html");
        MHD_queue_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, resp);
        MHD_destroy_response(resp);
        return MHD_YES;
    }

    // is it a call to the resource api?
    if(strstr(url, API_ENTRY_PATH) == url)
    {
        // create a new handler and store it in con_cls
        MHDHandlerBase* handler = new MHDApiHandler(&mApiServer);
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

    // else server static files
    std::string filename = mRootDir + url;
    // important: binary open mode,
    // else libmicrohttpd will replace crlf with lf and add garbage at the end of the file
    FILE* fd = fopen(filename.c_str(), "rb");
    if(fd == 0)
    {
        const char *error = "<html><body><p>Error: can't open the requested file.</p></body></html>";
        struct MHD_Response* resp = MHD_create_response_from_data(strlen(error), (void*)error, 0, 1);
        MHD_add_response_header(resp, "Content-Type", "text/html");
        MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, resp);
        MHD_destroy_response(resp);
        return MHD_YES;
    }

    struct stat s;
    if(fstat(fileno(fd), &s) == -1)
    {
        const char *error = "<html><body><p>Error: file was opened but stat failed.</p></body></html>";
        struct MHD_Response* resp = MHD_create_response_from_data(strlen(error), (void*)error, 0, 1);
        MHD_add_response_header(resp, "Content-Type", "text/html");
        MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, resp);
        MHD_destroy_response(resp);
        return MHD_YES;
    }

    struct MHD_Response* resp = MHD_create_response_from_fd(s.st_size, fileno(fd));
    MHD_queue_response(connection, MHD_HTTP_OK, resp);
    MHD_destroy_response(resp);
    return MHD_YES;
}

void ApiServerMHD::requestCompletedCallback(struct MHD_Connection *connection,
                                            void **con_cls, MHD_RequestTerminationCode toe)
{
    if(*con_cls)
    {
        delete (MHDHandlerBase*)(*con_cls);
    }
}

} // namespace resource_api

#pragma once
#include <api/ApiServer.h>
#include <util/rsthreads.h>

namespace resource_api {

// allows basic control from stdin/stdout
// - account selection
// - login
// - shutdown
class TerminalApiClient: private RsTickingThread{
public:
    // zero setup: create an instance of this class and destroy it when not needed anymore
    // no need to call start or stop or something
    // parameter api must not be null
    TerminalApiClient(ApiServer* api);
    ~TerminalApiClient();
protected:
    // from RsThread
    virtual void data_tick(); /* called once the thread is started. Should be overloaded by subclasses. */
private:
    void waitForResponse(ApiServer::RequestId id);
    bool isTokenValid(StateToken st);
    ApiServer* mApiServer;
};

} // namespace resource_api

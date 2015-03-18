#include "TransfersHandler.h"
#include "Operators.h"
#include <algorithm>

namespace resource_api
{

TransfersHandler::TransfersHandler(StateTokenServer *sts, RsFiles *files):
    mStateTokenServer(sts), mFiles(files), mLastUpdateTS(0)
{
    addResourceHandler("*", this, &TransfersHandler::handleWildcard);
    addResourceHandler("downloads", this, &TransfersHandler::handleDownloads);
    addResourceHandler("control_download", this, &TransfersHandler::handleControlDownload);
    mStateToken = mStateTokenServer->getNewToken();
    mStateTokenServer->registerTickClient(this);
}

TransfersHandler::~TransfersHandler()
{
    mStateTokenServer->unregisterTickClient(this);
}

const int UPDATE_PERIOD_SECONDS = 5;

void TransfersHandler::tick()
{
    if(time(0) > (mLastUpdateTS + UPDATE_PERIOD_SECONDS))
        mStateTokenServer->replaceToken(mStateToken);

    // extra check: was the list of files changed?
    // if yes, replace state token immediately
    std::list<RsFileHash> dls;
    mFiles->FileDownloads(dls);
    // there is no guarantee of the order
    // so have to sort before comparing the lists
    dls.sort();
    if(!std::equal(dls.begin(), dls.end(), mDownloadsAtLastCheck.begin()))
    {
        mDownloadsAtLastCheck.swap(dls);
        mStateTokenServer->replaceToken(mStateToken);
    }
}

void TransfersHandler::handleWildcard(Request &req, Response &resp)
{

}

void TransfersHandler::handleControlDownload(Request &req, Response &resp)
{
    mStateTokenServer->replaceToken(mStateToken);
    RsFileHash hash;
    std::string action;
    req.mStream << makeKeyValueReference("action", action);
    if(action == "begin")
    {
        std::string fname;
        double size;
        req.mStream << makeKeyValueReference("name", fname);
        req.mStream << makeKeyValueReference("size", size);
        req.mStream << makeKeyValueReference("hash", hash);
        std::list<RsPeerId> scrIds;
        bool ok = req.mStream.isOK();
        if(ok)
            ok = mFiles->FileRequest(fname, hash, size, "", RS_FILE_REQ_ANONYMOUS_ROUTING, scrIds);
        if(ok)
            resp.setOk();
        else
            resp.setFail("something went wrong. are all fields filled in? is the file already downloaded?");
        return;
    }

    req.mStream << makeKeyValueReference("id", hash);
    if(!req.mStream.isOK())
    {
        resp.setFail("error: could not deserialise the request");
        return;
    }
    bool ok = false;
    bool handled = false;
    if(action == "pause")
    {
        handled = true;
        ok = mFiles->FileControl(hash, RS_FILE_CTRL_PAUSE);
    }
    if(action == "start")
    {
        handled = true;
        ok = mFiles->FileControl(hash, RS_FILE_CTRL_START);
    }
    if(action == "check")
    {
        handled = true;
        ok = mFiles->FileControl(hash, RS_FILE_CTRL_FORCE_CHECK);
    }
    if(action == "cancel")
    {
        handled = true;
        ok = mFiles->FileCancel(hash);
    }
    if(ok)
        resp.setOk();
    else
        resp.setFail("something went wrong. not sure what or why.");
    if(handled)
        return;
    resp.setFail("error: action not handled");
}

void TransfersHandler::handleDownloads(Request &req, Response &resp)
{
    tick();
    resp.mStateToken = mStateToken;
    resp.mDataStream.getStreamToMember();
    for(std::list<RsFileHash>::iterator lit = mDownloadsAtLastCheck.begin();
        lit != mDownloadsAtLastCheck.end(); ++lit)
    {
        FileInfo fi;
        if(mFiles->FileDetails(*lit, RS_FILE_HINTS_DOWNLOAD, fi))
        {
            StreamBase& stream = resp.mDataStream.getStreamToMember();
            stream << makeKeyValueReference("id", fi.hash)
                   << makeKeyValueReference("hash", fi.hash)
                   << makeKeyValueReference("name", fi.fname);
            double size = fi.size;
            double transfered = fi.transfered;
            stream << makeKeyValueReference("size", size)
                   << makeKeyValueReference("transfered", transfered);

            std::string dl_status;
            /*
            const uint32_t FT_STATE_FAILED			= 0x0000 ;
            const uint32_t FT_STATE_OKAY				= 0x0001 ;
            const uint32_t FT_STATE_WAITING 			= 0x0002 ;
            const uint32_t FT_STATE_DOWNLOADING		= 0x0003 ;
            const uint32_t FT_STATE_COMPLETE 		= 0x0004 ;
            const uint32_t FT_STATE_QUEUED   		= 0x0005 ;
            const uint32_t FT_STATE_PAUSED   		= 0x0006 ;
            const uint32_t FT_STATE_CHECKING_HASH	= 0x0007 ;
            */
            switch(fi.downloadStatus)
            {
            case FT_STATE_FAILED:
                dl_status = "failed";
                break;
            case FT_STATE_OKAY:
                dl_status = "okay";
                break;
            case FT_STATE_WAITING:
                dl_status = "waiting";
                break;
            case FT_STATE_DOWNLOADING:
                dl_status = "downloading";
                break;
            case FT_STATE_COMPLETE:
                dl_status = "complete";
                break;
            case FT_STATE_QUEUED:
                dl_status = "queued";
                break;
            case FT_STATE_PAUSED:
                dl_status = "paused";
                break;
            case FT_STATE_CHECKING_HASH:
                dl_status = "checking";
                break;
            default:
                dl_status = "error_unknown";
            }

            stream << makeKeyValueReference("download_status", dl_status);
        }
    }
    resp.setOk();
}

} // namespace resource_api

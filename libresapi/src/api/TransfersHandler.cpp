/*******************************************************************************
 * libresapi/api/TransfersHandler.cpp                                          *
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
#include "TransfersHandler.h"
#include "Operators.h"
#include <algorithm>
#include <time.h>

namespace resource_api
{

TransfersHandler::TransfersHandler(StateTokenServer *sts, RsFiles *files, RsPeers *peers,
                                   RsNotify& notify):
    mStateTokenServer(sts), mFiles(files), mRsPeers(peers), mNotify(notify),
   mMtx("TransfersHandler"), mLastUpdateTS(0)
{
	addResourceHandler("*", this, &TransfersHandler::handleWildcard);
	addResourceHandler("downloads", this, &TransfersHandler::handleDownloads);
	addResourceHandler("uploads", this, &TransfersHandler::handleUploads);
	addResourceHandler("control_download", this, &TransfersHandler::handleControlDownload);

	addResourceHandler( "set_file_destination_directory", this,
	                    &TransfersHandler::handleSetFileDestinationDirectory );
	addResourceHandler( "set_file_destination_name", this,
	                    &TransfersHandler::handleSetFileDestinationName );
	addResourceHandler( "set_file_chunk_strategy", this,
	                    &TransfersHandler::handleSetFileChunkStrategy );

	mStateToken = mStateTokenServer->getNewToken();
	mStateTokenServer->registerTickClient(this);
	mNotify.registerNotifyClient(this);
}

TransfersHandler::~TransfersHandler()
{
    mStateTokenServer->unregisterTickClient(this);
	mNotify.unregisterNotifyClient(this);
}

void TransfersHandler::notifyListChange(int list, int /* type */)
{
	if(list == NOTIFY_LIST_TRANSFERLIST)
	{
		RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
		mStateTokenServer->discardToken(mStateToken);
		mStateToken = mStateTokenServer->getNewToken();
	}
}

const int UPDATE_PERIOD_SECONDS = 5;

void TransfersHandler::tick()
{
    if(time(0) > (mLastUpdateTS + UPDATE_PERIOD_SECONDS))
        mStateTokenServer->replaceToken(mStateToken);

	bool replace = false;
    // extra check: was the list of files changed?
    // if yes, replace state token immediately
    std::list<RsFileHash> dls;
    mFiles->FileDownloads(dls);
    // there is no guarantee of the order
    // so have to sort before comparing the lists
    dls.sort();
	if(!std::equal(dls.begin(), dls.end(), mDownloadsAtLastCheck.begin()))
		mDownloadsAtLastCheck.swap(dls);

	std::list<RsFileHash> upls;
	mFiles->FileUploads(upls);

	upls.sort();
	if(!std::equal(upls.begin(), upls.end(), mUploadsAtLastCheck.begin()))
		mUploadsAtLastCheck.swap(upls);

	if(replace)
		mStateTokenServer->replaceToken(mStateToken);
}

void TransfersHandler::handleWildcard(Request & /*req*/, Response & /*resp*/)
{

}

void TransfersHandler::handleControlDownload(Request &req, Response &resp)
{
    mStateTokenServer->replaceToken(mStateToken);

	std::string hashString;
    std::string action;
    req.mStream << makeKeyValueReference("action", action);
	req.mStream << makeKeyValueReference("hash", hashString);
	RsFileHash hash(hashString);

    if(action == "begin")
    {
        std::string fname;
        double size;
        req.mStream << makeKeyValueReference("name", fname);
		req.mStream << makeKeyValueReference("size", size);

		std::list<RsPeerId> srcIds;
		FileInfo finfo;
		mFiles->FileDetails(hash, RS_FILE_HINTS_REMOTE, finfo);

		for(std::vector<TransferInfo>::const_iterator it(finfo.peers.begin());it!=finfo.peers.end();++it)
			srcIds.push_back((*it).peerId);

        bool ok = req.mStream.isOK();
        if(ok)
			ok = mFiles->FileRequest(fname, hash, size, "", RS_FILE_REQ_ANONYMOUS_ROUTING, srcIds);
        if(ok)
            resp.setOk();
        else
            resp.setFail("something went wrong. are all fields filled in? is the file already downloaded?");
        return;
    }

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

void TransfersHandler::handleDownloads(Request & /* req */, Response &resp)
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
			       << makeKeyValueReference("transferred", transfered)
                   << makeKeyValue("transfer_rate", fi.tfRate);

            std::string dl_status;

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

void TransfersHandler::handleUploads(Request & /* req */, Response &resp)
{
	tick();
	resp.mStateToken = mStateToken;
	resp.mDataStream.getStreamToMember();

	RsPeerId ownId = mRsPeers->getOwnId();

	for(std::list<RsFileHash>::iterator lit = mUploadsAtLastCheck.begin();
	    lit != mUploadsAtLastCheck.end(); ++lit)
	{
		FileInfo fi;
		if(mFiles->FileDetails(*lit, RS_FILE_HINTS_UPLOAD, fi))
		{
			for( std::vector<TransferInfo>::iterator pit = fi.peers.begin(); pit != fi.peers.end(); ++pit)
			{
				if (pit->peerId == ownId) //don't display transfer to ourselves
					continue ;

				std::string sourceName = mRsPeers->getPeerName(pit->peerId);
				bool isAnon = false;
				bool isEncryptedE2E = false;

				if(sourceName == "")
				{
					isAnon = true;
					sourceName = pit->peerId.toStdString();

					if(rsFiles->isEncryptedSource(pit->peerId))
						isEncryptedE2E = true;
				}

				std::string status;
				switch(pit->status)
				{
				    case FT_STATE_FAILED:
					    status = "Failed";
					    break;
				    case FT_STATE_OKAY:
					    status = "Okay";
					    break;
				    case FT_STATE_WAITING:
					    status = "Waiting";
					    break;
				    case FT_STATE_DOWNLOADING:
					    status = "Uploading";
					    break;
				    case FT_STATE_COMPLETE:
					    status = "Complete";
					    break;
				    default:
					    status = "Complete";
					    break;
				}

				CompressedChunkMap cChunkMap;

				if(!rsFiles->FileUploadChunksDetails(*lit, pit->peerId, cChunkMap))
					continue;

				double dlspeed  	= pit->tfRate;
				double fileSize		= fi.size;
				double completed 	= pit->transfered;

				uint32_t chunk_size = 1024*1024;
				uint32_t nb_chunks = (uint32_t)((fi.size + (uint64_t)chunk_size - 1) / (uint64_t)(chunk_size));
				uint32_t filled_chunks = cChunkMap.filledChunks(nb_chunks);

				if(filled_chunks > 0 && nb_chunks > 0)
					completed = cChunkMap.computeProgress(fi.size, chunk_size);
				else
					completed = pit->transfered % chunk_size;

				resp.mDataStream.getStreamToMember()
				    << makeKeyValueReference("hash", fi.hash)
				    << makeKeyValueReference("name", fi.fname)
				    << makeKeyValueReference("source", sourceName)
				    << makeKeyValueReference("size", fileSize)
				    << makeKeyValueReference("transferred", completed)
				    << makeKeyValueReference("is_anonymous", isAnon)
				    << makeKeyValueReference("is_encrypted_e2e", isEncryptedE2E)
				    << makeKeyValueReference("transfer_rate", dlspeed)
				    << makeKeyValueReference("status", status);
			}
		}
	}
	resp.setOk();
}

void TransfersHandler::handleSetFileDestinationDirectory( Request& req,
                                                          Response& resp )
{
	mStateTokenServer->replaceToken(mStateToken);

	std::string hashString;
	std::string newPath;
	req.mStream << makeKeyValueReference("path", newPath);
	req.mStream << makeKeyValueReference("hash", hashString);
	RsFileHash hash(hashString);

	if (mFiles->setDestinationDirectory(hash, newPath)) resp.setOk();
	else resp.setFail();
}

void TransfersHandler::handleSetFileDestinationName( Request& req,
                                                     Response& resp )
{
	mStateTokenServer->replaceToken(mStateToken);

	std::string hashString;
	std::string newName;
	req.mStream << makeKeyValueReference("name", newName);
	req.mStream << makeKeyValueReference("hash", hashString);
	RsFileHash hash(hashString);

	if (mFiles->setDestinationName(hash, newName)) resp.setOk();
	else resp.setFail();
}

void TransfersHandler::handleSetFileChunkStrategy(Request& req, Response& resp)
{
	mStateTokenServer->replaceToken(mStateToken);

	std::string hashString;
	std::string newChunkStrategyStr;
	req.mStream << makeKeyValueReference("chuck_stategy", newChunkStrategyStr);
	req.mStream << makeKeyValueReference("hash", hashString);

	RsFileHash hash(hashString);
	FileChunksInfo::ChunkStrategy newStrategy =
	        FileChunksInfo::CHUNK_STRATEGY_PROGRESSIVE;

	if ( newChunkStrategyStr == "streaming" )
		newStrategy = FileChunksInfo::CHUNK_STRATEGY_STREAMING;
	else if ( newChunkStrategyStr == "random" )
		newStrategy = FileChunksInfo::CHUNK_STRATEGY_RANDOM;
	else if ( newChunkStrategyStr == "progressive" )
		newStrategy = FileChunksInfo::CHUNK_STRATEGY_PROGRESSIVE;

	if (mFiles->setChunkStrategy(hash, newStrategy)) resp.setOk();
	else resp.setFail();
}

} // namespace resource_api

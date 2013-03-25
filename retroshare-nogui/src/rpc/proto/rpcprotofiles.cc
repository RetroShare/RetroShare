/*
 * RetroShare External Interface.
 *
 * Copyright 2012-2012 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */

#include "rpc/proto/rpcprotofiles.h"
#include "rpc/proto/gencc/files.pb.h"

#include <retroshare/rsfiles.h>
#include <retroshare/rspeers.h>

#include "util/rsstring.h"
#include "util/rsdir.h"

#include <stdio.h>

#include <iostream>
#include <algorithm>

#include <set>


bool fill_file_from_details(rsctrl::core::File *file, DirDetails &details);
bool fill_file_as_dir(rsctrl::core::File *file, const std::string &dir_name);

RpcProtoFiles::RpcProtoFiles(uint32_t serviceId)
	:RpcQueueService(serviceId)
{ 
	return; 
}

int RpcProtoFiles::processMsg(uint32_t chan_id, uint32_t msg_id, uint32_t req_id, const std::string &msg)
{
	/* check the msgId */
	uint8_t topbyte = getRpcMsgIdExtension(msg_id); 
	uint16_t service = getRpcMsgIdService(msg_id); 
	uint8_t submsg  = getRpcMsgIdSubMsg(msg_id);
	bool isResponse = isRpcMsgIdResponse(msg_id);


	std::cerr << "RpcProtoFiles::processMsg() topbyte: " << (int32_t) topbyte;
	std::cerr << " service: " << (int32_t) service << " submsg: " << (int32_t) submsg;
	std::cerr << std::endl;

	if (isResponse)
	{
		std::cerr << "RpcProtoFiles::processMsg() isResponse() - not processing";
		std::cerr << std::endl;
		return 0;
	}

	if (topbyte != (uint8_t) rsctrl::core::CORE)
	{
		std::cerr << "RpcProtoFiles::processMsg() Extension Mismatch - not processing";
		std::cerr << std::endl;
		return 0;
	}

	if (service != (uint16_t) rsctrl::core::FILES)
	{
		std::cerr << "RpcProtoFiles::processMsg() Service Mismatch - not processing";
		std::cerr << std::endl;
		return 0;
	}

	if (!rsctrl::files::RequestMsgIds_IsValid(submsg))
	{
		std::cerr << "RpcProtoFiles::processMsg() SubMsg Mismatch - not processing";
		std::cerr << std::endl;
		return 0;
	}

	switch(submsg)
	{
		case rsctrl::files::MsgId_RequestTransferList:
			processReqTransferList(chan_id, msg_id, req_id, msg);
			break;

		case rsctrl::files::MsgId_RequestControlDownload:
			processReqControlDownload(chan_id, msg_id, req_id, msg);
			break;

		case rsctrl::files::MsgId_RequestShareDirList:
			processReqShareDirList(chan_id, msg_id, req_id, msg);
			break;

		default:
			std::cerr << "RpcProtoFiles::processMsg() ERROR should never get here";
			std::cerr << std::endl;
			return 0;
	}

	/* must have matched id to get here */
	return 1;
}



int RpcProtoFiles::processReqTransferList(uint32_t chan_id, uint32_t /* msg_id */, uint32_t req_id, const std::string &msg)
{
	std::cerr << "RpcProtoFiles::processReqTransferList()";
	std::cerr << std::endl;

	// parse msg.
	rsctrl::files::RequestTransferList req;
	if (!req.ParseFromString(msg))
	{
		std::cerr << "RpcProtoFiles::processReqTransferList() ERROR ParseFromString()";
		std::cerr << std::endl;
		return 0;
	}

	// response.
	rsctrl::files::ResponseTransferList resp;
	bool success = true;
	std::string errorMsg;

	std::list<std::string> file_list;
	FileSearchFlags hints(0);

	/* convert msg parameters into local ones */
	switch(req.direction())
	{
		case rsctrl::files::DIRECTION_UPLOAD:
		{
			rsFiles->FileUploads(file_list);
			hints = RS_FILE_HINTS_UPLOAD;
			break;
		}
		case rsctrl::files::DIRECTION_DOWNLOAD:
		{
			rsFiles->FileDownloads(file_list);
			hints = RS_FILE_HINTS_DOWNLOAD;
			break;
		}
		default:
			std::cerr << "RpcProtoFiles::processReqTransferList() ERROR Unknown Dir";
			std::cerr << std::endl;
			success = false;
			errorMsg = "Unknown Direction";
		break;
	}

	std::list<std::string>::iterator lit;
	for(lit = file_list.begin(); lit != file_list.end(); lit++)
	{
		rsctrl::files::FileTransfer *transfer = resp.add_transfers();
		transfer->set_direction(req.direction());
	
		FileInfo info;
		if (!rsFiles->FileDetails(*lit, hints, info))	
		{
			/* error */
			continue;
		}

		/* copy file details */
		rsctrl::core::File *filedetails = transfer->mutable_file();
		filedetails->set_hash(info.hash);
		filedetails->set_size(info.size);
		filedetails->set_name(info.fname);

		transfer->set_fraction( (float) info.transfered / info.size );
		transfer->set_rate_kbs( info.tfRate );
	}

	/* DONE - Generate Reply */
        if (success)
	{
		rsctrl::core::Status *status = resp.mutable_status();
		status->set_code(rsctrl::core::Status::SUCCESS);
	}
	else
	{
		rsctrl::core::Status *status = resp.mutable_status();
		status->set_code(rsctrl::core::Status::FAILED);
		status->set_msg(errorMsg);
	}

	std::string outmsg;
	if (!resp.SerializeToString(&outmsg))
	{
		std::cerr << "RpcProtoFiles::processReqTransferList() ERROR SerialiseToString()";
		std::cerr << std::endl;
		return 0;
	}
	
	// Correctly Name Message.
	uint32_t out_msg_id = constructMsgId(rsctrl::core::CORE, rsctrl::core::FILES, 
				rsctrl::files::MsgId_ResponseTransferList, true);

	// queue it.
	queueResponse(chan_id, out_msg_id, req_id, outmsg);

	return 1;
}

int RpcProtoFiles::processReqControlDownload(uint32_t chan_id, uint32_t /* msg_id */, uint32_t req_id, const std::string &msg)
{
	std::cerr << "RpcProtoFiles::processReqControlDownload()";
	std::cerr << std::endl;

	// parse msg.
	rsctrl::files::RequestControlDownload req;
	if (!req.ParseFromString(msg))
	{
		std::cerr << "RpcProtoFiles::processReqControlDownload() ERROR ParseFromString()";
		std::cerr << std::endl;
		return 0;
	}

	// response.
	rsctrl::files::ResponseControlDownload resp;
	bool success = true;
	std::string errorMsg;

	std::string filehash = req.file().hash();
	switch(req.action())
	{
		case rsctrl::files::RequestControlDownload::ACTION_START:
		{
			std::list<std::string> srcIds;
	
			std::string filename = req.file().name();
			uint64_t filesize = req.file().size();

			// We Set NETWORK_WIDE flag here -> as files will be found via search.
			// If this changes, we might be adjust flag (or pass it in!)
			if (!rsFiles -> FileRequest(filename, filehash, filesize, 
						"", RS_FILE_REQ_ANONYMOUS_ROUTING, srcIds))
			{
				success = false;
				errorMsg = "FileRequest ERROR";
			}
			break;
		}
		case rsctrl::files::RequestControlDownload::ACTION_CONTINUE:
		{
 			if (!rsFiles->changeQueuePosition(filehash,QUEUE_TOP))
			{
				success = false;
				errorMsg = "File QueuePosition(Top) ERROR";
			}
			break;
		}
		case rsctrl::files::RequestControlDownload::ACTION_WAIT:
		{
 			if (!rsFiles->changeQueuePosition(filehash,QUEUE_BOTTOM))
			{
				success = false;
				errorMsg = "File QueuePosition(Bottom) ERROR";

			}
			break;
		}
		case rsctrl::files::RequestControlDownload::ACTION_PAUSE:
		{
 			if (!rsFiles->FileControl(filehash,RS_FILE_CTRL_PAUSE))
			{
				success = false;
				errorMsg = "FileControl(Pause) ERROR";

			}
			break;
		}
		case rsctrl::files::RequestControlDownload::ACTION_RESTART:
		{
 			if (!rsFiles->FileControl(filehash,RS_FILE_CTRL_START))
			{
				success = false;
				errorMsg = "FileControl(Start) ERROR";

			}
			break;
		}
		case rsctrl::files::RequestControlDownload::ACTION_CHECK:
		{
 			if (!rsFiles->FileControl(filehash,RS_FILE_CTRL_FORCE_CHECK))
			{
				success = false;
				errorMsg = "FileControl(Check) ERROR";

			}
			break;
		}
		case rsctrl::files::RequestControlDownload::ACTION_CANCEL:
		{
			if (!rsFiles->FileCancel(filehash))
			{
				success = false;
				errorMsg = "FileCancel ERROR";
			}
			break;
		}
		default:
			success = false;
			errorMsg = "Invalid Action";
			break;
	}


	/* DONE - Generate Reply */
        if (success)
	{
		rsctrl::core::Status *status = resp.mutable_status();
		status->set_code(rsctrl::core::Status::SUCCESS);
	}
	else
	{
		rsctrl::core::Status *status = resp.mutable_status();
		status->set_code(rsctrl::core::Status::FAILED);
		status->set_msg(errorMsg);
	}

	std::string outmsg;
	if (!resp.SerializeToString(&outmsg))
	{
		std::cerr << "RpcProtoFiles::processReqControlDownload() ERROR SerialiseToString()";
		std::cerr << std::endl;
		return 0;
	}
	
	// Correctly Name Message.
	uint32_t out_msg_id = constructMsgId(rsctrl::core::CORE, rsctrl::core::FILES, 
				rsctrl::files::MsgId_ResponseControlDownload, true);

	// queue it.
	queueResponse(chan_id, out_msg_id, req_id, outmsg);

	return 1;
}


int RpcProtoFiles::processReqShareDirList(uint32_t chan_id, uint32_t /* msg_id */, uint32_t req_id, const std::string &msg)
{
	std::cerr << "RpcProtoFiles::processReqShareDirList()";
	std::cerr << std::endl;

	// parse msg.
	rsctrl::files::RequestShareDirList req;
	if (!req.ParseFromString(msg))
	{
		std::cerr << "RpcProtoFiles::processReqShareDirList() ERROR ParseFromString()";
		std::cerr << std::endl;
		return 0;
	}

	// response.
	rsctrl::files::ResponseShareDirList resp;
	bool success = true;
	std::string errorMsg;


	std::string uid = req.ssl_id();
	std::string path = req.path();
	DirDetails details;

	if (uid.empty())
	{
		uid = rsPeers->getOwnId();
	}

	std::cerr << "RpcProtoFiles::processReqShareDirList() For uid: " << uid << " & path: " << path;
	std::cerr << std::endl;

	if (path.empty())
	{
		/* we have to do a nasty hack to get anything useful.
		 * we do a ref=NULL to get the pointers to People, 
		 * then use the correct one to get root directories
		 */
		std::cerr << "RpcProtoFiles::processReqShareDirList() Hack to get root Dirs!";
		std::cerr << std::endl;

		FileSearchFlags flags;
		if (uid == rsPeers->getOwnId())
		{
			flags |= RS_FILE_HINTS_LOCAL;
		}

		DirDetails root_details;
		if (!rsFiles->RequestDirDetails(NULL, root_details, flags))
		{
			std::cerr << "RpcProtoFiles::processReqShareDirList() ref=NULL Hack failed";
			std::cerr << std::endl;
			success = false;
			errorMsg = "Root Directory Request Failed.";
		}
		else
		{
			void *person_ref = NULL;
			std::list<DirStub>::iterator sit;
			for(sit = root_details.children.begin(); sit != root_details.children.end(); sit++)
			{
				//std::cerr << "RpcProtoFiles::processReqShareDirList() Root.child->name : " << sit->name;
				if (sit->name == uid)
				{
					person_ref = sit->ref;
					break;
				}
			}

			if (!person_ref)
			{
				std::cerr << "RpcProtoFiles::processReqShareDirList() Person match failed";
				std::cerr << std::endl;
				success = false;
				errorMsg = "Missing Person Root Directory.";
			}
			else
			{
				// Doing the REAL request!
				if (!rsFiles->RequestDirDetails(person_ref, details, flags))
				{
					std::cerr << "RpcProtoFiles::processReqShareDirList() Personal Shared Dir Hack failed";
					std::cerr << std::endl;
					success = false;
					errorMsg = "Missing Person Shared Directories";
				}
			}
		}

	}
	else
	{
		// Path must begin with / for proper matching.
		if (path[0] != '/')
		{
			path = '/' + path;
		}

		if (!rsFiles->RequestDirDetails(uid, path, details))
		{
			std::cerr << "RpcProtoFiles::processReqShareDirList() ERROR Unknown Dir";
			std::cerr << std::endl;
			success = false;
			errorMsg = "Directory Request Failed.";
		}
	}





	if (success)
	{
		// setup basics of response.
		resp.set_ssl_id(uid);
		resp.set_path(path);

		switch(details.type)
		{
			case DIR_TYPE_ROOT:
			{
				std::cerr << "RpcProtoFiles::processReqShareDirList() Details.type == ROOT ??";
				std::cerr << std::endl;
				resp.set_list_type(rsctrl::files::ResponseShareDirList::DIRQUERY_ROOT);
				rsctrl::core::File *file = resp.add_files();
				fill_file_as_dir(file, details.name);

			}
			break;

			case DIR_TYPE_FILE:
			{
				std::cerr << "RpcProtoFiles::processReqShareDirList() Details.type == FILE";
				std::cerr << std::endl;
				resp.set_list_type(rsctrl::files::ResponseShareDirList::DIRQUERY_FILE);
				rsctrl::core::File *file = resp.add_files();
				fill_file_from_details(file, details);
			}
			break;

			default:
				std::cerr << "RpcProtoFiles::processReqShareDirList() Details.type == UNKNOWN => default to DIR";
				std::cerr << std::endl;

			case DIR_TYPE_PERSON:
			case DIR_TYPE_DIR:
			{
				std::cerr << "RpcProtoFiles::processReqShareDirList() Details.type == DIR or PERSON";
				std::cerr << std::endl;

				resp.set_list_type(rsctrl::files::ResponseShareDirList::DIRQUERY_DIR);

	        		//std::string dir_path = RsDirUtil::makePath(details.path, details.name);		
	        		std::string dir_path = details.path;

				std::cerr << "RpcProtoFiles::processReqShareDirList() details.path: " << details.path;
				std::cerr << " details.name: " << details.name;
				std::cerr << std::endl;

				std::list<DirStub>::iterator sit;
				for(sit = details.children.begin(); sit != details.children.end(); sit++)
				{
					std::cerr << "RpcProtoFiles::processReqShareDirList() checking child: " << sit->name;
					std::cerr << std::endl;
					if (sit->type == DIR_TYPE_FILE)
					{
						std::cerr << "RpcProtoFiles::processReqShareDirList() is FILE, fetching details.";
						std::cerr << std::endl;

						DirDetails child_details;
	               				std::string child_path = RsDirUtil::makePath(dir_path, sit->name);
						if (rsFiles->RequestDirDetails(uid, child_path, child_details))
						{
							rsctrl::core::File *file = resp.add_files();
							fill_file_from_details(file, child_details);
						}
						else
						{
							std::cerr << "RpcProtoFiles::processReqShareDirList() RequestDirDetails(" << child_path << ") Failed!!!";
							std::cerr << std::endl;
						}
					}
					else
					{
						std::cerr << "RpcProtoFiles::processReqShareDirList() is DIR";
						std::cerr << std::endl;

						rsctrl::core::File *file = resp.add_files();
						fill_file_as_dir(file, sit->name);
					}
				}
			}
			break;
		}
	}

	/* DONE - Generate Reply */
        if (success)
	{
		rsctrl::core::Status *status = resp.mutable_status();
		status->set_code(rsctrl::core::Status::SUCCESS);
	}
	else
	{
		rsctrl::core::Status *status = resp.mutable_status();
		status->set_code(rsctrl::core::Status::FAILED);
		status->set_msg(errorMsg);
	}

	std::string outmsg;
	if (!resp.SerializeToString(&outmsg))
	{
		std::cerr << "RpcProtoFiles::processReqTransferList() ERROR SerialiseToString()";
		std::cerr << std::endl;
		return 0;
	}
	
	// Correctly Name Message.
	uint32_t out_msg_id = constructMsgId(rsctrl::core::CORE, rsctrl::core::FILES, 
				rsctrl::files::MsgId_ResponseShareDirList, true);

	// queue it.
	queueResponse(chan_id, out_msg_id, req_id, outmsg);

	return 1;
}


/***** HELPER FUNCTIONS *****/


bool fill_file_from_details(rsctrl::core::File *file, DirDetails &details)
{
	file->set_hash(details.hash);	
	file->set_name(details.name);	
	file->set_size(details.count);

	return true;
}


bool fill_file_as_dir(rsctrl::core::File *file, const std::string &dir_name)
{
	file->set_hash("");
	file->set_name(dir_name);
	file->set_size(0);

	return true;
}


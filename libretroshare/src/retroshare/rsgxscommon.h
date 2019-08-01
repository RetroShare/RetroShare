/*******************************************************************************
 * libretroshare/src/retroshare: rsgxscommon.h                                 *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2012  Robert Fernie <retroshare@lunamutt.com>                 *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
#pragma once

#include <cstdint>
#include <string>
#include <list>

#include "rsgxsifacetypes.h"
#include "serialiser/rsserializable.h"

struct RsGxsFile : RsSerializable
{
	RsGxsFile();
	std::string mName;
	RsFileHash mHash;
	uint64_t    mSize;

	/// @see RsSerializable
	virtual void serial_process( RsGenericSerializer::SerializeJob j,
	                             RsGenericSerializer::SerializeContext& ctx )
	{
		RS_SERIAL_PROCESS(mName);
		RS_SERIAL_PROCESS(mHash);
		RS_SERIAL_PROCESS(mSize);
	}
};

struct RsGxsImage  : RsSerializable
{
	RsGxsImage();
	~RsGxsImage();

	/// Use copy constructor and duplicate memory.
	RsGxsImage(const RsGxsImage& a);

	RsGxsImage &operator=(const RsGxsImage &a); // Need this as well?

	/** NB: Must make sure that we always use methods - to be consistent about
	 * malloc/free for this data. */
	static uint8_t *allocate(uint32_t size);
	static void release(void *data);

	void take(uint8_t *data, uint32_t size); // Copies Pointer.
	void copy(uint8_t *data, uint32_t size); // Allocates and Copies.
	void clear(); 				// Frees.
	void shallowClear(); 			// Clears Pointer.

	uint32_t mSize;
	uint8_t* mData;

	/// @see RsSerializable
	virtual void serial_process( RsGenericSerializer::SerializeJob j,
	                             RsGenericSerializer::SerializeContext& ctx )
	{
		RsTypeSerializer::TlvMemBlock_proxy b(mData, mSize);
		RsTypeSerializer::serial_process(j, ctx, b, "mData");
	}
};

enum class RsGxsVoteType : uint32_t
{
	NONE = 0, /// Used to detect unset vote?
	DOWN = 1, /// Negative vote
	UP = 2    /// Positive vote
};


// Status Flags to indicate Voting....
// All Services that use the Comment service must not Use This space.
namespace GXS_SERV {
	/* Msg Vote Status */
	static const uint32_t GXS_MSG_STATUS_GXSCOMMENT_MASK  = 0x000f0000;
	static const uint32_t GXS_MSG_STATUS_VOTE_MASK        = 0x00030000;

	static const uint32_t GXS_MSG_STATUS_VOTE_UP          = 0x00010000;
	static const uint32_t GXS_MSG_STATUS_VOTE_DOWN        = 0x00020000;
}




struct RsGxsVote : RsSerializable
{
	RsGxsVote();
	RsMsgMetaData mMeta;
	uint32_t mVoteType;

	/// @see RsSerializable
	virtual void serial_process( RsGenericSerializer::SerializeJob j,
	                             RsGenericSerializer::SerializeContext& ctx )
	{
		RS_SERIAL_PROCESS(mMeta);
		RS_SERIAL_PROCESS(mVoteType);
	}
};

struct RsGxsComment : RsSerializable
{
	RsGxsComment();
	RsMsgMetaData mMeta;
	std::string mComment;

	// below is calculated.
	uint32_t mUpVotes;
	uint32_t mDownVotes;
	double   mScore; 

	uint32_t mOwnVote;

	// This is filled in if detailed Comment Data is called.
	std::list<RsGxsVote> mVotes;

	/// @see RsSerializable
	virtual void serial_process( RsGenericSerializer::SerializeJob j,
	                             RsGenericSerializer::SerializeContext& ctx )
	{
		RS_SERIAL_PROCESS(mMeta);
		RS_SERIAL_PROCESS(mComment);
		RS_SERIAL_PROCESS(mUpVotes);
		RS_SERIAL_PROCESS(mDownVotes);
		RS_SERIAL_PROCESS(mScore);
		RS_SERIAL_PROCESS(mOwnVote);
		RS_SERIAL_PROCESS(mVotes);
	}
};


struct RsGxsCommentService
{
	RsGxsCommentService() {}
	virtual ~RsGxsCommentService() {}

	/** Get previously requested comment data with token */
	virtual bool getCommentData( uint32_t token,
	                             std::vector<RsGxsComment> &comments ) = 0;
	virtual bool getRelatedComments( uint32_t token,
	                                 std::vector<RsGxsComment> &comments ) = 0;

	virtual bool createNewComment(uint32_t &token, RsGxsComment &comment) = 0;
	virtual bool createNewVote(uint32_t &token, RsGxsVote &vote) = 0;

	virtual bool acknowledgeComment(
	        uint32_t token,
	        std::pair<RsGxsGroupId, RsGxsMessageId>& msgId ) = 0;

	virtual bool acknowledgeVote(
	        uint32_t token,
	        std::pair<RsGxsGroupId, RsGxsMessageId>& msgId ) = 0;
};

/// @deprecated use RsGxsVoteType::NONE instead @see RsGxsVoteType
#define GXS_VOTE_NONE 0x0000

/// @deprecated use RsGxsVoteType::DOWN instead @see RsGxsVoteType
#define GXS_VOTE_DOWN 0x0001

/// @deprecated use RsGxsVoteType::UP instead @see RsGxsVoteType
#define GXS_VOTE_UP	0x0002

/*
 * libretroshare/src/ft/ ftfilecreator.h
 *
 * File Transfer for RetroShare.
 *
 * Copyright 2008 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2 as published by the Free Software Foundation.
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

#ifndef FT_FILE_CREATOR_HEADER
#define FT_FILE_CREATOR_HEADER

/* 
 * ftFileCreator
 *
 * TODO: Serialiser Load / Save.
 *
 */

class ftFileCreator: public ftFileProvider
{
public:

	ftFileCreator(std::string savepath, uint64_t size, std::string hash);
	~ftFileCreator();

	/* overloaded from FileProvider */
virtual bool 	getFileData(uint64_t offset, uint32_t chunk_size, void *data);

	/* creation functions for FileCreator */
bool	getMissingChunk(uint64_t &offset, uint32_t &chunk);
bool 	addFileData(uint64_t offset, uint32_t chunk_size, void *data);

private:

	/* TO BE DECIDED */
};


#endif // FT_FILE_PROVIDER_HEADER

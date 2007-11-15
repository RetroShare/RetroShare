/*
 * "$Id: pqichannel.h,v 1.3 2007-02-18 21:46:49 rmf24 Exp $"
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2004-2006 by Robert Fernie.
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



#ifndef MRK_PQI_CHANNELITEM_H
#define MRK_PQI_CHANNELITEM_H

#include "pqi/pqitunnel.h"

#define PQI_TUNNEL_CHANNEL_ITEM_TYPE 162

PQTunnel *createChannelItems(void *d, int n);
		
class PQChanItem: public PQTunnel
{
	protected:
	//PQChanItem(int st);
	public:
	PQChanItem();
	~PQChanItem();
virtual PQChanItem *clone();
void	copy(const PQChanItem *di);
void	clear();
virtual std::ostream &print(std::ostream &out);

	// Overloaded from PQTunnel.
virtual const int getSize() const;
virtual int out(void *data, const int size) const;
virtual int in(const void *data, const int size); 


	// So what data do we need.
	// data load.
	//
	// 1) a Message.
	// 2) a list of filenames/hashs/sizes.
	//
	// So you can different types
	// of channels.
	//
	// 1) Clear Text Signed. 
	// -> name
	// -> public key.
	// -> data load
	// -> signature. (of all)
	//
	// 2) Encrypted.
	// -> name
	// -> public key
	// -> encrypted data load.
	// -> signature (of all)
	// 
	
	class FileItem
	{
		public:
		std::string name;
		std::string hash;
		long 	    size;

		void	clear();
		std::ostream &print(std::ostream &out);

		const int getSize() const;
		int out(void *data, const int size) const;
		int in(const void *data, const int size); 
	};
	
	typedef std::list<FileItem> FileList;

	// Certificate/Key (containing Name)
	unsigned char *certDER;
	int certType;
	int certLen;

	std::string msg;
	std::string title;
	FileList    files;

	unsigned char *signature;
	int signType;
	int signLen;

	bool signValid; // not transmitted.
};


#endif // MRK_PQI_CHANNELITEM_H

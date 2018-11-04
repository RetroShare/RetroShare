/*******************************************************************************
 * libretroshare/src/pqi: pqibin.h                                             *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2004-2006 by Robert Fernie <retroshare@lunamutt.com>              *
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
#ifndef PQI_BIN_INTERFACE_HEADER
#define PQI_BIN_INTERFACE_HEADER


#include "pqi/pqi_base.h"
#include "pqi/pqihash.h"
#include "util/rstime.h"

#include <stdio.h>


//! handles writing to binary files
/*!
 * This allows for handling of binary file within Rs, In particular writing
 * binary data to file declared in constructor
 */
class BinFileInterface: public BinInterface
{
public:
	BinFileInterface(const char *fname, int flags);
virtual ~BinFileInterface();

virtual int     tick() { return 1; }

virtual int	senddata(void *data, int len);
virtual int	readdata(void *data, int len);
virtual int	netstatus() { return 1;}
virtual int	isactive()  { return (buf != NULL);}
virtual bool	moretoread(uint32_t /* usec */ ) 
	{ 
        if ((buf) && (bin_flags & BIN_FLAGS_READABLE))
		{
			if ((size - ftell(buf)) > 0)
			{
				return true;
			}
		}
		return false;
	}

virtual int	close();
virtual bool 	cansend(uint32_t /* usec */) 
	{ 
        return (bin_flags & BIN_FLAGS_WRITEABLE);
	}
virtual bool    bandwidthLimited() { return false; }

//! if HASHing is switched on
virtual RsFileHash gethash();
virtual uint64_t bytecount();

	virtual uint64_t getFileSize();

private:
	int   bin_flags;
	FILE *buf;
	uint64_t size;
	pqihash *hash;
	uint64_t bcount;
};

/*!
 * use this for writing encrypted data to file using user's (ownid) ssl key
 * hash for encrypted data is calculated, not the unencrypted data
 */
class BinEncryptedFileInterface : public BinFileInterface
{
public:

	BinEncryptedFileInterface(const char *fname, int flags);
	virtual ~BinEncryptedFileInterface();

	/*!
	 * pls note if hashing is on, it is the hash of the encrypted data that is calculated
	 * also note, sif data is sent more than once, in a single instance of this object, then you will need to store the
	 * encrypted file size for each send externally as they are encrypted with different random keys.
	 * @param data data to be sent
	 * @param len length of data in bytes
	 * @return -1 if failed to write data, if not number bytes sent to file is returned
	 */
	int	senddata(void *data, int len);

	/*!
	 * uses the hash of the encrypted data
	 * @param location to place data
	 * @param the length of data to be read
	 * @return -1 if failed to write data, if not number of bytes read is returned
	 */
	int	readdata(void *data, int len);

	/*!
	 * this releases resources held by an instance
	 */
	int	close();

	uint64_t bytecount();
	bool	moretoread(uint32_t usec);

private:

	char* data;
	bool haveData;
	uint64_t sizeData;
	uint64_t cpyCount;

};

//! handles writing to reading/writing to memory
/*!
 * This provide a memory interface for storing/retrieving information in memory
 * to be read/sent elsewhere
 */
class BinMemInterface: public BinInterface
{
public:
	BinMemInterface(int defsize, int flags);
	BinMemInterface(const void *data, const int size, int flags);
virtual ~BinMemInterface();

	/* Extra Interfaces */
int	fseek(int loc);
int	memsize() { return recvsize; }
void   *memptr() { return buf; }

	/* file interface */
bool	writetofile(const char *fname);
bool	readfromfile(const char *fname);

virtual int     tick() { return 1; }

virtual int	senddata(void *data, int len);
virtual int	readdata(void *data, int len);
virtual int	netstatus() { return 1; }
virtual int	isactive()  { return 1; }
virtual bool	moretoread(uint32_t /* usec */) 
	{ 
        if ((buf) && (bin_flags & BIN_FLAGS_READABLE ))
		{
			if (readloc < recvsize)
			{
				return true;
			}
		}
		return false;
	}

virtual int	close();
virtual bool 	cansend(uint32_t /* usec */)    
	{ 
        return (bin_flags & BIN_FLAGS_WRITEABLE);
	}
virtual bool    bandwidthLimited() { return false; }

virtual RsFileHash gethash();
virtual uint64_t bytecount();

	private:
	int   bin_flags;
	void *buf;
	int   size;
	int   recvsize;
	int   readloc;
	pqihash *hash;
	uint64_t bcount;
};


class NetBinDummy: public NetBinInterface
{
public:
	NetBinDummy(PQInterface *parent, const RsPeerId& id, uint32_t t);
virtual ~NetBinDummy() { return; }

	// Net Interface
virtual int connect(const struct sockaddr_storage &raddr);
virtual int listen();
virtual int stoplistening();
virtual int disconnect();
virtual int reset();
virtual bool connect_parameter(uint32_t type, uint32_t value) 
	{ 
		(void) type; /* suppress unused parameter warning */
		(void) value; /* suppress unused parameter warning */
		return false; 
	}
virtual int getConnectAddress(struct sockaddr_storage &raddr) 
	{
		(void) raddr; /* suppress unused parameter warning */
		return 0;
	}

	// Bin Interface.
virtual int     tick();

virtual int     senddata(void *data, int len);
virtual int     readdata(void *data, int len);
virtual int     netstatus();
virtual int     isactive();
virtual bool    moretoread(uint32_t usec);
virtual bool    cansend(uint32_t usec);
virtual int	close();

virtual RsFileHash gethash();

	private:
	uint32_t type;
	bool 	 dummyConnected;
        bool     toConnect;
	uint32_t connectDelta;
	rstime_t   connectTS;
};


#endif // PQI_BINARY_INTERFACES_HEADER


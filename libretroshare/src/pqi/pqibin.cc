/*******************************************************************************
 * libretroshare/src/pqi: pqibin.cc                                            *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2004-2006  Robert Fernie                                      *
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
#include "pqi/pqibin.h"
#include "pqi/authssl.h"
#include "util/rsnet.h"
#include "util/rsdir.h"
#include "util/rsmemory.h"

// #define DEBUG_PQIBIN

BinFileInterface::BinFileInterface(const char *fname, int flags)
	:bin_flags(flags), buf(NULL), hash(NULL), bcount(0)
{
	/* if read + write - open r+ */
	if ((bin_flags & BIN_FLAGS_READABLE) &&
		(bin_flags & BIN_FLAGS_WRITEABLE))
	{
		buf = RsDirUtil::rs_fopen(fname, "rb+");
		/* if the file don't exist */
		if (!buf)
		{
			buf = RsDirUtil::rs_fopen(fname, "wb+");
		}

	}
	else if (bin_flags & BIN_FLAGS_READABLE)
	{
		buf = RsDirUtil::rs_fopen(fname, "rb");
	}
	else if (bin_flags & BIN_FLAGS_WRITEABLE)
	{
		// This is enough to remove old file in Linux...
		// but not in windows.... (what to do)
		buf = RsDirUtil::rs_fopen(fname, "wb");
		fflush(buf); /* this might help windows! */
	}
	else
	{	
		/* not read or write! */
	}

	if (buf)
	{
#ifdef DEBUG_PQIBIN
		std::cerr << "BinFileInterface: " << (void*)this << ": openned file " << fname << std::endl;
#endif
		/* no problem */

		/* go to the end */
		fseek(buf, 0L, SEEK_END);
		/* get size */
		size = ftell(buf);
		/* back to the start */
		fseek(buf, 0L, SEEK_SET);
	}
	else
	{
		size = 0;
	}

	if (bin_flags & BIN_FLAGS_HASH_DATA)
	{
		hash = new pqihash();
	}
}


BinFileInterface::~BinFileInterface()
{
	if (buf)
	{
		fclose(buf);
#ifdef DEBUG_PQIBIN
		std::cerr << "BinFileInterface: " << (void*)this << ": closed file " << std::endl;
#endif
	}

	if (hash)
	{
		delete hash;
	}
}

int BinFileInterface::close()
{
	if (buf)
	{
		fclose(buf);
#ifdef DISTRIB_DEBUG
		std::cerr << "BinFileInterface: " << (void*)this << ": closed file " << std::endl;
#endif
		buf = NULL;
	}
	return 1;
}
		
int	BinFileInterface::senddata(void *data, int len)
{
	if (!buf)
		return -1;

	if (1 != fwrite(data, len, 1, buf))
	{
		return -1;
	}
	if (bin_flags & BIN_FLAGS_HASH_DATA)
	{  
		hash->addData(data, len);
		bcount += len;
	}
	return len;
}

int	BinFileInterface::readdata(void *data, int len)
{
	if (!buf)
		return -1;

	if (1 != fread(data, len, 1, buf))
	{
		return -1;
	}
	if (bin_flags & BIN_FLAGS_HASH_DATA)
	{  
		hash->addData(data, len);
		bcount += len;
	}
	return len;
}

RsFileHash  BinFileInterface::gethash()
{
	RsFileHash hashstr;
	if (bin_flags & BIN_FLAGS_HASH_DATA)
	{  
		hash->Complete(hashstr);
	}
	return hashstr;
}

uint64_t BinFileInterface::bytecount()
{
	if (bin_flags & BIN_FLAGS_HASH_DATA)
	{  
		return bcount;
	}
	return 0;
}

uint64_t BinFileInterface::getFileSize()
{
	return size;
}



BinEncryptedFileInterface::BinEncryptedFileInterface(const char* fname, int flags)
	: BinFileInterface(fname, flags), data(NULL), haveData(false), sizeData(0), cpyCount(0)
{
}

BinEncryptedFileInterface::~BinEncryptedFileInterface()
{
	if((sizeData > 0) && data != NULL)
	{
		free(data);
	}

}

int BinEncryptedFileInterface::senddata(void* data, int len)
{

	void* encrytedData = NULL;
	int encDataLen = 0;

	// encrypt using own ssl public key
	if(len > 0)
		AuthSSL::getAuthSSL()->encrypt((void*&)encrytedData, encDataLen, data, len, AuthSSL::getAuthSSL()->OwnId());
	else
		return -1;


	if((encDataLen > 0) && (encrytedData != NULL))
	{
		BinFileInterface::senddata((unsigned char *)encrytedData, encDataLen);
		free(encrytedData);
	}
	else
	{
		return -1;
	}

	return len;
}


int BinEncryptedFileInterface::readdata(void* data, int len)
{
	// to respect the inherited behavior of BinInterface
	// the whole file is read and decryped and store to be read by subsequent calls
	char* encryptedData = NULL;
	int encrypDataLen = 0;


	if(!haveData) // read whole data for first call, or first call after close()
	{

                uint64_t encrypDataLen64 = BinFileInterface::getFileSize();
                
                if(encrypDataLen64 > uint64_t(~(int)0))
                {
                    std::cerr << __PRETTY_FUNCTION__ << ": cannot decrypt files of size > " << ~(int)0 << std::endl;
                    return -1 ;
                }
		encrypDataLen = (int)encrypDataLen64 ;
		encryptedData = new char[encrypDataLen];

		// make sure assign was successful
		if(encryptedData == NULL)
			return -1;


		if(-1 == BinFileInterface::readdata(encryptedData, encrypDataLen))
		{
			delete[] encryptedData;
			return -1;
		}

		if((encrypDataLen > 0) && (encryptedData != NULL))
		{
            			int sizeDataInt = 0 ;
                        
				if(!AuthSSL::getAuthSSL()->decrypt((void*&)(this->data), sizeDataInt, encryptedData, encrypDataLen))
				{
					delete[] encryptedData;
					return -1;
				}
                
                		sizeData = sizeDataInt ;
				haveData = true;
				delete[] encryptedData;
		}


		if((uint64_t)len <= sizeData)
		{
			memcpy(data, this->data, len);
			cpyCount += len;
		}
		else
		{
			std::cerr << "BinEncryptedFileInterface::readData(): Error, Asking for more data than present" << std::endl;
			return -1;
		}
	}
	else
	{

		if((cpyCount + len) <= (uint64_t)sizeData)
		{
			memcpy(data, (void *) ((this->data) + cpyCount), len);
			cpyCount += len;
		}
		else
		{
			std::cerr << "BinEncryptedFileInterface::readData(): Error, Asking for more data than present" << std::endl;
			return -1;
		}
	}

	return len;
}

int BinEncryptedFileInterface::close()
{
	if(data != NULL)
	{
		free(data);
		sizeData = 0;
		haveData = false;
		cpyCount = 0;
	}

	return BinFileInterface::close();
}

uint64_t BinEncryptedFileInterface::bytecount()
{
	return cpyCount;
}

bool BinEncryptedFileInterface::moretoread(uint32_t /* usec */)
{
	if(haveData)
		return (cpyCount < (uint64_t)sizeData);
	else
		return cpyCount < (uint64_t)getFileSize();
}

BinMemInterface::BinMemInterface(int defsize, int flags)
	:bin_flags(flags), buf(NULL), size(defsize), 
	recvsize(0), readloc(0), hash(NULL), bcount(0)
	{
		buf = rs_malloc(defsize);
        
        	if(buf == NULL)
            {
                close() ;
                return ;
            }
		if (bin_flags & BIN_FLAGS_HASH_DATA)
		{
			hash = new pqihash();
		}
	}


BinMemInterface::BinMemInterface(const void *data, const int defsize, int flags)
	:bin_flags(flags), buf(NULL), size(defsize), 
	recvsize(0), readloc(0), hash(NULL), bcount(0)
	{
		buf = rs_malloc(defsize);
        
        	if(buf == NULL)
            {
                close() ;
                return ;
            }
		if (bin_flags & BIN_FLAGS_HASH_DATA)
		{
			hash = new pqihash();
		}

		/* just remove the const 
		 * *BAD* but senddata don't change it anyway 
		 */
		senddata((void *) data, defsize);
	}

BinMemInterface::~BinMemInterface() 
{ 
	if (buf)
		free(buf);

	if (hash)
		delete hash;
	return; 
}

int BinMemInterface::close()
{
	if (buf)
	{
		free(buf);
		buf = NULL;
	}
	size = 0;
	recvsize = 0;
	readloc = 0;
	return 1;
}

	/* some fns to mess with the memory */
int	BinMemInterface::fseek(int loc)
	{
		if (loc <= recvsize)
		{
			readloc = loc;
			return 1;
		}
		return 0;
	}


int	BinMemInterface::senddata(void *data, const int len)
	{
		if(recvsize + len > size)
		{
			/* resize? */
			return -1;
		}
		memcpy((char *) buf + recvsize, data, len);
		if (bin_flags & BIN_FLAGS_HASH_DATA)
		{  
			hash->addData(data, len);
			bcount += len;
		}
		recvsize += len;
		return len;
	}

		
int	BinMemInterface::readdata(void *data, int len)
	{
		if(readloc + len > recvsize)
		{
			/* no more stuff? */
			return -1;
		}
		memcpy(data, (char *) buf + readloc, len);
		if (bin_flags & BIN_FLAGS_HASH_DATA)
		{  
			hash->addData(data, len);
			bcount += len;
		}
		readloc += len;
		return len;
	}



RsFileHash  BinMemInterface::gethash()
	{
		RsFileHash hashstr;
		if (bin_flags & BIN_FLAGS_HASH_DATA)
		{  
			hash->Complete(hashstr);
		}
		return hashstr;
	}


uint64_t BinMemInterface::bytecount()
{
	if (bin_flags & BIN_FLAGS_HASH_DATA)
	{  
		return bcount;
	}
	return 0;
}
	
bool    BinMemInterface::writetofile(const char *fname)
{
	FILE *fd = RsDirUtil::rs_fopen(fname, "wb");
	if (!fd)
	{
		return false;
	}

	if (1 != fwrite(buf, recvsize, 1, fd))
	{
		fclose(fd);
		return false;
	}

	fclose(fd);

	return true;
}

bool    BinMemInterface::readfromfile(const char *fname)
{
	FILE *fd = RsDirUtil::rs_fopen(fname, "rb");
	if (!fd)
	{
		return false;
	}

	/* get size */
	::fseek(fd, 0L, SEEK_END);
	int fsize = ftell(fd);

	if (fsize > size)
	{
		/* not enough room */
		std::cerr << "BinMemInterface::readfromfile() not enough room";
		std::cerr << std::endl;

		fclose(fd);
		return false;
	}

	::fseek(fd, 0L, SEEK_SET);
	if (1 != fread(buf, fsize, 1, fd))
	{
		/* not enough room */
		std::cerr << "BinMemInterface::readfromfile() failed fread";
		std::cerr << std::endl;

		fclose(fd);
		return false;
	}

	recvsize = fsize;
	readloc = 0;
	fclose(fd);

	return true;
}


/**************************************************************************/


void printNetBinID(std::ostream &out, const RsPeerId& id, uint32_t t)
{
	out << "NetBinId(" << id << ",";
	if (t == PQI_CONNECT_TCP)
	{
		out << "TCP)";
	}
	else if (t & (PQI_CONNECT_HIDDEN_TOR_TCP | PQI_CONNECT_HIDDEN_I2P_TCP))
	{
		out << "HTCP";
	}
	else
	{
		out << "UDP)";
	}
}


/************************** NetBinDummy ******************************
 * A test framework, 
 *
 */

#include "pqi/pqiperson.h"

const uint32_t DEFAULT_DUMMY_DELTA 	= 5;

NetBinDummy::NetBinDummy(PQInterface *parent, const RsPeerId& id, uint32_t t)
       :NetBinInterface(parent, id), type(t), dummyConnected(false), toConnect(false), connectDelta(DEFAULT_DUMMY_DELTA)
{ 
	return; 
}

	// Net Interface
int NetBinDummy::connect(const struct sockaddr_storage &raddr)
{
	std::cerr << "NetBinDummy::connect(";
	std::cerr << sockaddr_storage_tostring(raddr);
	std::cerr << ") ";
	printNetBinID(std::cerr, PeerId(), type);
	std::cerr << std::endl; 

	std::cerr << "NetBinDummy::dummyConnect = true!";
	std::cerr << std::endl; 

	if (type == PQI_CONNECT_TCP)
	{
		std::cerr << "NetBinDummy:: Not connecting TCP";
		std::cerr << std::endl; 
		if (parent())
		{
			//struct sockaddr_storage addr = raddr;
			parent()->notifyEvent(this, CONNECT_FAILED, raddr);
		}
	}
	else if (!dummyConnected)
	{
		toConnect = true;
		connectTS = time(NULL) + connectDelta;
		std::cerr << "NetBinDummy::toConnect = true, connect in: ";
		std::cerr << connectDelta << " secs";
		std::cerr << std::endl; 
	}
	else
	{
		std::cerr << "NetBinDummy:: Already Connected!";
		std::cerr << std::endl; 
	}

	return 1;
}

int NetBinDummy::listen()
{
	std::cerr << "NetBinDummy::connect() ";
	printNetBinID(std::cerr, PeerId(), type);
	std::cerr << std::endl;

	return 1;
}

int NetBinDummy::stoplistening()
{
	std::cerr << "NetBinDummy::stoplistening() ";
	printNetBinID(std::cerr, PeerId(), type);
	std::cerr << std::endl;

	return 1;
}

int NetBinDummy::disconnect()
{
	std::cerr << "NetBinDummy::disconnect() ";
	printNetBinID(std::cerr, PeerId(), type);
	std::cerr << std::endl;

	std::cerr << "NetBinDummy::dummyConnect = false!";
	std::cerr << std::endl; 
	dummyConnected = false;

	if (parent())
	{
		struct sockaddr_storage addr;
		sockaddr_storage_clear(addr);
		
		parent()->notifyEvent(this, CONNECT_FAILED, addr);
	}

	return 1;
}

int NetBinDummy::reset()
{
	std::cerr << "NetBinDummy::reset() ";
	printNetBinID(std::cerr, PeerId(), type);
	std::cerr << std::endl;

	disconnect();

	return 1;
}


         // Bin Interface.
int     NetBinDummy::tick()
{

	if (toConnect)
	{
		if (connectTS < time(NULL))
		{
			std::cerr << "NetBinDummy::tick() dummyConnected = true ";
			printNetBinID(std::cerr, PeerId(), type);
			std::cerr << std::endl;
			dummyConnected = true;
			toConnect = false;
			if (parent())
			{
				struct sockaddr_storage addr;
				sockaddr_storage_clear(addr);

				parent()->notifyEvent(this, CONNECT_SUCCESS, addr);
			}
		}
		else
		{
			std::cerr << "NetBinDummy::tick() toConnect ";
			printNetBinID(std::cerr, PeerId(), type);
			std::cerr << std::endl;
		}
	}
	return 0;
}

int     NetBinDummy::senddata(void */*data*/, int len)
{
	std::cerr << "NetBinDummy::senddata() ";
	printNetBinID(std::cerr, PeerId(), type);
	std::cerr << std::endl;

	if (dummyConnected)
		return len;
	return 0;
}

int     NetBinDummy::readdata(void */*data*/, int /*len*/)
{
	std::cerr << "NetBinDummy::readdata() ";
	printNetBinID(std::cerr, PeerId(), type);
	std::cerr << std::endl;

	return 0;
}

int     NetBinDummy::netstatus()
{
	std::cerr << "NetBinDummy::netstatus() ";
	printNetBinID(std::cerr, PeerId(), type);
	std::cerr << std::endl;

	return 1;
}

int     NetBinDummy::isactive()
{
	std::cerr << "NetBinDummy::isactive() ";
	printNetBinID(std::cerr, PeerId(), type);
	if (dummyConnected)
		std::cerr << " true ";
	else
		std::cerr << " false ";
	std::cerr << std::endl;

	return dummyConnected;
}

bool    NetBinDummy::moretoread(uint32_t /* usec */)
{
	std::cerr << "NetBinDummy::moretoread() ";
	printNetBinID(std::cerr, PeerId(), type);
	std::cerr << std::endl;

	return false;
}

bool    NetBinDummy::cansend(uint32_t /* usec */)
{
	std::cerr << "NetBinDummy::cansend() ";
	printNetBinID(std::cerr, PeerId(), type);
	std::cerr << std::endl;

	return dummyConnected;
}

int	NetBinDummy::close()
{
	std::cerr << "NetBinDummy::close() ";
	printNetBinID(std::cerr, PeerId(), type);
	std::cerr << std::endl;

	return 1;
}

RsFileHash NetBinDummy::gethash()
{
	std::cerr << "NetBinDummy::gethash() ";
	printNetBinID(std::cerr, PeerId(), type);
	std::cerr << std::endl;

	return RsFileHash();
}


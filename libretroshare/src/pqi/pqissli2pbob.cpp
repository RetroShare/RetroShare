#include "pqissli2pbob.h"

bool pqissli2pbob::connect_parameter(uint32_t type, const std::string &value)
{
	if (type == NET_PARAM_CONNECT_DOMAIN_ADDRESS)
	{
		RS_STACK_MUTEX(mSslMtx);
		// a new line must be appended!
		mI2pAddr = value + '\n';
		return true;
	}

	return pqissl::connect_parameter(type, value);
}

int pqissli2pbob::Basic_Connection_Complete()
{
	int ret;

	if ((ret = pqissl::Basic_Connection_Complete()) != 1)
	{
		// basic connection not complete.
		return ret;
	}

	// send addr. (new line is already appended)
	ret = send(sockfd, mI2pAddr.c_str(), mI2pAddr.length(), 0);
	if (ret != (int)mI2pAddr.length())
		return -1;
	return 1;
}

#pragma once

#include <iostream>
#include <list>

#include <retroshare/rsids.h>
#include <pqi/p3netmgr.h>

class FakeNetMgr: public p3NetMgrIMPL
{
	public:
		FakeNetMgr()
		: p3NetMgrIMPL()
		{
			return;
		}
	private:
		//RsPeerId mOwnId;
};



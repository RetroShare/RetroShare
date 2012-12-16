
#include <iostream>
#include "menu/menu.h"

class MenuTest
{
public:
	MenuTest(MenuInterface *i, std::istream &in, std::ostream &out)
	:mMenus(i), mIn(in), mOut(out)
	{
		return;
	}

int	tick()
	{
		int c = mIn.get();
		uint8_t key = (uint8_t) c;
		uint32_t drawFlags = 0;
		std::string buffer;
		mMenus->process(key, drawFlags, buffer);

		return 1;
	}

private:


	MenuInterface *mMenus;
	std::istream &mIn;
	std::ostream &mOut;
};

#include <unistd.h>
#include <fcntl.h>

#include "rstermserver.h"

class RsConsole
{
	public:

	RsConsole(RsTermServer *s, int infd, int outfd)
	:mServer(s), mIn(infd), mOut(outfd)
	{
		const int fcflags = fcntl(mIn,F_GETFL);
		if (fcflags < 0) 
		{ 
			std::cerr << "RsConsole() ERROR getting fcntl FLAGS";
			std::cerr << std::endl;
			exit(1);
		}
		if (fcntl(mIn,F_SETFL,fcflags | O_NONBLOCK) <0) 
		{ 
			std::cerr << "RsConsole() ERROR setting fcntl FLAGS";
			std::cerr << std::endl;
			exit(1);
		}
	}

	int tick()
	{
		char buf;
		std::string output;
		int size = read(mIn, &buf, 1);
	
		bool haveInput = (size > 0);

		int rt = mServer->tick(haveInput, buf, output);

		if (output.size() > 0)
		{
			write(mOut, output.c_str(), output.size());
		}

		if (rt < 0)
		{
			std::cerr << "Exit Request";
			exit(1);
		}

		if (!haveInput)
		{
			return 0;
		}

		return 1;
	}
	
private:

	RsTermServer *mServer;
	int mIn, mOut;
};


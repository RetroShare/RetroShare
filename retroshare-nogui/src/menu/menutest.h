
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
		mMenus->process(key);

		return 1;
	}

private:


	MenuInterface *mMenus;
	std::istream &mIn;
	std::ostream &mOut;
};




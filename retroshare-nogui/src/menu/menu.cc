
#include "menu/menu.h"
#include <retroshare/rsconfig.h>
#include <retroshare/rspeers.h>
#include <retroshare/rsiface.h>

#include <iostream>

/**********************************************************
 * Menu Base Interface.
 */

int tailrec_printparents(Menu *m, std::ostream &out)
{
	Menu *p = m->parent();
	if (p)
	{
		tailrec_printparents(p, out);
	}
	out << m->ShortFnDesc() << " => ";

#if 0
	MenuList *ml = dynamic_cast<MenuList *>(m);
	MenuOpBasicKey *mbk = dynamic_cast<MenuOpBasicKey *>(m);
	MenuOpTwoKeys *mtk = dynamic_cast<MenuOpTwoKeys *>(m);

	if (ml)
	{
		out << "MenuList@" << (void *) m << " ";
	}
	else if (mbk)
	{
		out << "MenuBasicKey@" << (void *) m << " ";
	}
	else if (mtk)
	{
		out << "MenuOpTwoKeys@" << (void *) m << " ";
	}
	else
	{
		out << "Menu@" << (void *) m << " ";
	}
#endif

	return 1;
}



uint32_t MenuInterface::process(char key)
{

#ifdef MENU_DEBUG
	std::cout << "MenuInterface::process(" << key << ")";
	std::cout << std::endl;
#endif // MENU_DEBUG

	/* call process on current menu */
	uint32_t rt = mCurrentMenu->process(key);
	bool doRedraw = true;
	bool showError = false;
	bool showHelp = false;

#ifdef MENU_DEBUG
	std::cout << "MenuInterface::process() currentMenu says: " << rt;
	std::cout << std::endl;
#endif // MENU_DEBUG

	switch(rt)
	{
		case MENU_PROCESS_NONE:
		case MENU_PROCESS_DONE:
			/* no changes - operation performed, or noop */
			break;

		case MENU_PROCESS_NEEDDATA:
			/* no redraw, this could be called many times */
			doRedraw = false;
			break;
			
		case MENU_PROCESS_ERROR:
			/* Show Error at top of Page */
			showError = true;
			break;
			
		case MENU_PROCESS_HELP:
			/* Show Help at top of Page */
			showHelp = true;
			break;
			
		case MENU_PROCESS_MENU:
			/* new menu to switch to */
			if (mCurrentMenu->selectedMenu())
			{
				std::cout << "MenuInterface::process() Switching Menus";
				std::cout << std::endl;
				mCurrentMenu = mCurrentMenu->selectedMenu();
			}
			else
			{
				std::cout << "MenuInterface::process() ERROR";
				std::cout << " SelectedMenu == NULL";
				std::cout << std::endl;
			}
			break;

		case MENU_PROCESS_TOP:
			/* switch to Base Menu */
			mCurrentMenu = mBase;
			break;

		case MENU_PROCESS_QUIT:
			return MENU_PROCESS_QUIT;
			break;
	}

	/* now we redraw, and wait for next data */
	if (!doRedraw)
	{
		return MENU_PROCESS_NEEDDATA;
	}

	/* HEADER */
	for(int i = 0; i < 20; i++)
	{
		std::cout << std::endl;
	}

	/* ERROR */
	if (showError)
	{
		mCurrentMenu->showError();
	}
	/* HELP */
	else if (showHelp)
	{
		mCurrentMenu->showHelp();
	}

	/* MENU PAGE */		
	drawHeader();
	mCurrentMenu->drawPage();
	return MENU_PROCESS_NEEDDATA;
}

uint32_t MenuInterface::drawHeader()
{
	std::cout << "=======================================================";
	std::cout << std::endl;
	std::cout << "Retroshare Terminal Menu V2.xxxx ======================";
	std::cout << std::endl;

	unsigned int nTotal = 0;
	unsigned int nConnected = 0;
	rsPeers->getPeerCount(&nTotal, &nConnected, false);

	uint32_t netState = rsConfig->getNetState();
	std::string natState("Unknown");
	switch(netState)
	{
		default:
		case RSNET_NETSTATE_BAD_UNKNOWN:
			natState = "YELLOW:Unknown";
			break;

		case RSNET_NETSTATE_BAD_OFFLINE:
			natState = "GRAY:Offline";
			break;

		case RSNET_NETSTATE_BAD_NATSYM:
			natState = "RED:Nasty Firewall";
			break;

		case RSNET_NETSTATE_BAD_NODHT_NAT:
			natState = "RED:NoDht & Firewalled";
			break;

		case RSNET_NETSTATE_WARNING_RESTART:
			natState = "YELLOW:Restarting";
			break;

		case RSNET_NETSTATE_WARNING_NATTED:
			natState = "YELLOW:Firewalled";
			break;

		case RSNET_NETSTATE_WARNING_NODHT:
			natState = "YELLOW:DHT Disabled";
			break;

		case RSNET_NETSTATE_GOOD:
			natState = "GREEN:Good!";
			break;

		case RSNET_NETSTATE_ADV_FORWARD:
			natState = "GREEN:Forwarded Port";
			break;
	}

	float downKb = 0;
	float upKb = 0;
	rsicontrol -> ConfigGetDataRates(downKb, upKb);

	std::cout << "Friends " << nConnected << "/" << nTotal;
	std::cout << " Network: " << natState;
	std::cout << std::endl;
	std::cout << "Down: " << downKb << " (kB/s) ";
	std::cout << " Up: " << upKb << " (kB/s) ";
	std::cout << std::endl;
	std::cout << "Menu State: ";
	tailrec_printparents(mCurrentMenu, std::cout);
	std::cout << std::endl;

	std::cout << "=======================================================";
	std::cout << std::endl;

	return 1;
}

/**********************************************************
 * Menu (Base)
 */

Menu::~Menu()
{
	/* cleanup children */
	mParent = NULL;
	mSelectedMenu = NULL;
        std::map<uint8_t, Menu *>::iterator it;

	for(it = mChildren.begin(); it != mChildren.end(); it++)
	{
		delete (it->second);
	}

	mChildren.clear();
}

int Menu::addMenuItem(char key, Menu *child)
{
        std::map<uint8_t, Menu *>::iterator it;
	it = mChildren.find(key);
	if (it != mChildren.end())
	{
		std::cout << "Menu::addMenuItem() ERROR DUPLICATE MENU ITEM";
		std::cout << std::endl;
		return 0;
	}
	mChildren[key] = child;
	child->setParent(this);

	return 1;
}


uint32_t Menu::process(char key)
{
	/* try standard list ones */
	uint32_t rt = std_process(key);
	if (rt)
	{
		return rt;
	}

	/* now try children */
	rt = process_children(key);
	if (rt)
	{
		return rt;
	}

	return MENU_PROCESS_NONE;
}

uint32_t Menu::std_process(char key)
{
	switch(key)
	{
		case MENU_KEY_QUIT:
			return MENU_PROCESS_QUIT;
			break;
		case MENU_KEY_HELP:
			return MENU_PROCESS_HELP;
			break;
		case MENU_KEY_TOP:
			return MENU_PROCESS_TOP;
			break;
		case MENU_KEY_UP:
			setSelectedMenu(parent());
			return MENU_PROCESS_MENU;
			break;
	}

	return MENU_PROCESS_NONE;
}


uint32_t Menu::process_children(char key)
{
	std::map<uint8_t, Menu *>::iterator it;
	it = mChildren.find(key);

	if (it == mChildren.end())
	{
		return MENU_PROCESS_NONE;
	}

	/* have a child */

	/* now return, depending on type */
	switch(it->second->op())
	{
		/* Think I can handle these the same! Both will call DrawPage, 
		 * then Process on New Menu 
		 */

		case MENU_OP_NEEDDATA:
		case MENU_OP_SUBMENU:
			setSelectedMenu(it->second);
			return MENU_PROCESS_MENU;
			break;

		default:
		case MENU_OP_INSTANT:
			/* done already! */
			setSelectedMenu(NULL);
			return MENU_PROCESS_DONE;
			break;

		case MENU_OP_ERROR:
			/* done already! */
			setSelectedMenu(NULL);
			return MENU_PROCESS_ERROR;
			break;
	}
}


uint32_t Menu::drawPage()
{
	std::cout << "Universal Commands ( ";
	std::cout << (char) MENU_KEY_QUIT << ":Quit ";
	std::cout << (char) MENU_KEY_HELP << ":Help ";
	std::cout << (char) MENU_KEY_TOP << ":Top ";
	std::cout << (char) MENU_KEY_UP << ":Up ";
	std::cout << ")";
	std::cout << std::endl;

	std::cout << "Specific Commands (";
	std::map<uint8_t, Menu *>::iterator it;
	for(it = mChildren.begin(); it != mChildren.end(); it++)
	{
		std::cout << (char) it->first << ":";
		std::cout << it->second->ShortFnDesc() << " ";
	}
	std::cout << ")";
	std::cout << std::endl;

	return 1;
}


uint32_t Menu::drawHelpPage()
{
	std::cout << "Menu Help: Universal Commands are:";
	std::cout << std::endl;
	std::cout << "\tKey: " << (char) MENU_KEY_QUIT << " => Quit";
	std::cout << std::endl;
	std::cout << "\tKey: " << (char) MENU_KEY_HELP << " => Help";
	std::cout << std::endl;
	std::cout << "\tKey: " << (char) MENU_KEY_TOP << " => Top Menu";
	std::cout << std::endl;
	std::cout << "\tKey: " << (char) MENU_KEY_UP << " => Up a Menu";
	std::cout << std::endl;

	std::cout << "Specific Commands are:";
	std::cout << std::endl;
	std::map<uint8_t, Menu *>::iterator it;
	for(it = mChildren.begin(); it != mChildren.end(); it++)
	{
		std::cout << "\tKey: " << (char) it->first << " => ";
		std::cout << it->second->ShortFnDesc();
		std::cout << std::endl;
	}

	return 1;
}


/**********************************************************
 * Menu List (Base)
 */


uint32_t MenuList::drawPage()
{
	Menu::drawPage();

	std::cout << "Navigation Commands (";
	//std::cout << (char) MENULIST_KEY_LIST << ":List ";
	std::cout << (char) MENULIST_KEY_NEXT << ":Next ";
	std::cout << (char) MENULIST_KEY_PREV << ":Prev ";
	std::cout << ")";
	std::cout << std::endl;

	std::cout << "MenuList::Internals ";
	std::cout << "ListSize: " << getListCount();
	std::cout << " SelectIdx: " << mSelectIdx;
	std::cout << " Cursor: " << mCursor;
	std::cout << std::endl;

	int i = 0;

	int listCount = getListCount();
	int startCount = mCursor + 1;
	int endCount = mCursor + 10;
	if (endCount > listCount)
	{
		endCount = listCount;
	}

	if (mSelectIdx >= 0)	
	{
		std::cout << "Current Selection Idx: " << mSelectIdx << " : ";
		std::string desc;
		if (getEntryDesc(mSelectIdx, desc) & (desc != ""))
		{
			std::cout << desc;
		}
		else
		{
			std::cout << "Missing Description";
		}
		std::cout << std::endl;
	}
	else
	{
		std::cout << "No Current Selection: Use 0 - 9 to choose an Entry";
		std::cout << std::endl;
	}


	std::cout << "Showing " << startCount;
	std::cout << " to " << endCount << " of " << listCount << " Entries";
	std::cout << std::endl;

	std::list<std::string>::iterator it;
	for (it = mList.begin(); it != mList.end(); it++, i++)
	{
		int curIdx = i - mCursor;
		if ((curIdx >= 0) && (curIdx < 10))
		{
			if (i == mSelectIdx)
			{
				std::cout << "SELECTED => (" << curIdx << ")  ";
			}
			else
			{
				std::cout << "\t(" << curIdx << ")  ";
			}
			std::string desc;
			if (getEntryDesc(i, desc) & (desc != ""))
			{
				std::cout << desc;
			}
			else
			{
				std::cout << *it << " => ";
				std::cout << "Missing Description";
			}
			std::cout << std::endl;
		}
	}

	return 1;
}

uint32_t MenuList::drawHelpPage()
{
	Menu::drawPage();

	std::cout << "MenuList Help: Navigation Commands are:";
	std::cout << std::endl;
	//std::cout << "\tKey: " << (char) MENULIST_KEY_LIST << " => List";
	//std::cout << std::endl;
	std::cout << "\tKey: " << (char) MENULIST_KEY_NEXT << " => Next Page";
	std::cout << std::endl;
	std::cout << "\tKey: " << (char) MENULIST_KEY_PREV << " => Prev Page";
	std::cout << std::endl;

	std::cout << "MenuList::drawPage() Internal Details";
	std::cout << std::endl;
	std::cout << "List Size: " << getListCount();
	std::cout << std::endl;
	std::cout << "SelectIdx: " << mSelectIdx;
	std::cout << std::endl;
	std::cout << "Cursor: " << mCursor;
	std::cout << std::endl;
	return 1;
}


uint32_t MenuList::op()
{
	/* load friend list*/
	mList.clear();
	//rsPeers->getGpgAcceptedList(mList);
	mSelectIdx = -1;
	mCursor = 0;
	
	return MENU_OP_ERROR; // SUBMENU -> only for inherited classes;
}

uint32_t MenuList::getListCount()
{
	return mList.size();
}

int MenuList::getCurrentKey(std::string &key)
{
	return getListEntry(mSelectIdx, key);
}
	
int MenuList::getCurrentIdx(int &idx)
{
	idx = mSelectIdx;
	return 1;
}
	
int MenuList::getListEntry(int idx, std::string &key)
{
	if (idx < 0)
	{
		return MENU_ENTRY_NONE;
	}
	
	std::list<std::string>::iterator it;
	int i = 0;
	for (it = mList.begin(); (i < idx) && (it != mList.end()); it++, i++) ;

	if (it != mList.end())
	{
		key = *it;
		return MENU_ENTRY_OKAY;
	}
	return MENU_ENTRY_NONE;
}

int MenuList::getEntryDesc(int idx, std::string &desc)
{
	desc = "Entry Description";
	return MENU_ENTRY_OKAY;
}

uint32_t MenuList::process(char key)
{
	/* try standard list ones */
	uint32_t rt = Menu::process(key);
	if (rt)
	{
		return rt;
	}

	rt = list_process(key);
	return rt;
}

uint32_t MenuList::list_process(char key)
{
	if (((key >= '0') && (key <= '9')) ||
	    ((key >= 'a') && (key <= 'f'))) 
	{
		int idx = 0;
		/* select index */
		if ((key >= '0') && (key <= '9'))
		{
			idx = key - '0';
		}
		else
		{
			idx = key - 'a' + 9;
		}

		/* now change selection, dependent on pagination */	
		if (mCursor + idx < getListCount())
		{
			mSelectIdx = mCursor + idx;
			std::cout << "MenuList::list_process() Selected Idx: " << mSelectIdx;
			std::cout << std::endl;
		}
		else
		{
			std::cout << "MenuList::list_process() Idx Out of Range";
			std::cout << std::endl;
		}

		return MENU_PROCESS_DONE; /* ready for next key stroke */
	}


	switch(key)
	{
		case MENULIST_KEY_LIST:
			/* send a list to output */

			return MENU_PROCESS_DONE; /* ready for next key stroke */
			break;
		case MENULIST_KEY_NEXT:
			/* shift to next page */
			if (mCursor + 10 < getListCount())
			{
				mCursor += 10;
			}

			return MENU_PROCESS_DONE;
			break;
		case MENULIST_KEY_PREV:
			/* shift to prev page */
			if (((int) mCursor) - 10 >= 0)
			{
				mCursor -= 10;
			}

			return MENU_PROCESS_DONE;
			break;
	}

	return MENU_PROCESS_NONE;
}


uint32_t MenuOpBasicKey::op_basic(std::string key)
{
	parent()->setErrorMessage("MenuOpBasicKey Not Overloaded Correctly");
	return MENU_OP_ERROR;
}


uint32_t MenuOpBasicKey::op()
{
	std::string key;
	Menu *Parent=parent();
	MenuList *p = dynamic_cast<MenuList *>(Parent);

	if (!p)
	{
		if (Parent)
		{	
			Parent->setErrorMessage("Invalid (Basic) Menu Structure");
		}
		return MENU_OP_ERROR;
	}

	if (p->getCurrentKey(key))
	{
		return op_basic(key);
	}

	if (Parent)
	{
		Parent->setErrorMessage("Invalid Current Keys");
	}
	return MENU_OP_ERROR;
}



uint32_t MenuOpTwoKeys::op_twokeys(std::string parentkey, std::string key)
{
	parent()->setErrorMessage("MenuOpTwoKeys Not Overloaded Correctly");
	return MENU_OP_ERROR;
}


uint32_t MenuOpTwoKeys::op()
{
	std::string parentkey;
	std::string key;
	Menu *Parent=parent();
	MenuList *p = dynamic_cast<MenuList *>(Parent);

	Menu *grandParent=parent()->parent();
	MenuList *gp = dynamic_cast<MenuList *>(grandParent);

	if ((!gp) || (!p))
	{
		if (Parent)
		{	
			Parent->setErrorMessage("Invalid (TwoKeys) Menu Structure");
		}
		return MENU_OP_ERROR;
	}

	if ((gp->getCurrentKey(parentkey)) && (p->getCurrentKey(key)))
	{
		return op_twokeys(parentkey, key);
	}

	if (Parent)
	{
		Parent->setErrorMessage("Invalid Current Keys");
	}
	return MENU_OP_ERROR;
}



/**********************************************************
 * Menu Op Line Input (Base)
 */


uint32_t MenuOpLineInput::op()
{
	return MENU_OP_NEEDDATA;
}


uint32_t MenuOpLineInput::process_lines(std::string input)
{
	std::cout << "MenuOpLineInput::process_lines() => SHOULD BE OVERLOADED";
	std::cout << "Input Was: ";
	std::cout << std::endl;
	std::cout << "==================================================";
	std::cout << std::endl;
	std::cout << input;
	std::cout << "==================================================";
	std::cout << std::endl;

	return MENU_PROCESS_ERROR;
}


uint32_t MenuOpLineInput::process(char key)
{
	/* read data in and add to buffer */
	mInput += key;
	if (key != '\n')
	{
		return MENU_PROCESS_NEEDDATA;
	}

	uint32_t rt = process_lines(mInput);

	switch(rt)
	{
		case MENU_PROCESS_NEEDDATA:
			break;
		case MENU_PROCESS_ERROR:
			std::cout << "MenuOpLineInput::process() => ERROR";
			std::cout << std::endl;
		case MENU_PROCESS_DONE:
			/* cleanup for next command */
			std::cout << "MenuOpLineInput::process() Clearing Buffer";
			std::cout << std::endl;
			mInput.clear();

			/* go back to parent menu */
			rt = MENU_PROCESS_MENU;
			setSelectedMenu(parent());
			break;
	}

	return rt;
}



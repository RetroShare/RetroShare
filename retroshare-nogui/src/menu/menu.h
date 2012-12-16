/*
 * RetroShare External Interface.
 *
 * Copyright 2012-2012 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
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



#ifndef RSNOGUI_MENU_H
#define RSNOGUI_MENU_H

#include <inttypes.h>

#include <string>
#include <map>
#include <list>

#include "rpcsystem.h" // generic processing command.

#define MENU_PROCESS_MASK	0x0fff 

#define MENU_PROCESS_NONE	0x0000
#define MENU_PROCESS_TOP	0x0001
#define MENU_PROCESS_MENU	0x0002
#define MENU_PROCESS_DONE	0x0004
#define MENU_PROCESS_QUIT	0x0008
#define MENU_PROCESS_SHUTDOWN	0x0010
#define MENU_PROCESS_HELP	0x0020
#define MENU_PROCESS_ERROR	0x0040

#define MENU_PROCESS_NEEDDATA	0x1000 		// Able to be OR'd with ANOTHER CASE.

#define MENU_KEY_QUIT	'Q'
#define MENU_KEY_HELP	'h'
#define MENU_KEY_TOP	't'
#define MENU_KEY_REPEAT	'r'
#define MENU_KEY_UP	'u'

#define MENULIST_KEY_LIST	'l'  // Don't need this.
#define MENULIST_KEY_NEXT	'n'
#define MENULIST_KEY_PREV	'p'

#define MENU_OP_ERROR		0
#define MENU_OP_INSTANT		1
#define MENU_OP_SUBMENU 	2
#define MENU_OP_NEEDDATA 	3

#define MENU_ENTRY_NONE		0
#define MENU_ENTRY_OKAY		1

#define MENU_DRAW_FLAGS_STD	0
#define MENU_DRAW_FLAGS_HTML	1
#define MENU_DRAW_FLAGS_ECHO	2
#define MENU_DRAW_FLAGS_NOQUIT  4

class Menu;
class Screen;

class Menu
{
public:
	Menu(std::string shortDesc): mParent(NULL), mShortDesc(shortDesc) { return; }
virtual ~Menu();

	void setParent(Menu *p) { mParent = p; }
	Menu *parent() { return mParent; }
	Menu *selectedMenu() { return mSelectedMenu; }
	int addMenuItem(char key, Menu *child);

virtual void     reset();
virtual uint32_t op() { return MENU_OP_SUBMENU; }  /* what type is it? returns SUBMENU, INSTANT, NEEDINPUT */
virtual uint32_t process(char key);

	// THE BIT STILL TO BE DEFINED!

std::string 	ShortFnDesc() { return mShortDesc; }// Menu Text (for Help).
virtual	uint32_t drawPage(uint32_t drawFlags, std::string &buffer); 
virtual	uint32_t drawHelpPage(uint32_t drawFlags, std::string &buffer); 

//virtual	std::string menuText() = 0;
//virtual	std::string menuHelp() = 0;
virtual void setOpMessage(std::string msg) { return; }
virtual void setErrorMessage(std::string msg) { return; }
virtual	uint32_t showError() { return 1; } //= 0;
virtual	uint32_t showHelp() { return 1; } //= 0;

protected:
virtual	uint32_t std_process(char key);		/* for H/T/R/Q */
virtual	uint32_t process_children(char key);    /* check for children ops */

	void setSelectedMenu(Menu *m) { mSelectedMenu = m; }

private:
	Menu *mParent;
	Menu *mSelectedMenu; 
	std::string mShortDesc;
	std::map<uint8_t, Menu *> mChildren;
};


/**********************************************************
 * Generic MenuList... provides a list of KEYS, which 
 * can be iterated through.
 *
 * Maintains: List + Current + Next / Previous.
 * Single Child, which is called for all.
 */

class MenuList: public Menu
{
public:
	MenuList(std::string shortDesc): Menu(shortDesc) { return; }

virtual void     reset();
virtual uint32_t op();
virtual	uint32_t process(char key);

	// List Info.
	uint32_t getListCount();
	int getCurrentIdx(int &idx);
	int getCurrentKey(std::string &key);
	int getListEntry(int idx, std::string &key);
virtual	int getEntryDesc(int idx, std::string &desc);

	// Output.
virtual	uint32_t drawPage(uint32_t drawFlags, std::string &buffer); 
virtual	uint32_t drawHelpPage(uint32_t drawFlags, std::string &buffer); 

protected:
	virtual uint32_t list_process(char key);

	int32_t mSelectIdx; /* -1 => none */
	uint32_t mCursor;   /* offset for list display */
	std::list<std::string> mList;

private:
};



class MenuOpBasicKey: public Menu
{
public:
	MenuOpBasicKey(std::string shortDesc): Menu(shortDesc) { return; }
	virtual uint32_t op();

protected:
	virtual uint32_t op_basic(std::string key);
};



class MenuOpTwoKeys: public Menu
{

public:
	MenuOpTwoKeys(std::string shortDesc): Menu(shortDesc) { return; }
	virtual uint32_t op();

protected:
	virtual uint32_t op_twokeys(std::string parentkey, std::string key);

};

/* Read input, line by line...
 */

class MenuOpLineInput: public Menu
{

public:
	MenuOpLineInput(std::string shortDesc): Menu(shortDesc) { return; }
	virtual uint32_t op();
	virtual uint32_t process(char key);

protected:
	virtual uint32_t process_lines(std::string input);

	std::string mInput;
};


#if 0
class MenuInterface: public RsTermServer
{
public:

	MenuInterface(Menu *b, uint32_t drawFlags) :mCurrentMenu(b), mBase(b), mDrawFlags(drawFlags), mInputRequired(false)  { return; }
	uint32_t process(char key, uint32_t drawFlags, std::string &buffer); 
	uint32_t drawHeader(uint32_t drawFlags, std::string &buffer); 

	// RsTermServer Interface.
        virtual void reset();
        virtual int tick(bool haveInput, char keypress, std::string &output);


private:
	Menu *mCurrentMenu;
	Menu *mBase;
	uint32_t mDrawFlags;
	bool mInputRequired;
};

#endif


class MenuInterface: public RpcSystem
{
public:

	MenuInterface(RpcComms *c, Menu *b, uint32_t drawFlags)
	:mComms(c), mCurrentMenu(b), mBase(b), mDrawFlags(drawFlags), mInputRequired(false)  { return; }

	uint32_t process(char key, uint32_t drawFlags, std::string &buffer); 
	uint32_t drawHeader(uint32_t drawFlags, std::string &buffer); 

	// RsSystem Interface.
        virtual void reset(uint32_t chan_id);
        virtual int tick();


private:
	RpcComms *mComms;
	Menu *mCurrentMenu;
	Menu *mBase;
	uint32_t mDrawFlags;
	bool mInputRequired;
	time_t mUpdateTime;
};


#endif

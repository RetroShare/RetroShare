#ifndef RSNOGUI_MENU_H
#define RSNOGUI_MENU_H

#include <inttypes.h>

#include <string>
#include <map>
#include <list>

#define MENU_PROCESS_NONE	0
#define MENU_PROCESS_TOP	1
#define MENU_PROCESS_MENU	2
#define MENU_PROCESS_NEEDDATA	3
#define MENU_PROCESS_DONE	4
#define MENU_PROCESS_QUIT	5
#define MENU_PROCESS_SHUTDOWN	6
#define MENU_PROCESS_HELP	7
#define MENU_PROCESS_ERROR	8

#define MENU_KEY_QUIT	'Q'
#define MENU_KEY_HELP	'H'
#define MENU_KEY_TOP	'T'
#define MENU_KEY_REPEAT	'R'
#define MENU_KEY_UP	'U'

#define MENULIST_KEY_LIST	'L'  // Don't need this.
#define MENULIST_KEY_NEXT	'N'
#define MENULIST_KEY_PREV	'P'

#define MENU_OP_ERROR		0
#define MENU_OP_INSTANT		1
#define MENU_OP_SUBMENU 	2
#define MENU_OP_NEEDDATA 	3

#define MENU_ENTRY_NONE		0
#define MENU_ENTRY_OKAY		1

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

virtual uint32_t op() { return MENU_OP_SUBMENU; }  /* what type is it? returns SUBMENU, INSTANT, NEEDINPUT */
virtual uint32_t process(char key);

	// THE BIT STILL TO BE DEFINED!

std::string 	ShortFnDesc() { return mShortDesc; }// Menu Text (for Help).

//virtual	std::string menuText() = 0;
//virtual	std::string menuHelp() = 0;
virtual void setOpMessage(std::string msg) { return; }
virtual void setErrorMessage(std::string msg) { return; }
virtual	uint32_t drawPage(); // { return 1; } //= 0;
virtual	uint32_t drawHelpPage(); // { return 1; } //= 0;
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

virtual uint32_t op();
virtual	uint32_t process(char key);

	// List Info.
	uint32_t getListCount();
	int getCurrentIdx(int &idx);
	int getCurrentKey(std::string &key);
	int getListEntry(int idx, std::string &key);
virtual	int getEntryDesc(int idx, std::string &desc);

	// Output.
virtual	uint32_t drawPage(); 
virtual	uint32_t drawHelpPage();

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



class MenuInterface
{
public:

	MenuInterface(Menu *b) :mCurrentMenu(b), mBase(b) { return; }
	uint32_t process(char key);
	uint32_t drawHeader();

private:
	Menu *mCurrentMenu;
	Menu *mBase;
};


#endif

/*
 * "$Id: Fl_Funky_Browser.h,v 1.4 2007-02-18 21:46:49 rmf24 Exp $"
 *
 * FltkGUI for RetroShare.
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


#ifndef RS_FL_FUNKY_BROWSER
#define RS_FL_FUNKY_BROWSER

/* My new funky browser.....
 *
 * - Designed to sort/display a tree brower 
 *   for search results....
 *
 *   First we need the basic interface class that
 *   must wrap the Data....
 */

#include <string>
#include <list>
#include <vector>

class DisplayData;

class DisplayItem
{
	public:
	int expander; 
	int index;
	int flags;
	std::string txt;
	DisplayData *ref;
};

class DisplayData
{
	public:
	DisplayData() { return;}
virtual ~DisplayData() { return;}

	// a couple of functions that do the work.
virtual	int ndix() = 0;  // Number of Indices.

std::string txt(int col, int width)
	{
		std::string out1 = txt(col);
		int i;
		for(i = (signed) out1.length() + 1; i < width; i++)
		{
			out1 += " ";
		}
		if ((signed) out1.length() > width)
		{
			std::string out2;
			for(i = 0; i < width; i++)
			{
				out2 += out1[i];
			}
			return out2;
		}
		return out1;
	}

virtual std::string txt(int col) = 0;

virtual int cmp(int col, DisplayData *) = 0;
virtual	int check(int n, int v = -1) { return -1;}  
		// return -1 unless have a check box. then return 0/1
		// if v != -1 attempt to set value.
};

#include <FL/Fl_Browser.H>

class Fl_Funky_Browser : public Fl_Browser
{
	public:
	Fl_Funky_Browser(int, int, int, int, const char *, int ncol);
	//Fl_Funky_Browser(int ncol);

	// add items.
int	selectDD(DisplayData *);
int	addItem(DisplayData *);
	// add in a batch (faster)
int	addItemSeries(DisplayData *);
int	ItemSeriesDone();

int	setTitle(int col, std::string name);
int	setCheck(int col); // enables check for the column;

DisplayData *removeItem(int idx = -1);
DisplayData *removeItem(int (*fn)(DisplayData *)); // remove first matching fn.
DisplayData *getSelected();
DisplayData *getCurrentItem();

int     checkSort(); // check if update affected order.
int     updateList(); // if affected call this.

int 	toggleCheckBox(int row, int col = -1);
int	getCheckState(int row = -1, int col = -1);

	// old - don't use
int 	current_SetCollapsed(bool col) { return 1;}

void	clear();

	// change browser config.
int	setup(std::string opts);
std::string setup();

	// Worker Functions.
int 	drawList();

	// Overload the Browser Functions.......
	protected:
virtual void item_draw(void* v, int X, int Y, int W, int H) const;
virtual int handle(int event);
	private:

int 	toggleCollapseLevel(int row);
int 	toggle_TreeSetting(int);
int 	toggle_ArrowSetting(int);
int	selectItems(int row);

int     cmp_upto(int lvl, DisplayItem *i1, DisplayItem *i2);
int     cmp(DisplayItem *i1, DisplayItem *i2);

	// Worker Functions.
int	checkIndices();

int	SortList();
int	RePopulate();

	// drag and ticks mouse stuff
	int handle_push(int x, int y);
	bool dragging();
	int handle_release(int x, int y);

	int ncols;
	std::list<DisplayItem *> dlist;
	std::vector<int> sort_order;
	std::vector<int> sort_direction;
	std::vector<int> tree;
	std::vector<int> display_order;
	std::vector<int> widths;
	std::vector<std::string> titles;
	std::vector<bool> check_box;

	int *fl_widths;
	int ntrees;

	int drag_mode;
	int drag_column; // which column
	int drag_x, drag_y;
	bool one_select;
};


#endif


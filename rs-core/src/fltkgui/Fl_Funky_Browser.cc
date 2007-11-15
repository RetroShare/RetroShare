/*
 * "$Id: Fl_Funky_Browser.cc,v 1.11 2007-02-18 21:46:49 rmf24 Exp $"
 *
 * FltkGUI for RetroShare.
 *
 * Copyright 2004-2006 by Robert Fernie.
 * NB: Parts of this code are derived from FLTK Fl_Browser code.
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

/* My new funky browser.....
 *
 * - Designed to sort/display a tree brower 
 *   for search results....
 *
 *   First we need the basic interface class that
 *   must wrap the Data....
 */

const static int FUNKY_EXPANDED = 0x001;
const static int FUNKY_VISIBLE  = 0x002;
const static int FUNKY_SELECTED = 0x004;

#include <sstream>
#include "pqi/pqidebug.h"
const int pqiflfbzone = 91393;

#include "fltkgui/Fl_Funky_Browser.h"
#include <FL/Fl_Browser.H>
#include <FL/fl_draw.H>
#include <FL/Fl.H>

#include <iostream>

	Fl_Funky_Browser::Fl_Funky_Browser(int a, int b, int c, 
				int d, const char *n, int ncol)
	:Fl_Browser(a,b,c,d,n), 
	ncols(ncol), sort_order(ncol), sort_direction(ncol), tree(ncol),
	display_order(ncol), widths(ncol), titles(ncol), check_box(ncol),
	drag_mode(0), one_select(true)
{
	// set the widths for the Columns onto Fl_Browser.
	fl_widths = (int *) malloc((ncol + 1) * sizeof(int));

	for(int i = 0; i < ncol; i++)
	{
		sort_order[i] = i;
		sort_direction[i] = 1;
		tree[i] = 0;
		display_order[i] = i;
		widths[i] = 150;
		fl_widths[i] = widths[i];
		titles[i] = "Unknown";
		check_box[i] = false;
	}
	// temp hack to make the first two columns expandable.
	if (ncol > 0)
		tree[0] = 1;
	if (ncol > 1)
		tree[1] = 1;
	ntrees = 2;

	// NOTE: It only makes sense to have the tree at the 
	// left hand side..... so all tree ticked ones 
	// should automatically be moved to left columns.

	fl_widths[ncol] = 0;
	column_widths(fl_widths);
	
	return;
}

int	Fl_Funky_Browser::addItemSeries(DisplayData *d)
{
	int idx = 1;
	int i;

	pqioutput(PQL_DEBUG_BASIC, pqiflfbzone,
		"Fl_Funky_Browser::addItem Properly()....");

	idx++;

	std::list<DisplayItem *>::iterator it;

	DisplayItem *last[ntrees];

	DisplayItem *ni = new DisplayItem();
	ni -> expander = -1;
	ni -> flags = FUNKY_EXPANDED | FUNKY_VISIBLE;
	ni -> ref = d;

	it = dlist.begin();
	if (it == dlist.end())
	{
		dlist.push_back(ni);
		pqioutput(PQL_DEBUG_BASIC, pqiflfbzone,
		  "Fl_Funky_Browser::addItem Empty List");

		RePopulate();
		return 1;
	}

	// add in all the initial levels.
	for(i = 0; (i < ntrees) && (it != dlist.end()); i++, it++)
	{
		last[i] = (*it);
	}

	if (it == dlist.end())
	{
		pqioutput(PQL_ALERT, pqiflfbzone,
		 "FL_Funky_Browser::Serious Accounting Condition!");

		SortList();
		RePopulate();
		return 1;
	}


	idx += ntrees;
	{
		std::ostringstream out;

		out << "Fl_Funky_Browser::addItem Inserting:";
		out << std::endl;

		for(i = 0; i < d -> ndix();i++)
		{
			out << "\t:" << d -> txt(i) << std::endl;
		}

		pqioutput(PQL_DEBUG_BASIC, pqiflfbzone, out.str());
	}

	for(; it != dlist.end(); it++, idx++)
	{
		std::ostringstream out;
		out << "Fl_Funky_Browser::addItem Checking Against:";
		out << std::endl;
		for(i = 0; i < (*it)->ref->ndix();i++)
		{
			out << "\t:" << (*it)->ref->txt(i);
			out << std::endl;
		}

		// cmp with item.
		int res = cmp(ni, *it); // if ni < *it
		out << "Cmp *ni vs *it: " << res << std::endl;

		if (0 > res) // if ni < *it
		{
			// if before it, stop and add item in.
			// check the level difference between
			// item and location...
			// best way to do this is checking against
			// the last[ntree], and *it.

			int level = 0;
			level = cmp_upto(ntrees, ni, *it);
			out << "(Cmp < 0):Level Match:" << level;
			out << std::endl;

			// if expanders following - redirect.
			for(i = 0;(it != dlist.end()) && 
				((*it) -> expander < level) && 
				((*it) -> expander != -1); it++, idx++)
			{
				(*it) -> ref = ni -> ref;
				last[(*it) -> expander] = (*it);
			}

			// add in extra unique ones.
			for(i = level; i < ntrees; i++)
			{
				last[i] = new DisplayItem();
				last[i] -> expander = i;
				last[i] -> index = idx++;
				last[i] -> ref = ni -> ref;
				// random initial flags - visibility checked later.
				if (i == 0)
		  		   last[i] -> flags = FUNKY_EXPANDED | FUNKY_VISIBLE;
				else
				   last[i] -> flags = FUNKY_VISIBLE;
				   //last[i-1] -> flags;

				dlist.insert(it, last[i]);
			}


			// add in item.
			dlist.insert(it, ni);
			// update details.
			ni -> index = idx++;
			if ((last[ntrees-1]->flags & FUNKY_EXPANDED) 
			  &(last[ntrees-1]->flags & FUNKY_VISIBLE))
			{ 
				ni -> flags = FUNKY_VISIBLE;
			}

			// update indices.
			for(; it != dlist.end(); it++)
			{
				(*it) -> index = idx++;
			}

			// finished and added item.
			pqioutput(PQL_DEBUG_BASIC, pqiflfbzone, out.str());
			return 1;
			// drawList(); //not every time!
		}
		pqioutput(PQL_DEBUG_BASIC, pqiflfbzone, out.str());
	}

	// emergency procedures. (stick in at the end)
	// if before it, stop and add item in.
	// check the level difference between
	// item and location...
	// best way to do this is checking against
	// the last[ntree]

	int level = 0;
	level = cmp_upto(ntrees, last[ntrees-1], ni);

	{
		std::ostringstream out;
		out << "Sticking at the End." << std::endl;
		out << "(Cmp < 0): Level Match:" << level;
		out << std::endl;
		pqioutput(PQL_DEBUG_BASIC, pqiflfbzone, out.str());
	}

	// add in extra unique ones.
	for(i = level; i < ntrees; i++)
	{
		std::ostringstream out;
		out << "--> Adding in extra Unique Ones." << std::endl;
		out << "i: " << i << " index: " << idx << " ref: " << ni->ref;
		out << std::endl;
		pqioutput(PQL_DEBUG_BASIC, pqiflfbzone, out.str());

		last[i] = new DisplayItem();
		last[i] -> expander = i;
		last[i] -> index = idx++;
		last[i] -> ref = ni -> ref;
		// random initial flags - visibility checked later.
		if (i == 0)
  		   last[i] -> flags = FUNKY_EXPANDED | FUNKY_VISIBLE;
		else
		   last[i] -> flags = FUNKY_VISIBLE;
		   //last[i-1] -> flags;

		dlist.push_back(last[i]);
	}


	// add in item.
	{
		std::ostringstream out;
		out << "--> Adding in Last Item" << std::endl;
		out << " index: ";
		out << std::endl;
		pqioutput(PQL_DEBUG_BASIC, pqiflfbzone, out.str());
	}

	dlist.push_back(ni);
	// update details.
	ni -> index = idx++;
	if ((last[ntrees-1]->flags & FUNKY_EXPANDED) 
	  &(last[ntrees-1]->flags & FUNKY_VISIBLE))
	{ 
		ni -> flags = FUNKY_VISIBLE;
	}

	// finished and added item.
	return 1;
}

int Fl_Funky_Browser::selectDD(DisplayData *dd)
{
	std::list<DisplayItem *>::iterator it;
	
	if (dd == NULL)
		return 0;
	for(it = dlist.begin(); it != dlist.end();it++)
	{
		if (dd == (*it) -> ref)
		{
			return selectItems((*it) -> index);
		}

	}
	return 0;
}


int	Fl_Funky_Browser::addItem(DisplayData *d)
{
	addItemSeries(d);
	return ItemSeriesDone();
}

int     Fl_Funky_Browser::ItemSeriesDone()
{
	updateList();
	return 1;
}

int	Fl_Funky_Browser::updateList()
{
	if (0 > checkSort())
	{
		checkIndices();
		SortList();
		RePopulate();
	}
	return drawList();
}


// if sorted 1
int	Fl_Funky_Browser::checkSort()
{
	std::list<DisplayItem *>::iterator it, prev;

	it = dlist.begin();
	if (it == dlist.end())
	{
		return 1;
	}

	// add in all the initial levels.
	int res;
	for(prev = it++; it != dlist.end(); prev = it++)
	{
		if (0 < (res = cmp(*prev, *it))) // expect prev <= it, so if (prev > it)
		{
			pqioutput(PQL_WARNING, pqiflfbzone, 
			  "Fl_Funky_Browser::checkSort() Not Sorted()");
			return -1;
		}
		else
		{
			pqioutput(PQL_DEBUG_BASIC, pqiflfbzone, 
			  "Fl_Funky_Browser::checkSort() Sorted()");
		}
	}
	pqioutput(PQL_DEBUG_BASIC, pqiflfbzone, 
		"Fl_Funky_Browser::checkSort() ListSorted()");
	// sorted!
	return 1;
}





void Fl_Funky_Browser::clear()
{
	std::list<DisplayItem *>::iterator it;
	for(it = dlist.begin(); it != dlist.end();)
	{
		delete *it;
		it = dlist.erase(it);
	}
	Fl_Browser::clear();
}

DisplayData *Fl_Funky_Browser::removeItem(int idx)
{
	return NULL;
}

DisplayData *Fl_Funky_Browser::removeItem(int (*fn)(DisplayData *))
{
	return NULL;
}

DisplayData *Fl_Funky_Browser::getSelected()
{
	return NULL;
}

DisplayData *Fl_Funky_Browser::getCurrentItem()
{
	int itemnum = value();
	if (value() <= 1)
		return NULL;

	std::list<DisplayItem *>::iterator it;
	for(it = dlist.begin(); it != dlist.end(); it++)
	{
		if (itemnum == (*it) -> index)
		{
			if ((*it) -> expander != -1)
				return NULL;
			return (*it) -> ref;
		}
	}
	return NULL;
}

	// Worker Functions.
int	Fl_Funky_Browser::SortList()
{
	std::list<DisplayItem *> tmplist = dlist;
	std::list<DisplayItem *>::iterator it;
	std::list<DisplayItem *>::iterator it2;

	dlist.clear();

	// terrible sorting algorithm --- fix with template class.
	// But Okay for now.

	// clean out the expanders first.
	it = tmplist.begin();
	while(it != tmplist.end())
	{
		if ((*it) -> expander > -1)
		{
			// XXX Memory leak???
			it = tmplist.erase(it);
		}
		else
		{
			it++;
		}
	}

	while(tmplist.size() > 0)
	{
		for(it2 = it = tmplist.begin(); it != tmplist.end(); it++)
		{
			if (0 > cmp(*it, *it2)) // if *it < *it2
			{
				it2 = it;
			}
		}
		dlist.push_back(*it2);
		tmplist.erase(it2);
	}
	// sorted!
	return 1;
}

int	Fl_Funky_Browser::RePopulate()
{
	// This adds in the expanders where necessary
	// a major change that should be done as 
	// little as possible.

	int idx = 1;
	int i;
	std::string titleline;
	
	Fl_Browser::clear();

	pqioutput(PQL_DEBUG_BASIC, pqiflfbzone,
		"Fl_Funky_Browser::RePopulate()....");

	idx++;

	std::list<DisplayItem *>::iterator it;

	DisplayItem *last[ntrees];

	it = dlist.begin();
	if (it == dlist.end())
	{
		return drawList();
	}

	// add in all the initial levels.
	for(i = ntrees - 1; i >= 0; i--)
	{
		last[i] = new DisplayItem();
		last[i] -> expander = i;
		last[i] -> index = idx + i;
		last[i] -> ref = (*it) -> ref;
		// random initial flags - visibility checked later.
		if (i == 0)
			last[i] -> flags = FUNKY_EXPANDED | FUNKY_VISIBLE;
		else
			last[i] -> flags = FUNKY_VISIBLE;

		dlist.push_front(last[i]);
	}

	idx += ntrees;
	(*it) -> index = idx++;

	DisplayItem *prev = (*it);

	for(it++; it != dlist.end(); it++)
	{
		// if matching at current of expansion. then ignore.
		int level = 0;
		level = cmp_upto(ntrees, prev, *it);
		for(i = level; i < ntrees; i++)
		{
			last[i] = new DisplayItem();
			last[i] -> expander = i;
			last[i] -> index = idx++;
			last[i] -> ref = (*it) -> ref;
			// random initial flags - visibility checked later.
			if (i == 0)
		  	   last[i] -> flags = FUNKY_EXPANDED | FUNKY_VISIBLE;
			else
			   last[i] -> flags = FUNKY_VISIBLE;

			dlist.insert(it, last[i]);
		}
		(*it) -> index = idx++;
		(*it) -> flags |= FUNKY_VISIBLE;
		prev = (*it);
	}

	return drawList();
}



int	Fl_Funky_Browser::drawList()
{
	// No need to remake list...
	// just iterate through and change the text.
	
	int i;
	int idx = 1;
	std::string titleline;
	//std::cerr << "Fl_Funky_Browser::drawList() 1...." << std::endl;
	
	int nlen = dlist.size() + 1;
	if (nlen != size())
	{
		// fix size.
		// two loops should do the trick.
		for(;size() > nlen;)
			remove(size());
		add("dummy");
		for(;nlen > size();)
			add("dummy");
	}

	//std::cerr << "Fl_Funky_Browser::drawList() 1b...." << std::endl;

	// draw the title.
	for(i = 0; i < ncols; i++)
	{
		// ticked header or not.....
		if (tree[display_order[i]] == 1)
		{
			titleline += "@T";
		}
		else
		{
			titleline += "@E";
		}
		if (sort_direction[display_order[i]] == 1)
		{
			titleline += "@A";
		}
		else
		{
			titleline += "@a";
		}

		titleline += "@D@b@i@.";

		titleline += titles[display_order[i]];
		titleline += "\t";
	}
	// change the text.
	text(idx++, titleline.c_str());

	std::list<DisplayItem *>::iterator it;
	DisplayItem *last[ntrees];
	// Now create it....
	for(it = dlist.begin(); it != dlist.end(); it++, idx++)
	{
		std::string line;
		bool vis = true;
		int depth = ntrees;

		//std::cerr << "Fl_Funky_Browser::drawList() " << idx << std::endl;

		if ((*it) -> expander > -1)
		{
			// save.
			depth = (*it) -> expander; 
			last[(*it) -> expander] = (*it);

			// the line is visible if all of the lasts..
			// are expanded.

			// have an expander.... get the text.
			// only creating the unique bit. - do spaces before.
			for(i = 0; i < (*it) -> expander; i++)
			{
				line += "\t";
			}
		
			// put the subsign..
			line += "@I";

			// if the last one add an expander.
			if ((*it) -> flags & FUNKY_EXPANDED)
			{
				line += "@p@.";
			}
			else
			{
				line += "@P@.";
			}

			line += (*it) -> ref -> txt(display_order[(*it) -> expander]);
			line += "\t";
		}
		else // A Normal element.
		{
			for(i = 0; i < ntrees; i++)
			{
				line += "\t";
			}
			for(i = ntrees; i < ncols; i++)
			{
				if (check_box[display_order[i]])
				{
					if (1 == (*it) -> ref -> check(i))
					{
						line += "@T@.";
					}
					else
					{
						line += "@E@.";
					}
				}

				line += (*it) -> ref -> txt(display_order[i]);
				line += "\t";
			}
			std::ostringstream out;
			out << "Normal Line:" << line << std::endl;
			pqioutput(PQL_DEBUG_BASIC, pqiflfbzone, out.str());
		}

		// check the visibility.
		for(i = 0; i < depth; i++)
		{
			if (!(last[i] -> flags & FUNKY_EXPANDED))
				vis = false;
			line += "\t";
		}
		
		text(idx, line.c_str());
		// Set Visibility and Selected.
		if (!vis)
		{
			(*it) -> flags &= ~FUNKY_VISIBLE;
			hide(idx);
		}
		else
		{
			(*it) -> flags |= FUNKY_VISIBLE;
			show(idx);
		}

		if ((*it) -> flags & FUNKY_SELECTED)
		{
			select(idx, 1);
		}
	}
	redraw();
	return 1;
}

int Fl_Funky_Browser::checkIndices()
{
	// The just checks that the indices are valid....
	// interestingly - you could justifiably want to 
	// display the same column twice so we only have to check 
	// that they are valid.
	
	// Now sort columns so all the trees are 
	// first (both in display + sort)
	int nontree = -1;
	int i;

	// simple sort which is order n if correct.
	for(i = 0; i < ncols; i++)
	{
		// check item.
		if (tree[display_order[i]] == 0)
		{
			// catch first non tree.
			if (nontree < 0)
			{
				nontree = i;
			}
		}
		else // a tree one
		{
			if (nontree != -1) // with nontrees ahead!
			{
				std::ostringstream out;
				out << "Swapping Items (" << i;
				out << ") and (" << nontree << ")";
				out << std::endl;
				pqioutput(PQL_DEBUG_BASIC, pqiflfbzone, out.str());

				// swap the items along.
				for(;i > nontree; i--)
				{
					int sdo = display_order[i - 1];
					display_order[i - 1] = display_order[i];
					sort_order[i - 1] = display_order[i];
					display_order[i] = sdo;
					sort_order[i] = sdo;
				}
				// increment and continue.
				nontree++;
				i++;
			}
		}
	}
	// finally fix the fl_width array.
	for(i = 0; i < ncols; i++)
	{
		fl_widths[i] = widths[display_order[i]];
	}
	fl_widths[ncols] = 0;
	return 1;
}



int Fl_Funky_Browser::toggleCollapseLevel(int row)
{
	int itemnum = row;
	int i;
	std::list<DisplayItem *>::iterator it, it2;

	DisplayItem *last[ntrees];

	for(it = dlist.begin(); it != dlist.end(); it++)
	{
		if ((*it) -> expander > -1)
		{
			last[(*it) -> expander] = (*it);
		}

		if (itemnum == (*it) -> index)
		{
			if ((*it) -> expander < 0)
			{
				// not a expansion point.
				pqioutput(PQL_DEBUG_BASIC, pqiflfbzone, 
				 	"toggleCollapseLevel() Bad PT");
				return 0;
			}

			{
				std::ostringstream out;
				out << "toggleCollapseLevel() Found(" << itemnum;
				out << ")" << std::endl;
				pqioutput(PQL_DEBUG_BASIC, pqiflfbzone, out.str());
			}
			
			if ((*it) -> flags & FUNKY_EXPANDED)
			{
				pqioutput(PQL_DEBUG_BASIC, pqiflfbzone, 
					"Setting EXP -> OFF");
				(*it) -> flags &= ~FUNKY_EXPANDED; // FLIP BIT.
			}
			else
			{
				pqioutput(PQL_DEBUG_BASIC, pqiflfbzone, 
					"Setting EXP -> ON");
				(*it) -> flags |= FUNKY_EXPANDED; // FLIP BIT.
			}

			// Must change the text to match!.
			std::string line;

			// only creating the unique bit. - do spaces before.
			for(i = 0; i < (*it) -> expander; i++)
			{
				line += "\t";
			}
		
			// put the subsign..
			line += "@I";

			// if the last one add an expander.
			if ((*it) -> flags & FUNKY_EXPANDED)
			{
				line += "@p@.";
			}
			else
			{
				line += "@P@.";
			}

			line += (*it) -> ref -> txt(display_order[(*it) -> expander]);
			line += "\t";
			// replace the text.
			text(itemnum, line.c_str());


			// continue down the list 
			// until we reach the next collapse 
			// of the same level.
			it2 = it;
			int idx = itemnum;
			for(it2++, idx++; it2 != dlist.end(); it2++, idx++)
			{
				// if expander + higher level.
			  	if (((*it2) -> expander > -1) &&
				    ((*it2) -> expander <= (*it) -> expander))
				{
					// finished.
					return 1;
				}
				int depth = ntrees;
				// update still.
				if ((*it2) -> expander > -1)
				{
					depth = (*it2) -> expander;
					last[(*it2) -> expander] = (*it2);
				}

				// check visibility.
				bool vis = true;

				{
				  std::ostringstream out;
				  out << "LINE: " << text(idx) << std::endl;
				  out << "EXP_TREE: ";
				  pqioutput(PQL_DEBUG_BASIC, pqiflfbzone, out.str());
				}

				for(i = 0; i < depth; i++)
				{
					if (!(last[i] -> flags & FUNKY_EXPANDED))
					{
				  		pqioutput(PQL_DEBUG_BASIC, pqiflfbzone, "\tOFF");
						vis = false;
					}
					else
					{
				  		pqioutput(PQL_DEBUG_BASIC, pqiflfbzone, "\tON");
					}
				}
		
				// Set Visibility and Selected.
				if (!vis)
				{
					(*it2) -> flags &= ~FUNKY_VISIBLE;
				  	pqioutput(PQL_DEBUG_BASIC, pqiflfbzone, "\tSUB EXP -> OFF");
					hide(idx);
				}
				else
				{
					(*it2) -> flags |= FUNKY_VISIBLE;
				  	pqioutput(PQL_DEBUG_BASIC, pqiflfbzone, "\tSUB EXP -> ON");
					show(idx);
				}
			}
		}
	}
	return 0;
}


int Fl_Funky_Browser::selectItems(int row)
{
	int itemnum = row;
	int idx;
	std::list<DisplayItem *>::iterator it, it2;

	for(it = dlist.begin(), idx = 2; it != dlist.end(); it++, idx++)
	{
		// Not Selected.
		(*it) -> flags &= ~FUNKY_SELECTED;
		select(idx, 0);

		{
		  std::ostringstream out;
		  out << "Un - Selecting(" << idx << "):";
		  out << text(idx) << std::endl;
		  pqioutput(PQL_DEBUG_BASIC, pqiflfbzone, out.str());
		}

		if (itemnum == (*it) -> index) {
		if (one_select)
		{
			if ((*it) -> expander == -1)
			{
				(*it) -> flags |= FUNKY_SELECTED;
				select(idx, 1);
			}
			else // iterate down until.
			{
				for(; (it != dlist.end()) &&
					((*it) -> expander != -1); it++, idx++)
				{
					(*it) -> flags &= ~FUNKY_SELECTED;
					select(idx, 0);
					if (!((*it) -> flags & FUNKY_EXPANDED))
						toggleCollapseLevel(idx);
				}
				// end condition
				if (it == dlist.end())
				{
					do_callback();
					return 1;
				}
				(*it) -> flags |= FUNKY_SELECTED;
				select(idx, 1);

			}
		}
		else 
		{
			(*it) -> flags |= FUNKY_SELECTED;
			select(idx, 1);
			int depth = (*it) -> expander;

			for(it++, idx++; (it != dlist.end()) && 
				(((*it) -> expander == -1) || 
				((*it) -> expander > depth)); it++, idx++)
			{
				(*it) -> flags |= FUNKY_SELECTED;
				select(idx, 1);

				std::ostringstream out;
				out << "Selecting(" << idx << "):";
				out << text(idx) << std::endl;
				pqioutput(PQL_DEBUG_BASIC, pqiflfbzone, out.str());
			}
			if (it == dlist.end())
			{ 
				do_callback();
				return 1;
			}
		}}
	}
	do_callback();
	return 0;
}


int     Fl_Funky_Browser::getCheckState(int row, int col)
{
	if (row != -1)
	{
		pqioutput(PQL_DEBUG_BASIC, pqiflfbzone, 
			"Fl_Funky_Browser::getCheckState() Cannot Handle Random Row yet!");
	}
	row = value();
	col = 0;

	int idx;
	std::list<DisplayItem *>::iterator it, it2;

	for(it = dlist.begin(), idx = 2; it != dlist.end() 
				&& (idx  <= row); it++, idx++)
	{
		if (row == (*it) -> index)
		{
			if ((*it) -> expander != -1)
			{
				pqioutput(PQL_DEBUG_BASIC, pqiflfbzone, "No CheckBox");
				return 0;
			}

			// toggle.
			return (*it) -> ref -> check(display_order[col]);
		}
	}
	return 0;
}


int Fl_Funky_Browser::toggleCheckBox(int row, int col)
{
	int itemnum = row;
	col = 0;
	int i, idx;
	std::list<DisplayItem *>::iterator it, it2;

	{
	  std::ostringstream out;
	  out << "Fl_Funky_Browser::toggleCheckBox(" << row << ".";
	  out << col << ")" << std::endl;
	  pqioutput(PQL_DEBUG_BASIC, pqiflfbzone, out.str());
	}

	for(it = dlist.begin(), idx = 2; it != dlist.end(); it++, idx++)
	{
		if (itemnum == (*it) -> index)
		{
			if ((*it) -> expander != -1)
			{
		  		pqioutput(PQL_DEBUG_BASIC, pqiflfbzone, "No CheckBox");
				return 0;
			}

			// toggle.
			int chk = (*it) -> ref -> check(display_order[col]);
			if (chk == 1)
			{
		  		pqioutput(PQL_DEBUG_BASIC, pqiflfbzone, "Checked Off");
				(*it) -> ref -> check(display_order[col], 0);
			}
			else
			{
		  		pqioutput(PQL_DEBUG_BASIC, pqiflfbzone, "Checked On");
				(*it) -> ref -> check(display_order[col], 1);
			}
			
			// do line again....
			std::string line;
			for(i = 0; i < ntrees; i++)
			{
				line += "\t";
			}
			for(i = ntrees; i < ncols; i++)
			{
				if (check_box[display_order[i]])
				{
					if (1 == (*it) -> ref -> check(i))
					{
						line += "@T@.";
					}
					else
					{
						line += "@E@.";
					}
				}

				line += (*it) -> ref -> txt(display_order[i]);
				line += "\t";
			}
			text(itemnum, line.c_str());


			// select line, and then return.
			return selectItems(itemnum);
		}
	}
	return 0;
}

int Fl_Funky_Browser::toggle_TreeSetting(int col)
{
	if (tree[display_order[col]] == 1)
	{
		// already a tree -> make non tree.
		tree[display_order[col]] = 0;
		pqioutput(PQL_DEBUG_BASIC, pqiflfbzone, 
			"Fl_Funky_Browser::toggle_TreeSetting() - Off!");
		ntrees--;
	}
	else
	{
		tree[display_order[col]] = 1;
		ntrees++;
		pqioutput(PQL_DEBUG_BASIC, pqiflfbzone, 
			"Fl_Funky_Browser::toggle_TreeSetting() - On!");
	}

	checkIndices();
	SortList();
	RePopulate();
	return 1;
}



int Fl_Funky_Browser::toggle_ArrowSetting(int col)
{
	if (sort_direction[display_order[col]] == 1)
	{
		sort_direction[display_order[col]] = 0;
	}
	else
	{
		sort_direction[display_order[col]] = 1;
	}
	SortList();
	RePopulate();

	return 1;
}







// can handle NULLs.
int	Fl_Funky_Browser::cmp_upto(int lvl, DisplayItem *i1, DisplayItem *i2)
{
	if ((i1 == NULL) || (i2 == NULL))
	{
		return 0;
	}

	//std::cerr << "Fl_Funky_Browser::cmp_upto(): ";
	int i;
	for(i = 0; i < lvl; i++)
	{
		if (0 != i1 -> ref -> cmp(sort_order[i], i2 -> ref))
		{
	//		std::cerr << "D(" << i << ")" << std::endl;
			return i;
		}
	//	std::cerr << "M(" << i << ") ";
	}
	//std::cerr << "S(" << lvl << ")" << std::endl;
	return lvl;
}


int	Fl_Funky_Browser::cmp(DisplayItem *i1, DisplayItem *i2)
{
	int i;
	int ret;
	for(i = 0; i < ncols; i++)
	{
		if (0 != (ret = i1 -> ref -> cmp(sort_order[i], i2 -> ref)))
		{
			if (sort_direction[sort_order[i]] < 1)
			{
				// reverse direction.
				return -ret;
			}
			return ret;
		}
	}
	return 0;
}
	
int     Fl_Funky_Browser::setTitle(int col, std::string name)
{
	titles[col % ncols] = name;
	drawList();
	return 1;
}

	
int     Fl_Funky_Browser::setCheck(int col)
{
	check_box[col % ncols] = true;
	return 1;
}


static const int FL_SIGN_NONE = 0;
static const int FL_SIGN_EXPANDED = 0x01;
static const int FL_SIGN_CONTRACT = 0x02;
static const int FL_SIGN_SUBITEM =  0x04;
static const int FL_SIGN_TICKED =  0x08;
static const int FL_SIGN_BOX =  0x10;
static const int FL_SIGN_UP_ARROW =  0x20;
static const int FL_SIGN_DOWN_ARROW =  0x40;

void Fl_Funky_Browser::item_draw(void* v, int X, int Y, int W, int H) const
{
  // Round About way of getting the text.
  int inum = lineno(v);
  // bad cast, but it doesn't change the text....
  // actually it does!!!! XXX BAD. should copy.
  char* str = (char *) text(inum);
  const int* i = column_widths();


  while (W > 6) {	// do each tab-seperated field
    int w1 = W;	// width for this field
    char* e = 0; // pointer to end of field or null if none
    if (*i) { // find end of field and temporarily replace with 0
      e = strchr(str, column_char());
      if (e) {*e = 0; w1 = *i++;}
    }
    int tsize = textsize();
    Fl_Font font = textfont();
    Fl_Color lcol = textcolor();
    Fl_Align talign = FL_ALIGN_LEFT;

    //rmf24 new variable.
    int expand = FL_SIGN_NONE;

    // check for all the @-lines recognized by XForms:
    while (*str == format_char() && *++str && *str != format_char()) {
      switch (*str++) {
      case 'l': case 'L': tsize = 24; break;
      case 'm': case 'M': tsize = 18; break;
      case 's': tsize = 11; break;
      case 'b': font = (Fl_Font)(font|FL_BOLD); break;
      case 'i': font = (Fl_Font)(font|FL_ITALIC); break;
      case 'f': case 't': font = FL_COURIER; break;
      case 'c': talign = FL_ALIGN_CENTER; break;
      case 'r': talign = FL_ALIGN_RIGHT; break;
      case 'B': 
	// rmf24 changing to we can reach the info, 
	//if (!(((FL_BLINE*)v)->flags & SELECTED)) {
	if (1 == selected(inum))
	{
	  fl_color((Fl_Color)strtol(str, &str, 10));
	  fl_rectf(X, Y, w1, H);
	} else strtol(str, &str, 10);
        break;
      case 'C':
	lcol = (Fl_Color)strtol(str, &str, 10);
	break;
      case 'F':
	font = (Fl_Font)strtol(str, &str, 10);
	break;
      case 'N':
	lcol = FL_INACTIVE_COLOR;
	break;
      case 'S':
	tsize = strtol(str, &str, 10);
	break;
      case '-':
	fl_color(FL_DARK3);
	fl_line(X+3, Y+H/2, X+w1-3, Y+H/2);
	fl_color(FL_LIGHT3);
	fl_line(X+3, Y+H/2+1, X+w1-3, Y+H/2+1);
	break;
      case 'u':
      case '_':
	fl_color(lcol);
	fl_line(X+3, Y+H-1, X+w1-3, Y+H-1);
	break;
// rmf24 - mods to include the (P)lus/(p) available Buttons.
      case 'T':
	expand |= FL_SIGN_TICKED;
	break;
      case 'E':
	expand |= FL_SIGN_BOX;
	break;
      case 'P':
	expand |= FL_SIGN_EXPANDED;
	break;
      case 'p':
	expand |= FL_SIGN_CONTRACT;
	break;
      case 'I':
	expand |= FL_SIGN_SUBITEM;
	break;
      case 'A':
	expand |= FL_SIGN_UP_ARROW;
	break;
      case 'a':
	expand |= FL_SIGN_DOWN_ARROW;
	break;
      case 'D':
	fl_color(FL_DARK3);
	fl_rectf(X + 2, Y + 1, w1 - 4, H - 2);
	// change the drawing color.
        lcol = fl_contrast(lcol, selection_color());
	break;
      case '.':
	goto BREAK;
      case '@':
	str--; goto BREAK;
      }
    }
  BREAK:
    fl_font(font, tsize);
    // changed to reach the data.
    //if (((FL_BLINE*)v)->flags & SELECTED)
    if (1 == selected(inum))
      lcol = fl_contrast(lcol, selection_color());
    if (!active_r()) lcol = fl_inactive(lcol);
    fl_color(lcol);

// rmf24 extensions to the class.
    if (expand != FL_SIGN_NONE)
    {
	int req_end_len = 0;
	int req_init_len = 0;
	int CS = H - 4;

	if (expand & FL_SIGN_TICKED)
	{
		CS = H - 6;
			// draw a box with tick at the start.
		fl_loop(X+H/2-CS/2, Y+H/2-CS/2, 
			X+H/2-CS/2, Y+H/2+CS/2, 
			X+H/2+CS/2, Y+H/2+CS/2, 
			X+H/2+CS/2, Y+H/2-CS/2);

		fl_line(X+H/2-CS/2, Y+H/2, X+H/2, Y+H/2+CS/2);
		fl_line(X+H/2, Y+H/2+CS/2, X+H/2+CS/2, Y+H/2-CS/2);
		// second set of lines.... to make it visible.
		fl_line(X+H/2-CS/2-1, Y+H/2, X+H/2-1, Y+H/2+CS/2);
		fl_line(X+H/2-1, Y+H/2+CS/2, X+H/2+CS/2-1, Y+H/2-CS/2);

		// indicate space required.
		req_init_len = (int) H;
	}
	else if (expand & FL_SIGN_BOX)
	{
		CS = H - 6;
			// draw a box at the start.
		fl_loop(X+H/2-CS/2, Y+H/2-CS/2, 
			X+H/2-CS/2, Y+H/2+CS/2, 
			X+H/2+CS/2, Y+H/2+CS/2, 
			X+H/2+CS/2, Y+H/2-CS/2);

		// indicate space required.
		req_init_len = (int) H;
	}
	else if (expand & FL_SIGN_CONTRACT)
	{
			// draw a sign with a plus.... at the start.
		fl_loop(X+H/2-CS/2, Y+H/2-CS/2, 
			X+H/2-CS/2, Y+H/2+CS/2, 
			X+H/2+CS/2, Y+H/2+CS/2, 
			X+H/2+CS/2, Y+H/2-CS/2);

		fl_line(X+H/2, Y+H/2-CS/2, X+H/2, Y+H/2+CS/2);
		fl_line(X+H/2-CS/2, Y+H/2, X+H/2+CS/2, Y+H/2);

		// indicate space required.
		req_init_len = (int) H;
	}
	else if (expand & FL_SIGN_EXPANDED)
	{
			// draw a sign with a minus.... at the start.
		fl_loop(X+H/2-CS/2, Y+H/2, 
			X+H/2, Y+H/2+CS/2, 
			X+H/2+CS/2, Y+H/2, 
			X+H/2, Y+H/2-CS/2);

		// No Vertical Line.
		//fl_line(X+H/2, Y+H/2-CS/2, X+H/2, Y+H/2+CS/2);
		fl_line(X+H/2-CS/2, Y+H/2, X+H/2+CS/2, Y+H/2);

		// indicate space required.
		req_init_len = (int) H;
	}

	if (expand & FL_SIGN_UP_ARROW)
	{
		// draw an uparrow.
		fl_polygon(X+req_init_len+H/2-CS/2, Y+H/2+CS/2, 
			X+req_init_len+H/2, Y+H/2-CS/2, 
			X+req_init_len+H/2+CS/2, Y+H/2+CS/2);

		// indicate space required.
		req_init_len += (int) H;
	}
	else if (expand & FL_SIGN_DOWN_ARROW)
	{
		fl_polygon(X+req_init_len+H/2-CS/2, Y+H/2-CS/2, 
			X+req_init_len+H/2, Y+H/2+CS/2, 
			X+req_init_len+H/2+CS/2, Y+H/2-CS/2);

		// indicate space required.
		req_init_len += (int) H;
	}

	// actually draw the text
        fl_draw(str, X+3+req_init_len, Y, w1-6-req_end_len-req_init_len, H, e ? Fl_Align(talign|FL_ALIGN_CLIP) : talign, 0, 0);
    }
    else
    {
	  // the original case.
          fl_draw(str, X+3, Y, w1-6, H, e ? Fl_Align(talign|FL_ALIGN_CLIP) : talign, 0, 0);
    }

    if (!e) break; // no more fields...
    *e = column_char(); // put the seperator back
    X += w1;
    W -= w1;
    str = e+1;
  }
}


// taken from the FLTK template 

int Fl_Funky_Browser::handle(int event) 
{
 	int x = Fl::event_x();
	int y = Fl::event_y();

	int bbx, bby, bbw, bbh;
	bbox(bbx, bby, bbw, bbh);
	if (!Fl::event_inside(bbx, bby, bbw, bbh))
	{
		return Fl_Browser::handle(event);
	}


	switch(event) 
	{
	case FL_PUSH:
		// save location, and tell the world.
		{
		  std::ostringstream out;
		  out << "FL_PUSH Event at (" << x << "," << y << ")";
		  out << std::endl;
		  pqioutput(PQL_DEBUG_BASIC, pqiflfbzone, out.str());
		}
		// if in row 1, then dragndrop. else
		// might before an expansion.
		if (0 == handle_push(x,y))
		{
		  	std::ostringstream out;
			out << "Sending FL_PUSH to Fl_Browser...";
			out << " for selection and callback.";
			out << std::endl;
		  	pqioutput(PQL_DEBUG_BASIC, pqiflfbzone, out.str());

			return Fl_Browser::handle(event);
		}
		redraw();
		return 1;
	case FL_DRAG: 
		{
		  std::ostringstream out;
		  out << "FL_DRAG Event at (" << x << "," << y << ")";
		  out << std::endl;
		  pqioutput(PQL_DEBUG_BASIC, pqiflfbzone, out.str());
		}
		return 1;
	case FL_RELEASE:
		{
		  std::ostringstream out;
		  out << "FL_RELEASE Event at (" << x << "," << y << ")";
		  out << std::endl;
		  pqioutput(PQL_DEBUG_BASIC, pqiflfbzone, out.str());
		}
		if (dragging())
		{
			handle_release(x,y);
			redraw();
		}
		return 1;
	default:
		return Fl_Browser::handle(event);
	}
}

static const int DRAG_MODE_NONE = 0;
static const int DRAG_MODE_EDGE = 1;
static const int DRAG_MODE_COLUMN = 2;

static const int EDGE_TOLERANCE = 5;
static const int BOX_TOLERANCE_RIGHT = 15;
static const int ARROW_TOLERANCE_RIGHT = 30;

static const int BOX_TOLERANCE_LEFT = -10;
static const int MIN_WIDTH = 40;

int Fl_Funky_Browser::handle_push(int x, int y)
{
	void *v = find_item(y);
	if (v == NULL)
	{
		// deselect.
		//deselect();
		return 0;
	}
  	int inum = lineno(v);
	int i;

	// which row???
	// if standard row - was it at a box (ie H from start)....
	if (inum != 1)
	{
		drag_mode = DRAG_MODE_NONE;
		int xloc = x - leftedge();
		for(i = 0; (xloc > BOX_TOLERANCE_RIGHT) && (i < ncols); i++)
		{
			xloc -= widths[display_order[i]];
		}
		if (xloc > BOX_TOLERANCE_LEFT)
		{
			pqioutput(PQL_DEBUG_BASIC, pqiflfbzone, "Clicked on The Edge!");
			// we hit an edge.
			if (tree[display_order[i]] == 1)
			{
				toggleCollapseLevel(inum);
				return 1;
			}
			else if (check_box[display_order[i]] == true)
			{
				toggleCheckBox(inum, i);
				return 1;
			}
		}
		// our select policy...
		selectItems(inum);
		// else select it.
		// select(inum);
		return 1;
	}
	else
	{
		pqioutput(PQL_DEBUG_BASIC, pqiflfbzone, "Clicked on The Header!");
		drag_mode = DRAG_MODE_NONE;
		int xloc = x - leftedge();
		for(i = 0; (xloc > widths[display_order[i]]) && (i < ncols); i++)
		{
			xloc -= widths[display_order[i]];
		}
		{
			std::ostringstream out;
			out << "Click Parameters: X: " << x << " Y: " << y;
			out << " NCols: " << ncols;
			out << " Drag Column: " << i << std::endl;
			pqioutput(PQL_DEBUG_BASIC, pqiflfbzone, out.str());
		}

		if (abs(xloc) < EDGE_TOLERANCE)
		{
			pqioutput(PQL_DEBUG_BASIC, pqiflfbzone, 
				"Clicked on The Edge!");
			drag_mode = DRAG_MODE_EDGE;
			// set mouse icon.
			// save point.
			drag_column = i;
			drag_x = x;
			drag_y = y;
			fl_cursor(FL_CURSOR_INSERT, FL_FOREGROUND_COLOR, FL_BACKGROUND_COLOR);
			return 1;
		}
		else if ((xloc > EDGE_TOLERANCE) && 
				(xloc < BOX_TOLERANCE_RIGHT))
		{
			pqioutput(PQL_DEBUG_BASIC, pqiflfbzone, 
				"Clicked on The Box!");
			toggle_TreeSetting(i);
		}
		else if ((xloc > BOX_TOLERANCE_RIGHT) && 
				(xloc < ARROW_TOLERANCE_RIGHT))
		{
			pqioutput(PQL_DEBUG_BASIC, pqiflfbzone, 
				"Clicked on The Arrow!");
			toggle_ArrowSetting(i);
		}
		else if ((xloc > ARROW_TOLERANCE_RIGHT) && 
				(xloc < widths[display_order[i]] - EDGE_TOLERANCE))
		{
			pqioutput(PQL_DEBUG_BASIC, pqiflfbzone, 
				"Clicked on The Title!");

			drag_mode = DRAG_MODE_COLUMN;
			// set mouse icon.
			// save point.
			drag_column = i;
			drag_x = x;
			drag_y = y;
			fl_cursor(FL_CURSOR_HAND, FL_FOREGROUND_COLOR, FL_BACKGROUND_COLOR);
			return 1;
		}
	}

	drag_mode = DRAG_MODE_NONE;

	return 1;
}

bool Fl_Funky_Browser::dragging()
{
	if (drag_mode != DRAG_MODE_NONE)
		return true;
	return false;
}

int Fl_Funky_Browser::handle_release(int x, int y)
{
	{
	  std::ostringstream out;
	  out << "Drag Handling from (" << drag_x << ",";
	  out << drag_y << ") -> (" << x << "," << y << ")" << std::endl;
	  pqioutput(PQL_DEBUG_BASIC, pqiflfbzone, out.str());
	}
	
	fl_cursor(FL_CURSOR_DEFAULT, FL_FOREGROUND_COLOR, FL_BACKGROUND_COLOR);
	if (drag_mode == DRAG_MODE_EDGE)
	{
		if ((drag_column < 1) || (drag_column > ncols))
		{
	  		pqioutput(PQL_DEBUG_BASIC, pqiflfbzone, "Can't Drag that Column!");
			drag_mode = DRAG_MODE_NONE;
			return 1;
		}

		int x_change = x - drag_x;
		int new_width = widths[display_order[drag_column - 1]];
		new_width += x_change;
		if (new_width < MIN_WIDTH)
		{
			new_width = MIN_WIDTH;
		}

		widths[display_order[drag_column - 1]] = new_width;
		fl_widths[drag_column - 1] = new_width;
	}
	else
	{

	}

	drag_mode = DRAG_MODE_NONE;
	return 1;
}


int	Fl_Funky_Browser::setup(std::string opts)
{
	{
	  std::ostringstream out;
	  out << "Fl_Funky_Browser::setup(" << opts << ")" << std::endl;
	  pqioutput(PQL_DEBUG_BASIC, pqiflfbzone, out.str());
	}
	int loc = 0;
	char name[1024];
	int so, w, n, t;

	int tc = 0;
	for(int i = 0; i < ncols; i++)
	{
		int r = sscanf(&(opts.c_str()[loc]), 
				" %1000s (%d:%d:%d)%n", name, &so, &w, &t, &n);
		if (r < 4)
		{
		  pqioutput(PQL_DEBUG_BASIC, pqiflfbzone, 
		  	"Failed to Finish Reading");
		  break;
		}
		// save
		{
		  std::ostringstream out;
		  out << "Data for Column(" << i << ") Name: " << name;
		  out << " sOrder: " << so << " Width: " << w;
		  pqioutput(PQL_DEBUG_BASIC, pqiflfbzone, out.str());
		}
		titles[i] = name;
		sort_order[i] = display_order[i] = so;
		widths[i] = w;
		if (t == 1)
		{
			tree[i] = 1;
			tc++;
		}
		else
			tree[i] = 0;
	

		loc += n;
	}
	ntrees = tc;
	checkIndices();
	SortList();
	RePopulate();
	return 1;
}

std::string Fl_Funky_Browser::setup()
{
	std::string opts;
	char str[1024];
	for(int i = 0; i < ncols; i++)
	{
		sprintf(str, "%s (%d:%d:%d)", titles[i].c_str(), sort_order[i], widths[i], tree[i]);
		opts += str;
		if (i + 1 != ncols)
			opts += "\t";
	}

	{
	  std::ostringstream out;
	  out << "Fl_Funky_Browser::setup() = " << opts << std::endl;
	  pqioutput(PQL_DEBUG_BASIC, pqiflfbzone, out.str());
	}

	return opts;
}



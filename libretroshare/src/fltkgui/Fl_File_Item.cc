//
// "$Id: Fl_File_Item.cc,v 1.3 2007-02-18 21:46:49 rmf24 Exp $"
//
// This is hacked up FLTK code, and so is released under  
// their licence. (below)
// Copyright 2004-2006 by Robert Fernie.
//
// Please report all bugs and problems to "retroshare@lunamutt.com".
//
/////////////////////////////////////////////////////////////////////
//
// C function type code for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2004 by Bill Spitzak and others.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA.
//
// Please report all bugs and problems to "fltk-bugs@fltk.org".
//

#include <FL/Fl.H>
#include "Fl_Tree_Browser.h"
#include <stdio.h>
#include <stdlib.h>

#include <iostream>

//
//extern int i18n_type;
//extern const char* i18n_include;
//extern const char* i18n_function;
//extern const char* i18n_file;
//extern const char* i18n_set;
//extern char i18n_program[];

////////////////////////////////////////////////////////////////
// quick check of any C code for legality, returns an error message

static char buffer[128]; // for error messages

// check a quoted string ending in either " or ' or >:
const char *_q_check(const char * & c, int type) {
  for (;;) switch (*c++) {
  case '\0':
    sprintf(buffer,"missing %c",type);
    return buffer;
  case '\\':
    if (*c) c++;
    break;
  default:
    if (*(c-1) == type) return 0;
  }
}

// check normal code, match braces and parenthesis:
const char *_c_check(const char * & c, int type) {
  const char *d;
  for (;;) switch (*c++) {
  case 0:
    if (!type) return 0;
    sprintf(buffer, "missing %c", type);
    return buffer;
  case '/':
    // Skip comments as needed...
    if (*c == '/') {
      while (*c != '\n' && *c) c++;
    } else if (*c == '*') {
      c++;
      while ((*c != '*' || c[1] != '/') && *c) c++;
      if (*c == '*') c+=2;
      else {
        return "missing '*/'";
      }
    }
    break;
  case '#':
    // treat cpp directives as a comment:
    while (*c != '\n' && *c) c++;
    break;
  case '{':
    if (type==')') goto UNEXPECTED;
    d = _c_check(c,'}');
    if (d) return d;
    break;
  case '(':
    d = _c_check(c,')');
    if (d) return d;
    break;
  case '\"':
    d = _q_check(c,'\"');
    if (d) return d;
    break;
  case '\'':
    d = _q_check(c,'\'');
    if (d) return d;
    break;
  case '}':
  case ')':
  UNEXPECTED:
    if (type == *(c-1)) return 0;
    sprintf(buffer, "unexpected %c", *(c-1));
    return buffer;
  }
}

const char *c_check(const char *c, int type) {
  return _c_check(c,type);
}

////////////////////////////////////////////////////////////////
// UNSURE
//#include "function_panel.h"
//#include <FL/fl_ask.H>
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

Fl_Item *Fl_Dir_Item::make(Fl_Item *p) {
  //std::cerr << "Fl_Dir_Item::make()" << std::endl;
  //Fl_Item *p = Fl_Item::current;
  while (p && p->is_file()) p = p->parent;

  Fl_Dir_Item *o = new Fl_Dir_Item();
  o->name("Dir: printf(\"Hello, World!\\n\");");
  o->add(p);
  o->factory = this;
  return o;
}

void file_requestdir(std::string person, std::string dir);

void Fl_Dir_Item::open() 
{
	//std::cerr << "open Dir..." << std::endl;
  	Fl_Item *p = NULL;

	int ccount = 0;
  	for(p = next; (p && (p->parent == this));p = p->next)
	{
		ccount++;
	}

	//std::cerr << "count:" << ccount << std::endl;

	if (ccount == 0) // && (lload > time(NULL) + MIN_RELOAD))
	{
		// get our path....
		std::string fulldir(name());
		int lvl = level;
		//std::cerr << "level:" << level << std::endl;
		//std::cerr << "prev:" << (int) prev << std::endl;
		//std::cerr << "prev->parent:" << (int) prev->parent << std::endl;

  		for(p = prev; (p && p->parent);p = p->prev)
		{
			//std::cerr << "fulldir srch: " << p -> name() << std::endl;
			if (p->level < lvl)
			{
				std::string subdir(p->name());
				fulldir = subdir + "/" + fulldir;
				lvl = p->level;
			}
		}
		fulldir = "/" + fulldir;

		if (p)
		{
			Fl_Person_Item *pi;
			if ((pi = dynamic_cast<Fl_Person_Item *>(p)))
			{
				std::string person(pi->person_hash);
				file_requestdir(person, fulldir);
				//std::cerr << "Requesting: " << person << ":" << fulldir << std::endl;
			}
			else
			{
				//std::cerr << "Not PersonItem" << std::endl;
			}

		}
		else
		{
			//std::cerr << "No Requesting: NULL" << ":" << fulldir << std::endl;
		}


	}
	else
	{
  	  for(p = next; (p && (p->parent == this));p = p->next)
	  {
		//std::cerr << "Setting Item Visible" << std::endl;
		p -> visible = 1;
		//p -> open_ = 1;
	  }
	  redraw_browser();
	}
}

Fl_Dir_Item Fl_Dir_Item_type;

////////////////////////////////////////////////////////////////

Fl_Item *Fl_File_Item::make(Fl_Item *p) {
  //std::cerr << "Fl_File_Item::make()" << std::endl;
  while (p && p->is_file()) p = p->parent;
  if (!p) {
	//std::cerr << "File in no directory!" << std::endl;
  	return 0;
  }

  Fl_File_Item *o = new Fl_File_Item();
  o->name("file: hello world");
  o->add(p);
  o->factory = this;
  //redraw_browser();
  return o;
}

void Fl_File_Item::open() 
{
	//std::cerr << "open File? How..." << std::endl;
	redraw_browser();
}

Fl_File_Item Fl_File_Item_type;

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

Fl_Item *Fl_Person_Item::make(Fl_Item *p) {
  //std::cerr << "Fl_Person_Item::make()" << std::endl;
  //while (p) && p->is_file()) p = p->parent;
  //while (p) && p->is_file()) p = p->parent;
  p = NULL; // always at top.

  Fl_Person_Item *o = new Fl_Person_Item();
  o->name("person");
  o->add(p);
  o->factory = this;
  return o;
}


void Fl_Person_Item::open() 
{
	//std::cerr << "open Person? How..." << std::endl;
	Fl_Item *p;
	{
  	  for(p = next; (p && (p->parent == this));p = p->next)
	  {
		//std::cerr << "Setting Item Visible" << std::endl;
		p -> visible = 1;
	  }
	  redraw_browser();
	}
}

Fl_Person_Item Fl_Person_Item_type;

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

Fl_Item *Fl_Msg_Item::make(Fl_Item *p) {
  //std::cerr << "Fl_Msg_Item::make()" << std::endl;
  while (p && p->is_file()) p = p->parent;
  if (!p) {
	//std::cerr << "File in no directory!" << std::endl;
  	return 0;
  }

  Fl_Msg_Item *o = new Fl_Msg_Item();
  o->name("msg");
  o->add(p);
  o->factory = this;
  //redraw_browser();
  return o;
}

void Fl_Msg_Item::open() 
{
	//std::cerr << "open Msg? How..." << std::endl;
	redraw_browser();
}

Fl_Msg_Item Fl_Msg_Item_type;

////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////

const char* Fl_Item::class_name(const int need_nest) const {
  Fl_Item* p = parent;
  while (p) {
    if (p->is_class()) {
      // see if we are nested in another class, we must fully-qualify name:
      // this is lame but works...
      const char* q = 0;
      if(need_nest) q=p->class_name(need_nest);
      if (q) {
	static char s[256];
	if (q != s) fltk_strlcpy(s, q, sizeof(s));
	fltk_strlcat(s, "::", sizeof(s));
	fltk_strlcat(s, p->name(), sizeof(s));
	return s;
      }
      return p->name();
    }
    p = p->parent;
  }
  return 0;
}

//
// End of "$Id: Fl_File_Item.cc,v 1.3 2007-02-18 21:46:49 rmf24 Exp $".
//

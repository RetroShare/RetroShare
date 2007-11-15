//
// "$Id: Fl_Tree_Browser.cc,v 1.3 2007-02-18 21:46:49 rmf24 Exp $"
//
// This is hacked up FLTK code, and so is released under 
// their licence. (below)
// Copyright 2004-2006 by Robert Fernie.
//
// Please report all bugs and problems to "retroshare@lunamutt.com".
//
/////////////////////////////////////////////////////////////////////
//
// Widget type code for the Fast Light Tool Kit (FLTK).
//
// Each object described by Fluid is one of these objects.  They
// are all stored in a double-linked list.
//
// They "type" of the object is covered by the virtual functions.
// There will probably be a lot of these virtual functions.
//
// The type browser is also a list of these objects, but they
// are "factory" instances, not "real" ones.  These objects exist
// only so the "make" method can be called on them.  They are
// not in the linked list and are not written to files or
// copied or otherwise examined.
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
#include <FL/Fl_Browser_.H>
#include <FL/fl_draw.H>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <iostream>

#include "Fl_Tree_Browser.h"

#include <FL/Fl_Pixmap.H>

//static Fl_Pixmap	lock_pixmap(lock_xpm);
//static Fl_Pixmap	unlock_pixmap(unlock_xpm);

void selection_changed(Fl_Item *p);
size_t  fltk_strlcpy(char *dst, const char *src, size_t size);
const char* subclassname(Fl_Item *i) { return i -> type_name(); }


////////////////////////////////////////////////////////////////

class Fl_Tree_Browser : public Fl_Browser_ {
  friend class Fl_Item;

  // required routines for Fl_Browser_ subclass:
  void *item_first() const ;
  void *item_next(void *) const ;
  void *item_prev(void *) const ;
  int item_selected(void *) const ;
  void item_select(void *,int);
  int item_width(void *) const ;
  int item_height(void *) const ;
  void item_draw(void *,int,int,int,int) const ;
  int incr_height() const ;

public:	

  int handle(int);
  void callback();
  Fl_Tree_Browser(int,int,int,int,const char * =0);
};

static Fl_Tree_Browser *tree_browser;

Fl_Widget *make_tree_browser(int x,int y,int w,int h, const char *) {
  return (tree_browser = new Fl_Tree_Browser(x,y,w,h));
}

void select(Fl_Item *o, int v) {
  tree_browser->select(o,v,1);
  //  Fl_Item::current = o;
}

void select_only(Fl_Item *o) {
  tree_browser->select_only(o,1);
}

void deselect() {
  tree_browser->deselect();
  //Fl_Item::current = 0; // this breaks the paste & merge functions
}

Fl_Item *Fl_Item::first;
Fl_Item *Fl_Item::last;
Fl_Item *Fl_Item::current;

static void Fl_Tree_Browser_callback(Fl_Widget *o,void *) {
  ((Fl_Tree_Browser *)o)->callback();
}

Fl_Tree_Browser::Fl_Tree_Browser(int X,int Y,int W,int H,const char*l)
: Fl_Browser_(X,Y,W,H,l) {
  type(FL_MULTI_BROWSER);
  Fl_Widget::callback(Fl_Tree_Browser_callback);
  when(FL_WHEN_RELEASE);
}

void *Fl_Tree_Browser::item_first() const {return Fl_Item::first;}

void *Fl_Tree_Browser::item_next(void *l) const {return ((Fl_Item*)l)->next;}

void *Fl_Tree_Browser::item_prev(void *l) const {return ((Fl_Item*)l)->prev;}

int Fl_Tree_Browser::item_selected(void *l) const {return ((Fl_Item*)l)->new_selected;}

void Fl_Tree_Browser::item_select(void *l,int v) {((Fl_Item*)l)->new_selected = v;}

int Fl_Tree_Browser::item_height(void *l) const {
  return ((Fl_Item *)l)->visible ? textsize()+2 : 0;
}

int Fl_Tree_Browser::incr_height() const {return textsize()+2;}

static Fl_Item* pushedtitle;

// Generate a descriptive text for this item, to put in browser & window titles
const char* Fl_Item::title() {
  const char* c = name(); if (c) return c;
  return type_name();
}

extern const char* subclassname(Fl_Item*);

void Fl_Tree_Browser::item_draw(void *v, int X, int Y, int, int) const {
  Fl_Item *l = (Fl_Item *)v;
  X += 3 + 18 + l->level * 12;
  if (l->new_selected) fl_color(fl_contrast(FL_BLACK,FL_SELECTION_COLOR));
  else fl_color(FL_BLACK);
  Fl_Pixmap *pm = NULL; //pixmap[l->pixmapID()];
  if (pm) pm->draw(X-18, Y);
  //if (l->is_public() == 0) lock_pixmap.draw(X - 17, Y);
  //else if (l->is_public() > 0) ; //unlock_pixmap.draw(X - 17, Y);
  if (l->is_parent()) {
    if (!l->next || l->next->level <= l->level) {
      if (l->open_!=(l==pushedtitle)) {
	fl_loop(X,Y+7,X+5,Y+12,X+10,Y+7);
      } else {
	fl_loop(X+2,Y+2,X+7,Y+7,X+2,Y+12);
      }
    } else {
      if (l->open_!=(l==pushedtitle)) {
	fl_polygon(X,Y+7,X+5,Y+12,X+10,Y+7);
      } else {
	fl_polygon(X+2,Y+2,X+7,Y+7,X+2,Y+12);
      }
    }
    X += 10;
  }
  if (l->is_widget() || l->is_class()) {
  //if (l->is_widget()) {
    const char* c = subclassname(l);
    if (!strncmp(c,"Fl_",3)) c += 3;
    fl_font(textfont(), textsize());
    fl_draw(c, X, Y+13);
    X += int(fl_width(c)+fl_width('n'));
    c = l->name();
    if (c) {
      fl_font(textfont()|FL_BOLD, textsize());
      fl_draw(c, X, Y+13);
    } else if ((c=l->label())) {
      char buf[50]; char* p = buf;
      *p++ = '"';
      for (int i = 20; i--;) {
	if (! (*c & -32)) break;
	*p++ = *c++;
      }
      if (*c) {strcpy(p,"..."); p+=3;}
      *p++ = '"';
      *p = 0;
      fl_draw(buf, X, Y+13);
    }
  } else {
    const char* c = l->title();
    char buf[60]; char* p = buf;
    for (int i = 55; i--;) {
      if (! (*c & -32)) break;
      *p++ = *c++;
    }
    if (*c) {strcpy(p,"..."); p+=3;}
    *p = 0;
    fl_font(textfont() | (l->is_code_block() && (l->level==0 || l->parent->is_class())?0:FL_BOLD), textsize());
    fl_draw(buf, X, Y+13);
  }
}

int Fl_Tree_Browser::item_width(void *v) const {
  Fl_Item *l = (Fl_Item *)v;

  if (!l->visible) return 0;

  int W = 3 + 16 + 18 + l->level*10;
  if (l->is_parent()) W += 10;

  if (l->is_widget() || l->is_class()) {
    const char* c = l->type_name();
    if (!strncmp(c,"Fl_",3)) c += 3;
    fl_font(textfont(), textsize());
    W += int(fl_width(c) + fl_width('n'));
    c = l->name();
    if (c) {
      fl_font(textfont()|FL_BOLD, textsize());
      W += int(fl_width(c));
    } else if ((c=l->label())) {
      char buf[50]; char* p = buf;
      *p++ = '"';
      for (int i = 20; i--;) {
	if (! (*c & -32)) break;
	*p++ = *c++;
      }
      if (*c) {strcpy(p,"..."); p+=3;}
      *p++ = '"';
      *p = 0;
      W += int(fl_width(buf));
    }
  } else {
    const char* c = l->title();
    char buf[60]; char* p = buf;
    for (int i = 55; i--;) {
      if (! (*c & -32)) break;
      *p++ = *c++;
    }
    if (*c) {strcpy(p,"..."); p+=3;}
    *p = 0;
    fl_font(textfont() | (l->is_code_block() && (l->level==0 || l->parent->is_class())?0:FL_BOLD), textsize());
    W += int(fl_width(buf));
  }

  return W;
}

void redraw_browser() {
  tree_browser->redraw();
}

void Fl_Tree_Browser::callback() {
  selection_changed((Fl_Item*)selection());
}

int Fl_Tree_Browser::handle(int e) {
  static Fl_Item *title;
  Fl_Item *l;
  int X,Y,W,H; bbox(X,Y,W,H);
  switch (e) {
  case FL_PUSH:
    if (!Fl::event_inside(X,Y,W,H)) break;
    l = (Fl_Item*)find_item(Fl::event_y());
    if (l) {
      X += 12*l->level + 18 - hposition();
      if (l->is_parent() && Fl::event_x()>X && Fl::event_x()<X+13) {
	title = pushedtitle = l;
	redraw_line(l);
	return 1;
      }
    }
    break;
  case FL_DRAG:
    if (!title) break;
    l = (Fl_Item*)find_item(Fl::event_y());
    if (l) {
      X += 12*l->level + 18 - hposition();
      if (l->is_parent() && Fl::event_x()>X && Fl::event_x()<X+13) ;
      else l = 0;
    }
    if (l != pushedtitle) {
      if (pushedtitle) redraw_line(pushedtitle);
      if (l) redraw_line(l);
      pushedtitle = l;
    }
    return 1;
  case FL_RELEASE:
    if (!title) {
      l = (Fl_Item*)find_item(Fl::event_y());
      if (l && l->new_selected && (Fl::event_clicks() || Fl::event_state(FL_CTRL)))
	l->open();
      break;
    }
    l = pushedtitle;
    title = pushedtitle = 0;
    if (l) {
      if (l->open_) {
	l->open_ = 0;
	for (Fl_Item*k = l->next; k&&k->level>l->level; k = k->next)
	  k->visible = 0;
      } else {
	l->open_ = 1;
	for (Fl_Item*k=l->next; k&&k->level>l->level;) {
	  k->visible = 1;
	  if (k->is_parent() && !k->open_) {
	    Fl_Item *j;
	    for (j = k->next; j && j->level>k->level; j = j->next);
	    k = j;
	  } else
	    k = k->next;
	}
      }
      redraw();
    }
    return 1;
  }
  return Fl_Browser_::handle(e);
}

Fl_Item::Fl_Item() {
  factory = 0;
  parent = 0;
  next = prev = 0;
  selected = new_selected = 0;
  visible = 0;
  name_ = 0;
  label_ = 0;
  user_data_ = 0;
  user_data_type_ = 0;
  callback_ = 0;
  rtti = 0;
  level = 0;
}

static void fixvisible(Fl_Item *p) {
  Fl_Item *t = p;
  for (;;) {
    if (t->parent) t->visible = t->parent->visible && t->parent->open_;
    else t->visible = 1;
    t = t->next;
    if (!t || t->level <= p->level) break;
  }
}

// turn a click at x,y on this into the actual picked object:
Fl_Item* Fl_Item::click_test(int,int) {return 0;}
void Fl_Item::add_child(Fl_Item*, Fl_Item*) {}
void Fl_Item::move_child(Fl_Item*, Fl_Item*) {}
void Fl_Item::remove_child(Fl_Item*) {}

// add a list of widgets as a new child of p:
void Fl_Item::add(Fl_Item *p) {
  if (p && parent == p) return;
  parent = p;
  Fl_Item *end = this;
  while (end->next) end = end->next;
  Fl_Item *q;
  int newlevel;
  if (p) {
    for (q = p->next; q && q->level > p->level; q = q->next);
    newlevel = p->level+1;
  } else {
    q = 0;
    newlevel = 0;
  }
  for (Fl_Item *t = this->next; t; t = t->next) t->level += (newlevel-level);
  //std::cerr << "add -> level = " << newlevel << std::endl;

  level = newlevel;
  if (q) {
    prev = q->prev;
    prev->next = this;
    q->prev = end;
    end->next = q;
  } else if (first) {
    prev = last;
    prev->next = this;
    end->next = 0;
    last = end;
  } else {
    first = this;
    last = end;
    prev = end->next = 0;
  }
  if (p) p->add_child(this,0);
  open_ = 1;
  fixvisible(this);
  //modflag = 1;
  tree_browser->redraw();
}

// add to a parent before another widget:
void Fl_Item::insert(Fl_Item *g) {
  Fl_Item *end = this;
  while (end->next) end = end->next;
  parent = g->parent;
  int newlevel = g->level;
  visible = g->visible;
  for (Fl_Item *t = this->next; t; t = t->next) t->level += newlevel-level;
  level = newlevel;
  prev = g->prev;
  if (prev) prev->next = this; else first = this;
  end->next = g;
  g->prev = end;
  fixvisible(this);
  if (parent) parent->add_child(this, g);
  tree_browser->redraw();
}

// Return message number for I18N...
int
Fl_Item::msgnum() {
  int		count;
  Fl_Item	*p;

  for (count = 0, p = this; p;) {
    if (p->label()) count ++;
    //if (p != this && p->is_widget() && ((Fl_Widget_Type *)p)->tooltip()) count ++;
    if (p != this && p->is_widget()) count ++;

    if (p->prev) p = p->prev;
    else p = p->parent;
  }

  return count;
}


// delete from parent:
Fl_Item *Fl_Item::remove() {
  Fl_Item *end = this;
  for (;;) {
    if (!end->next || end->next->level <= level) break;
    end = end->next;
  }
  if (prev) prev->next = end->next;
  else first = end->next;
  if (end->next) end->next->prev = prev;
  else last = prev;
  Fl_Item *r = end->next;
  prev = end->next = 0;
  if (parent) parent->remove_child(this);
  parent = 0;
  tree_browser->redraw();
  selection_changed(0);
  return r;
}

// update a string member:
int storestring(const char *n, const char * & p, int nostrip) {
  if (n == p) return 0;
  int length = 0;
  if (n) { // see if blank, strip leading & trailing blanks
    if (!nostrip) while (isspace(*n)) n++;
    const char *e = n + strlen(n);
    if (!nostrip) while (e > n && isspace(*(e-1))) e--;
    length = e-n;
    if (!length) n = 0;
  }    
  if (n == p) return 0;
  if (n && p && !strncmp(n,p,length) && !p[length]) return 0;
  if (p) free((void *)p);
  if (!n || !*n) {
    p = 0;
  } else {
    char *q = (char *)malloc(length+1);
    fltk_strlcpy(q,n,length+1);
    p = q;
  }
  //modflag = 1;
  return 1;
}

void Fl_Item::name(const char *n) {
  if (storestring(n,name_)) {
    if (visible) tree_browser->redraw();
  }
}

void Fl_Item::label(const char *n) {
  if (storestring(n,label_,1)) {
    setlabel(label_);
    if (visible && !name_) tree_browser->redraw();
  }
}

void Fl_Item::callback(const char *n) {
  storestring(n,callback_);
}

void Fl_Item::user_data(const char *n) {
  storestring(n,user_data_);
}

void Fl_Item::user_data_type(const char *n) {
  storestring(n,user_data_type_);
}

void Fl_Item::open() {
  printf("Open of '%s' is not yet implemented\n",type_name());
}

void Fl_Item::setlabel(const char *) {}

Fl_Item::~Fl_Item() {
  // warning: destructor only works for widgets that have been add()ed.
  if (tree_browser) tree_browser->deleting(this);
  if (prev) prev->next = next; else first = next;
  if (next) next->prev = prev; else last = prev;
  if (current == this) current = 0;
  //modflag = 1;
  if (parent) parent->remove_child(this);
}

int Fl_Item::is_parent() const {return 0;}
int Fl_Item::is_widget() const {return 0;}
int Fl_Item::is_valuator() const {return 0;}
int Fl_Item::is_button() const {return 0;}
int Fl_Item::is_menu_item() const {return 0;}
int Fl_Item::is_menu_button() const {return 0;}
int Fl_Item::is_group() const {return 0;}
int Fl_Item::is_window() const {return 0;}
int Fl_Item::is_code_block() const {return 0;}
int Fl_Item::is_decl_block() const {return 0;}
int Fl_Item::is_class() const {return 1;}
int Fl_Item::is_public() const {return 1;}
int Fl_Item::is_file() const {return 0;}

int Fl_File_Item::is_public()const { return 1; }
int Fl_Dir_Item::is_public()const { return 1; }
int Fl_File_Item::is_parent() const {return 0;}
int Fl_Dir_Item::is_parent() const {return 1;}

int Fl_Person_Item::is_public()const { return 1; }
int Fl_Person_Item::is_parent() const {return 1;}
int Fl_Msg_Item::is_public()const { return 1; }
int Fl_Msg_Item::is_parent() const {return 0;}


////////////////////////////////////////////////////////////////

Fl_Item *in_this_only; // set if menu popped-up in window

void select_all_cb(Fl_Widget *,void *) {
  Fl_Item *p = Fl_Item::current ? Fl_Item::current->parent : 0;
  if (in_this_only) {
    Fl_Item *t = p;
    for (; t && t != in_this_only; t = t->parent);
    if (t != in_this_only) p = in_this_only;
  }
  for (;;) {
    if (p) {
      int foundany = 0;
      for (Fl_Item *t = p->next; t && t->level>p->level; t = t->next) {
	if (!t->new_selected) {tree_browser->select(t,1,0); foundany = 1;}
      }
      if (foundany) break;
      p = p->parent;
    } else {
      for (Fl_Item *t = Fl_Item::first; t; t = t->next)
	tree_browser->select(t,1,0);
      break;
    }
  }
  selection_changed(p);
}

// rmfern - removed static....
void delete_children(Fl_Item *p) {
  Fl_Item *f;
  for (f = p; f && f->next && f->next->level > p->level; f = f->next);
  for (; f != p; ) {
    Fl_Item *g = f->prev;
    delete f;
    f = g;
  }
}

void delete_all(int selected_only) {
  for (Fl_Item *f = Fl_Item::first; f;) {
    if (f->selected || !selected_only) {
      delete_children(f);
      Fl_Item *g = f->next;
      delete f;
      f = g;
    } else f = f->next;
  }
  //if(!selected_only)    include_H_from_C=1;

  selection_changed(0);
}

// move f (and it's children) into list before g:
// returns pointer to whatever is after f & children
void Fl_Item::move_before(Fl_Item* g) {
  if (level != g->level) printf("move_before levels don't match! %d %d\n",
				level, g->level);
  Fl_Item* n;
  for (n = next; n && n->level > level; n = n->next);
  if (n == g) return;
  Fl_Item *l = n ? n->prev : Fl_Item::last;
  prev->next = n;
  if (n) n->prev = prev; else Fl_Item::last = prev;
  prev = g->prev;
  l->next = g;
  if (prev) prev->next = this; else Fl_Item::first = this;
  g->prev = l;
  if (parent) parent->move_child(this,g);
  tree_browser->redraw();
}

// move selected widgets in their parent's list:
void earlier_cb(Fl_Widget*,void*) {
  Fl_Item *f;
  for (f = Fl_Item::first; f; ) {
    Fl_Item* nxt = f->next;
    if (f->selected) {
      Fl_Item* g;
      for (g = f->prev; g && g->level > f->level; g = g->prev);
      if (g && g->level == f->level && !g->selected) f->move_before(g);
    }
    f = nxt;
  }
}

void later_cb(Fl_Widget*,void*) {
  Fl_Item *f;
  for (f = Fl_Item::last; f; ) {
    Fl_Item* prv = f->prev;
    if (f->selected) {
      Fl_Item* g;
      for (g = f->next; g && g->level > f->level; g = g->next);
      if (g && g->level == f->level && !g->selected) g->move_before(f);
    }
    f = prv;
  }
}

// FROM Fl_Widget_Type.cxx

// Called when ui changes what objects are selected:
// p is selected object, null for all deletions (we must throw away
// old panel in that case, as the object may no longer exist)
void selection_changed(Fl_Item *p) {
  // store all changes to the current selected objects:
  //if (p && the_panel && the_panel->visible()) {
  if (p) {
      //set_cb(0,0);
      // if there was an error, we try to leave the selected set unchanged:
//      if (haderror) {
//         Fl_Item *q = 0;
//         for (Fl_Item *o = Fl_Item::first; o; o = o->next) {
//              o->new_selected = o->selected;
//              if (!q && o->selected) q = o;
//         }
//         if (!p || !p->selected) p = q;
//         Fl_Item::current = p;
//         redraw_browser();
//         return;
//      }
  }
  // update the selected flags to new set:
  Fl_Item *q = 0;
  for (Fl_Item *o = Fl_Item::first; o; o = o->next) {
      o->selected = o->new_selected;
      if (!q && o->selected) q = o;
  }
  if (!p || !p->selected) p = q;
      Fl_Item::current = p;
}


/*
 * 'fltk_strlcpy()' - Safely copy two strings. (BSD style)
 * Taken from fltk/src/flstring.c
 */

size_t                          /* O - Length of string */
fltk_strlcpy(char     *dst,     /* O - Destination string */
	   const char *src,     /* I - Source string */
	   size_t      size) {  /* I - Size of destination string buffer */
  size_t        srclen;         /* Length of source string */


   /*
    * Figure out how much room is needed...
    */

    size --;

    srclen = strlen(src);

   /*
    * Copy the appropriate amount...
    */

    if (srclen > size) srclen = size;

    memcpy(dst, src, srclen);
    dst[srclen] = '\0';

    return (srclen);
}

size_t                          /* O - Length of string */
fltk_strlcat(char       *dst,     /* O - Destination string */
	  const char *src,     /* I - Source string */
  	size_t     size) {   /* I - Size of destination string buffer */
  size_t        srclen;         /* Length of source string */
  size_t        dstlen;         /* Length of destination string */
  
  
  /*
  *   * Figure out how much room is left...
  *     */
  
  dstlen = strlen(dst);
  size   -= dstlen + 1;
  
  if (!size) return (dstlen);   /* No room, return immediately... */
  
  /*
  *   * Figure out how much room is needed...
  *     */
  
  srclen = strlen(src);
  
  /*
  *   * Copy the appropriate amount...
  *     */
  
  if (srclen > size) srclen = size;
  
  memcpy(dst + dstlen, src, srclen);
  dst[dstlen + srclen] = '\0';
  
  return (dstlen + srclen);
}
  
//
// End of "$Id: Fl_Tree_Browser.cc,v 1.3 2007-02-18 21:46:49 rmf24 Exp $".
//

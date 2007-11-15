//
// "$Id: Fl_Tree_Browser.h,v 1.2 2007-02-18 21:46:49 rmf24 Exp $"
//
// This is hacked up FLTK code, and so is released under 
// their licence. (below)
// Copyright 2004-2006 by Robert Fernie.
//
// Please report all bugs and problems to "retroshare@lunamutt.com".
//
///////////////////////////////////////////////////////////////////
//
// Widget type header file for the Fast Light Tool Kit (FLTK).
//
// Each object described by Fluid is one of these objects.  They
// are all stored in a double-linked list.
//
// There is also a single "factory" instance of each type of this.
// The method "make()" is called on this factory to create a new
// instance of this object.  It could also have a "copy()" function,
// but it was easier to implement this by using the file read/write
// that is needed to save the setup anyways.
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

#include <FL/Fl_Widget.H>
#include <FL/Fl_Menu.H>
#include <stdlib.h>
#include <string>

size_t  fltk_strlcpy(char *dst, const char *src, size_t size);
size_t  fltk_strlcat(char *dst, const char *src, size_t size);

Fl_Widget *make_tree_browser(int,int,int,int,const char *l=0);
void redraw_browser();
void delete_all(int selected_only); 

class Fl_Item {

  friend class Tree_Browser;
  friend Fl_Widget *make_tree_browser(int,int,int,int,const char *l);
  friend class Fl_Window_Type;
  virtual void setlabel(const char *); // virtual part of label(char*)

protected:

  Fl_Item();

  const char *name_;
  const char *label_;
  const char *callback_;
  const char *user_data_;
  const char *user_data_type_;

public:	// things that should not be public:

  Fl_Item *parent; // parent, which is previous in list
  char new_selected; // browser highlight
  char selected; // copied here by selection_changed()
  char open_;	// state of triangle in browser
  char visible; // true if all parents are open
  char rtti;	// hack because I have no rtti, this is 0 for base class
  int level;	// number of parents over this
  static Fl_Item *first, *last; // linked list of all objects
  Fl_Item *next, *prev;	// linked list of all objects

  Fl_Item *factory;
  const char *callback_name();

public:

  virtual ~Fl_Item();
  virtual Fl_Item *make(Fl_Item *p) = 0;

  void add(Fl_Item *parent); // add as new child
  void insert(Fl_Item *n); // insert into list before n
  Fl_Item* remove();	// remove from list
  void move_before(Fl_Item*); // move before a sibling

  virtual const char *title(); // string for browser
  virtual const char *type_name() = 0; // type for code output

  const char *name() const {return name_;}
  void name(const char *);
  const char *label() const {return label_;}
  void label(const char *);
  const char *callback() const {return callback_;}
  void callback(const char *);
  const char *user_data() const {return user_data_;}
  void user_data(const char *);
  const char *user_data_type() const {return user_data_type_;}
  void user_data_type(const char *);

  virtual Fl_Item* click_test(int,int);
  virtual void add_child(Fl_Item*, Fl_Item* beforethis);
  virtual void move_child(Fl_Item*, Fl_Item* beforethis);
  virtual void remove_child(Fl_Item*);

  static Fl_Item *current;  // most recently picked object
  virtual void open();	// what happens when you double-click

  // get message number for I18N
  int msgnum();

  // fake rtti:
  virtual int is_parent() const;
  virtual int is_widget() const;
  virtual int is_button() const;
  virtual int is_valuator() const;
  virtual int is_menu_item() const;
  virtual int is_menu_button() const;
  virtual int is_public() const;
  virtual int is_group() const;
  virtual int is_window() const;
  virtual int is_code_block() const;
  virtual int is_decl_block() const;
  virtual int is_class() const;
  virtual int is_file() const;


  virtual int pixmapID() { return 0; }

  const char* class_name(const int need_nest) const;
};

class Fl_Dir_Item : public Fl_Item {
public:
  Fl_Item *make(Fl_Item *p);
  void open();
  virtual const char *type_name() {return "dir";}
  int is_file() const {return 0;}
  int is_parent() const; /* {return 1;} */
  int pixmapID() { return 8; }
  virtual int is_public() const;
};

extern Fl_Dir_Item Fl_Dir_Item_type;

class Fl_File_Item : public Fl_Item {
public:
  int size;
  std::string filename;

  Fl_Item *make(Fl_Item *p);
  void open();
  virtual const char *type_name() {return "file";}
  int is_file() const {return 1;}
  int is_parent() const; /* {return 0;} */
  virtual int is_public() const;
  int pixmapID() { return 9; }
};

extern Fl_File_Item Fl_File_Item_type;


class Fl_Person_Item : public Fl_Item {
public:
  Fl_Item *make(Fl_Item *p);
  void open();
  virtual const char *type_name() {return "Person:";}
  int is_file() const {return 0;}
  int is_parent() const; /* {return 1;} */
  virtual int is_public() const;
  int pixmapID() { return 1; }
  std::string person_hash;
};

extern Fl_Person_Item Fl_Person_Item_type;

class Fl_Msg_Item : public Fl_Item {
public:
  Fl_Item *make(Fl_Item *p);
  void open();
  virtual const char *type_name() {return "msg";}
  int is_file() const {return 0;}
  int is_parent() const; /* {return 1;} */
  virtual int is_public() const;
  int pixmapID() { return 2; }
};

extern Fl_Msg_Item Fl_Msg_Item_type;


// bonus helper function.
void delete_children(Fl_Item *p);



// replace a string pointer with new value, strips leading/trailing blanks:
int storestring(const char *n, const char * & p, int nostrip=0);

//
// End of "$Id: Fl_Tree_Browser.h,v 1.2 2007-02-18 21:46:49 rmf24 Exp $".
//

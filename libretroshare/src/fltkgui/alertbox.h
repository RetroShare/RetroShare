/*
 * "$Id: alertbox.h,v 1.4 2007-02-18 21:46:49 rmf24 Exp $"
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



#ifndef RETROSHARE_ALERT_H
#define RETROSHARE_ALERT_H

#include <string>
#include <deque>

#include <FL/Fl_Window.H>
#include <FL/Fl_Text_Display.H>

class alertMsg
{
	public:

	int epoch;
	int type;
	int severity;
	std::string msg;
	std::string source;
};

class alertBox
{
	public:
	alertBox(Fl_Window *w, Fl_Text_Display *box);
virtual	~alertBox();

int	sendMsg(int type, int severity, 
		std::string msg, std::string source);

	protected:
	int     loadInitMsg();
virtual void	displayMsgs();
virtual int     showMsgs(int severity);

	int showThreshold; // At what severity we show ourselves.
	Fl_Window *alert_win;
	Fl_Text_Display *alert_box;
	int maxMessages;

	std::deque<alertMsg> msgs;
};

class chatterBox: public alertBox
{
	public:
	chatterBox(Fl_Window *w, Fl_Text_Display *box)
	:alertBox(w, box), firstMsg(true) 
	{ 
		chatterBox::loadInitMsg();
		return; 
	}

	protected:
	int     loadInitMsg();
virtual void	displayMsgs();
virtual int     showMsgs(int severity);

	private:
	bool firstMsg;

};


#endif

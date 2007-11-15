/*
 * "$Id: alertbox.cc,v 1.3 2007-02-18 21:46:49 rmf24 Exp $"
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




#include "fltkgui/pqistrings.h"
#include "fltkgui/alertbox.h"

#include <sstream>

alertBox::alertBox(Fl_Window *w, Fl_Text_Display *box)
	:showThreshold(2), alert_win(w), alert_box(box), 
	maxMessages(40)
	{
		/* can't call virtual fn here! */
		alertBox::loadInitMsg();
		return;
	}

alertBox::~alertBox()
{
	/* clean up msgs */
	return;
}

int	alertBox::loadInitMsg()
{
	sendMsg(0, 10,"Welcome to RetroShare ----", "");
	return 1;
}

int	chatterBox::loadInitMsg()
{
	sendMsg(0, 10,"", "");
	sendMsg(0, 10,"\tThis is your Universal ChatterBox Window", "");
	sendMsg(0, 10,"", "");
	sendMsg(0, 10,"\tYou can use it to chat with everyone", "");
	sendMsg(0, 10,"\tthat is online at this moment", "");

	sendMsg(0, 10,"", "");
	sendMsg(0, 10,"\tSometimes the conversations can seem", "");
	sendMsg(0, 10,"\tweird and wonderful, if you're not", "");
	sendMsg(0, 10,"\tconnected all the same people.", "");
	sendMsg(0, 10,"", "");
	sendMsg(0, 10,"\tIt is a bit of an experiment, so", "");
	sendMsg(0, 10,"\tlet me know if you enjoy it or hate it!", "");
	sendMsg(0, 10,"", "");
	sendMsg(0, 10,"Note. Your ChatterBox only pop up once, when the", "");
	sendMsg(0, 10,"first message is received. After that it'll leave", "");
	sendMsg(0, 10,"you in peace. Use the \"Chat\" Button on the main", "");
	sendMsg(0, 10,"window to open and close your ChatterBox", "");
	sendMsg(0, 10,"", "");
	sendMsg(0, 10,"", "");
	return 1;
}


int	alertBox::sendMsg(int type, int severity, 
			std::string msg, std::string source)
{
	alertMsg newal;

	newal.epoch = time(NULL);
	newal.type = type;
	newal.severity = severity;
	newal.msg = msg;
	newal.source = source;

	msgs.push_front(newal);
	if (msgs.size() > (unsigned) maxMessages)
	{
		msgs.pop_back();
	}
	displayMsgs();
	showMsgs(severity);
	return 1;
}


int	alertBox::showMsgs(int severity)
{
	if (severity <= showThreshold)
	{
		alert_win -> show();
		return 1;
	}
	return 0;
}

/* chatterbox will only pop up for the first chat */
int	chatterBox::showMsgs(int severity)
{
	if ((firstMsg) && (alertBox::showMsgs(severity)))
	{
		firstMsg = false;
		return 1;
	}
	return 0;
}

void	alertBox::displayMsgs()
{
	std::ostringstream msg;

	std::deque<alertMsg>::reverse_iterator it;
	for(it = msgs.rbegin(); it != msgs.rend(); it++)
	{
		if (it -> severity <= showThreshold)
		{
		  msg << "----------------->>>> ALERT: ";
	 	  msg << timeFormat(it -> epoch, TIME_FORMAT_NORMAL);	
		  msg << std::endl;
		  msg << std::endl;

		  msg << it -> msg << std::endl;
		  msg << std::endl;
		  msg << "<<<<----------------";
		  msg << std::endl;
		  msg << std::endl;
		}
		else
		{
	 	  msg << timeFormat(it -> epoch, TIME_FORMAT_NORMAL);	
		  msg << ":" << it -> msg << std::endl;
		  msg << std::endl;
		}
	}
	Fl_Text_Buffer *buf = alert_box -> buffer();
	buf -> text(msg.str().c_str());
	int c = buf -> length();
	int lines = buf -> count_lines(0, c-1);
	// want to scroll to the bottom.
	alert_box -> scroll(lines, 0);
}


void	chatterBox::displayMsgs()
{
	std::ostringstream msg;

	std::deque<alertMsg>::reverse_iterator it;
	std::string lsrc = "";
	int lts = 0;
	for(it = msgs.rbegin(); it != msgs.rend(); it++)
	{
		if ((it->epoch-lts > 30) || // more than 30 seconds.
		    (it->source != lsrc))    // not same source
		{
		  msg << "\t\t\t\t\t\t[";
		  msg << it->source;
		  msg << " @ ";
	 	  msg << timeFormat(it -> epoch, TIME_FORMAT_NORMAL);	
		  msg << "]";
		  msg << std::endl;
		}

		msg << it -> msg;
		msg << std::endl;

		/* store last one */
		lts  = it->epoch;
		lsrc = it->source;
	}

	Fl_Text_Buffer *buf = alert_box -> buffer();
	buf -> text(msg.str().c_str());
	int c = buf -> length();
	int lines = buf -> count_lines(0, c-1);
	// want to scroll to the bottom.
	alert_box -> scroll(lines, 0);
}




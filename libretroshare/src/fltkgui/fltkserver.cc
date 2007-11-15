/*
 * "$Id: fltkserver.cc,v 1.29 2007-02-18 21:46:49 rmf24 Exp $"
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




#include "fltkgui/fltkserver.h"
#include "fltkgui/pqibrowseitem.h"

#include <iostream>
#include <sstream>

#include "pqi/pqidebug.h"
const int fltksrvrzone = 25915;

#include <sys/time.h>
#include <time.h>

/* add in an extension if necessary */
int ensureExtension(std::string &name, std::string def_ext);

fltkserver::fltkserver()
	:server(NULL), pqih(NULL), sslr(NULL), ui(NULL), ad(NULL), 
	msg(NULL), chat(NULL),loop(0),
	search_browser(NULL)
{
	recommend = NULL;


	init();
	return;
}

fltkserver::~fltkserver()
{
	return;
}

int fltkserver::init()
{
	cert_list_ok = false;
	cert_neighbour_list_ok = false;
	cert_item_ok = false;
	cert_neighbour_item_ok = false;

	search_list_ok = false;
	result_list_ok = false;
	result_item_ok = false;
	msg_list_ok = false;
	msg_item_ok = false;
	msg_dialog_ok = false;
	transfer_ok = false;
	transfer_item_ok = false;

	dir_list_ok = false;
	save_dir_ok = false;

	server_set_ok = false;
	transfer_rates_ok = false;


#ifdef PQI_USE_CHANNELS
	channel_list_ok = false;
	channel_msg_list_ok = false;
	channel_msg_item_ok = false;
#endif



	return 1;
}

int	fltkserver::run()
{

    double timeDelta = 0.25;
    double minTimeDelta = 0.1; // 25;
    double maxTimeDelta = 2.0;
    double kickLimit = 0.5;

    double avgTickRate = timeDelta;

    struct timeval lastts, ts;
    gettimeofday(&lastts, NULL);
    long   lastSec = 0; /* for the slower ticked stuff */

    int min = 0;

    while(1)
    {
	Fl::wait(timeDelta);

    	gettimeofday(&ts, NULL);
	double delta = (ts.tv_sec - lastts.tv_sec) + 
			((double) ts.tv_usec - lastts.tv_usec) / 1000000.0;

	/* for the fast ticked stuff */
	if (delta > timeDelta) 
	{
		//std::cerr << "Delta: " << delta << std::endl;
		//std::cerr << "Time Delta: " << timeDelta << std::endl;
		//std::cerr << "Avg Tick Rate: " << avgTickRate << std::endl;

		lastts = ts;
		int moreToTick = server -> tick();

		/* adjust tick rate depending on whether there is more.
		 */

		avgTickRate = 0.2 * timeDelta + 0.8 * avgTickRate;

		if (1 == moreToTick)
		{
			timeDelta = 0.9 * avgTickRate;
			if (timeDelta > kickLimit)
			{
				/* force next tick in one sec
				 * if we are reading data.
				 */
				timeDelta = kickLimit;
				avgTickRate = kickLimit;
			}
		}
		else
		{
			timeDelta = 1.1 * avgTickRate;
		}

		/* limiter */
		if (timeDelta < minTimeDelta)
		{
			timeDelta = minTimeDelta;
		}
		else if (timeDelta > maxTimeDelta)
		{
			timeDelta = maxTimeDelta;
		}

		/* now we have the slow ticking stuff */
		/* stuff ticked once a second (but can be slowed down) */
		if (ts.tv_sec > lastSec)
		{
			lastSec = ts.tv_sec;
	
			// every ten loops (> 10 secs)
			if (loop % 10 == 0)
			{
				update_quick_stats();
			}
	
			// every 60 loops (> 1 min)
			if (++loop >= 60)
			{
				loop = 0;
	
				update_other_stats();
				server -> status();
	
				// save the config every 5 minutes.
				if (min % 5 == 0)
				{
					cert_save_config();
#ifdef PQI_USE_CHANNELS
					/* hack to update for now 
					 * Only occassionally - cos disabled
					 */
					channel_list_ok = false;
					update_channels();
#endif
				}
	
				/* hour loop */
				if (++min >= 60)
				{
					min = 0;
				}
				update_dirlist();
			}

			// slow update tick as well.
			update();
		} // end of slow tick.

	} // end of only once a second.

	// update graphics.. ( but only if gui is visible )
	// This is also triggered in slow tick...
	if ((ui->main_win->shown()) || (ui->chatter_window->shown()))
	{
		update();
	}
     }
	return 1;
}



/*********************** DISPLAY DETAILS ************************************/

int	fltkserver::update()
{
	update_certs();
	update_neighbour_certs();

	update_search();
	update_msgs();
	update_transfer();
	update_config();
	update_dirs();

	return 1;
}


int	fltkserver::update_quick_stats()
{
	cert_list_ok = false;
	cert_neighbour_list_ok = false;
	transfer_ok = false;
	transfer_item_ok = false;

	return 1;
}

int	fltkserver::update_other_stats()
{
	search_list_ok = false;
	result_list_ok = false;
	result_item_ok = false;
	msg_list_ok = false;
	msg_item_ok = false;
	msg_dialog_ok = false;

	//transfer_ok = false;
	//server_set_ok = false;
	//transfer_rates_ok = false;

	return 1;
}

// Now including Funky_Browser.
int	fltkserver::update_certs()
{
	// to draw certs, we need to retrieve the list from sslroot.
	if (sslr == NULL)
		return -1;

	// once this is true, we do all following redraw steps.
	bool certs_mod = false;
	int i;

	Fl_Funky_Browser *cb = ui -> cert_list;
	// Get the Current Cert, by getting current item 
	// and extracting ref.
	DisplayData *selected = cb -> getSelected();
	Person *selperson = NULL;
	if (selected != NULL)
	{
		selperson = ((PersonDisItem *) selected) -> getItem();
	}

	//int inum = cb -> value();

	// attempt to reset item.

	// only do if 
	if ((sslr -> CertsChanged()) || (!cert_list_ok))
	{
		certs_mod = true;

		if (sslr -> CertsMajorChanged()) // add or remove.
		{
			// full repopulate.
			// or add/remove. (do later)
			
			// clear the data.
			cb  -> clear();
			ui -> msg_online -> clear();
	
			std::list<cert *>::iterator it;
			std::list<cert *> &certs = sslr -> getCertList();
	
			std::string emptystr("");
			int online = 0;

			selected = NULL;
			for(it = certs.begin(), i = 0; 
					it != certs.end(); it++, i++)
			{
				cert *c = (*it);
	
				PersonDisItem *cdi = new PersonDisItem(c);

				if (selperson == c)
					selected = cdi;

				// set the check flag.
				if (c -> Manual())
					cdi -> check(0, 0);
				else
					cdi -> check(0, 1);
	
				if (c -> Connected())
					online++;
	
				cb -> addItemSeries(cdi);
	
				//ui -> msg_online -> add((*it).c_str(), 
				//		c -> Group("msg"));
			}
			cb -> ItemSeriesDone();
			ui -> onlinecounter -> value(online);
		}
		else
		{
			// else just update list.
			cb -> updateList();
		}

		cb -> selectDD(selected);
		cert_list_ok = true;
	}


	if ((certs_mod) || (!cert_item_ok))
	{
		PersonDisItem *pdi = (PersonDisItem *) cb -> getCurrentItem();
		Person *p = NULL;
		if (pdi != NULL)
		{
			p = pdi -> getItem();
		}
		// do always - neighbours does it afterwards.
		cert_display((cert *) p, true);
	}
	return 1;
}

int fltkserver::cert_display(cert *c, bool isfriend)
{
	Fl_Text_Buffer *buf = NULL;
	std::string certtext;

	if ((c == NULL) || (c->certificate == NULL))
	{
		ui -> cert_status -> value("N/A");

		ui -> cert_server -> value("None");
		ui -> cert_port -> value(0);

		ui -> cert_authcode -> value("");
	  	ui ->cert_authcode->deactivate();
	  	ui ->cert_sign_button->deactivate();

		buf = ui -> cert_details -> buffer();
		buf -> text(certtext.c_str());

		ui -> cert_allow -> value(0);
		ui -> cert_listen -> value(0);
		ui -> cert_connect -> value(0);

		cert_item_ok = false;
		return 0;
	}

	int status = c -> Status();
	if (!isfriend)
	{
		status = 0;
	}

	ui -> cert_status -> value((get_status_string(status)).c_str());

	if (isfriend)
	{
	  ui -> cert_status -> value((get_status_string(status)).c_str());
	}
	else
	{
	  ui -> cert_status -> value("Neighbour");
	}

	// SET SERVER.
	if (!cert_item_ok)
	{
	  ui->cert_server->value(inet_ntoa(c->serveraddr.sin_addr));
	  ui ->cert_port->value(ntohs(c->serveraddr.sin_port));

	  /* check if we have signed the certificate */
	  if (0 < validateCertificateIsSignedByKey(c->certificate, 
			sslr -> getOwnCert() -> certificate))

	  {
	  	ui->cert_authcode->value(
			getXPGPAuthCode(c->certificate).c_str());
	  	ui ->cert_authcode->deactivate();
	  	ui ->cert_sign_button->deactivate();
	  }
	  else
	  {
	  	ui ->cert_authcode->value("");
	  	ui ->cert_authcode->activate();
	  	ui ->cert_sign_button->deactivate();
	  }
	}

	if (c -> Manual() || (!isfriend))
		ui -> cert_auto -> value(0);
	else
		ui -> cert_auto -> value(1);

	// Finally Fill in the Text Editor.
	certtext = get_cert_info(c);

	// Last Connection 
	// Server Address
	// Status
	// Hashes

	buf = ui -> cert_details -> buffer();
	buf -> text(certtext.c_str());


	if (status & PERSON_STATUS_ACCEPTED)
		ui -> cert_allow -> value(1);
	else
		ui -> cert_allow -> value(0);

	if (status & PERSON_STATUS_WILL_LISTEN)
		ui -> cert_listen -> value(1);
	else
		ui -> cert_listen -> value(0);

	if (status & PERSON_STATUS_WILL_CONNECT)
		ui -> cert_connect -> value(1);
	else
		ui -> cert_connect -> value(0);

	if (status & PERSON_STATUS_TRUSTED)
	{
		ui -> cert_trust_person -> value(1);
	}
	else
	{
		ui -> cert_trust_person -> value(0);
	}


	cert_item_ok = true;
	return 1;
}


int	fltkserver::cert_update_auth()
{

	cert *c = cert_get_current();
	if ((c == NULL) || (c->certificate == NULL))
	{
		ui -> cert_authcode -> value("");
	  	ui ->cert_authcode->deactivate();
	  	ui ->cert_sign_button->deactivate();
		return 0;
	}

	std::string inp_code = ui->cert_authcode->value();
	std::string real_code = getXPGPAuthCode(c->certificate);
	if (inp_code == real_code)
	{
		ui ->cert_sign_button->activate();
	}
	else
	{
		ui ->cert_sign_button->deactivate();
	}
	return 1;
}




int	fltkserver::update_neighbour_certs()
{

	// to draw certs, we need to retrieve the list from AutoDiscovery.
	if (ad == NULL)
		return -1;


	// once this is true, we do all following redraw steps.
	bool certs_mod = false;
	int i;

	Fl_Funky_Browser *cb = ui -> cert_neighbour_list;


	// only do if 
	//if ((sslr -> CertsChanged()) || (!cert_list_ok))
	if (!cert_neighbour_list_ok)
	{
		certs_mod = true;

		DisplayData *selected = cb -> getCurrentItem();
		Person *selperson = NULL;
		if (selected != NULL)
		{
			selperson = ((NeighDisItem *) selected) -> getItem();
			std::cerr << "Currently Selected is: ";
			std::cerr << selperson -> Name();
			std::cerr << std::endl;
		}
		else
		{
			std::cerr << "Nothing Selected";
			std::cerr << std::endl;
		}

		std::cerr << "Updating Neighbour List";
		std::cerr << std::endl;

		// if (sslr -> CertsMajorChanged()) // add or remove.
		{
			// full repopulate.
			cb  -> clear();
			std::list<cert *>::iterator it;
			std::list<cert *> &certs = ad -> getDiscovered();
	
			std::string emptystr("");

			selected = NULL;
			for(it = certs.begin(), i = 0; 
					it != certs.end(); it++, i++)
			{
				cert *c = (*it);
				NeighDisItem *cdi = new NeighDisItem(c);

				if (selperson == c)
				{
					std::cerr << "Matched Selected: ";
					std::cerr << c -> Name();
					std::cerr << std::endl;

					selected = cdi;
				}

				cb -> addItemSeries(cdi);
			}
			cb -> ItemSeriesDone();
		}
		//else
		//{
		//	// else just update list.
		//	cb -> updateList();
		//}

		cb -> selectDD(selected);
		cert_neighbour_list_ok = true;
	}


	if ((certs_mod) || (!cert_neighbour_item_ok))
	{
		NeighDisItem *pdi = (NeighDisItem *) cb -> getCurrentItem();
		Person *p = NULL;
		if (pdi != NULL)
		{
			p = pdi -> getItem();
		}
		// do always - neighbours does it afterwards.
		neigh_display((cert *) p);
	}
	return 1;
}

int fltkserver::neigh_display(cert *c)
{
	ui -> neigh_signers -> clear();
	if ((c == NULL) || (c->certificate == NULL))
	{
		ui -> neigh_name    -> value("N/A");
		ui -> neigh_org     -> value("N/A");
		ui -> neigh_loc     -> value("N/A");
		ui -> neigh_country -> value("N/A");
		ui -> neigh_trust   -> value("N/A");
		ui -> neigh_auth_notice -> hide();
		ui -> neigh_authcode    -> value("");
	  	ui -> neigh_authcode    ->deactivate();
	  	ui -> neigh_add_button  ->deactivate();

		cert_neighbour_item_ok = true;
		return 0;
	}

	/* else we need to populate it properly
	 *
	 */
	ui -> neigh_name    -> value((get_cert_name(c)).c_str());
	ui -> neigh_org     -> value((get_cert_org(c)).c_str());
	ui -> neigh_loc     -> value((get_cert_loc(c)).c_str());
	ui -> neigh_country -> value((get_cert_country(c)).c_str());

	std::list<std::string>::iterator it;
	std::list<std::string> signers = getXPGPsigners(c->certificate);
	for(it = signers.begin(); it != signers.end(); it++)
	{
		ui -> neigh_signers -> add(it->c_str());
		std::cerr << *it << std::endl;
	}

	if (c->trustLvl < TRUST_SIGN_AUTHEN)
	{
		sslr -> checkAuthCertificate(c);
	}

	/* calculate trust level! */
	ui -> neigh_trust -> value((get_trust_string(c)).c_str());
	if (!cert_neighbour_item_ok)
	{
	  /* check if we have signed the certificate */
	  if (0 < validateCertificateIsSignedByKey(c->certificate, 
			sslr -> getOwnCert() -> certificate))

	  {
	  	ui -> neigh_authcode->value(
			getXPGPAuthCode(c->certificate).c_str());
	  	ui -> neigh_authcode->deactivate();
		ui -> neigh_auth_notice -> hide();
		if ((c->Accepted()) || (c == sslr->getOwnCert()))
		{
	  		ui -> neigh_add_button->deactivate(); 
		}
		else
		{
	  		ui -> neigh_add_button->activate(); /* can add! */
		}
	  }
	  else
	  {
		/* only reset auth code if it was inactive */
		if (!ui -> neigh_authcode -> active())
		{
		  ui -> neigh_authcode    -> value("");
		}
		if (c->trustLvl < TRUST_SIGN_AUTHEN)
		{
			/* need auth code to add */
			ui -> neigh_authcode    ->activate();
			//ui -> neigh_auth_notice -> show();
			//ui -> neigh_add_button  ->deactivate();
			//handled in update_auth.
			neigh_update_auth();
		}
		else
		{
			/* can add without auth code */
			ui -> neigh_auth_notice -> hide();
			ui -> neigh_authcode    ->activate();
			ui -> neigh_add_button  ->activate();
		}
	  }
	}


	/* TODO:
	 * 1) signer list
	 * 2) trust level
	 */

	cert_neighbour_item_ok = true;
	return 1;
}


int	fltkserver::neigh_update_auth()
{

	std::cerr << "fltkserver::neigh_update_auth() called";
	std::cerr << std::endl;

	cert *c = cert_get_current_neighbour();
	if ((c == NULL) || (c->certificate == NULL))
	{
		std::cerr << "fltkserver::neigh_update_auth() found NULL";
		std::cerr << std::endl;

		ui -> neigh_authcode -> value("");
	  	ui -> neigh_authcode->deactivate();
	  	ui -> neigh_add_button->deactivate();
	  	ui -> neigh_auth_notice -> hide();
		return 0;
	}

	std::string inp_code = ui->neigh_authcode->value();
	std::string real_code = getXPGPAuthCode(c->certificate);
	if (inp_code == real_code)
	{
		std::cerr << "fltkserver::neigh_update_auth() ANS!";
		std::cerr << std::endl;
	  	ui -> neigh_auth_notice -> hide();
		ui ->neigh_add_button->activate();
	}
	else
	{
		std::cerr << "fltkserver::neigh_update_auth() NC";
		std::cerr << std::endl;
	  	ui -> neigh_auth_notice -> show();
		ui -> neigh_add_button->deactivate();
	}
	return 1;
}

int fltkserver::update_msgs()
{
	if (server == NULL)
		return -1;

	// draw the Messages.

	Fl_Funky_Browser *mb = ui -> msg_list;
	bool msg_changed = false;
	std::list<MsgItem *>::iterator mit;

	DisplayData *selected = mb -> getSelected();
	MsgItem *selitem;
	if (selected != NULL)
	{
		selitem = ((MsgDisItem *) selected) -> getItem();
	}

	if ((server -> msgChanged.Changed(0)) || (!msg_list_ok))
	{
		if (server->msgMajorChanged.Changed(0))
		{
	        	pqioutput(PQL_DEBUG_BASIC, fltksrvrzone,
	 			"fltkserver::update_msgs() (server->searchMajorChanged.Changed(0))");
	
			std::list<MsgItem *> &msgs = server -> getMsgList();
			mb -> clear();
			for(mit = msgs.begin(); mit != msgs.end(); mit++)
			{
				MsgDisItem *mdi = new MsgDisItem(*mit);
				mb -> addItemSeries(mdi);
			}
		}
		else
		{
			std::list<MsgItem *> msgs = server -> getNewMsgs();
			for(mit = msgs.begin(); mit != msgs.end(); mit++)
			{
				MsgDisItem *mdi = new MsgDisItem(*mit);
				mb -> addItemSeries(mdi);
			}
		}

		mb -> ItemSeriesDone();
		mb -> selectDD(selected);
		msg_list_ok = true;
		msg_changed = true;
	}

	if ((msg_changed) || (!msg_item_ok))
	{
		Fl_Text_Buffer *buf = ui -> msg_details -> buffer();

		MsgDisItem *mdi = (MsgDisItem *) mb -> getCurrentItem();
		MsgItem *mi = NULL;

		if (mdi != NULL)
		{
			mi = mdi -> getItem();
		}
		if (mi != NULL)
		{
			// beautify the message.
			std::string msgtext;
			msgtext += "From:";
			int width = 60;
			int i;

			if (mi -> p != NULL)
			{
				msgtext += (mi -> p) -> Name();
			}
			else
			{
				msgtext += "Unknown";
			}
			msgtext += "   Date: N/A\n";
	
			msgtext += "Recommendation:";
			msgtext += mi -> recommendname;
			msgtext += "\n";

			msgtext += "__Message";

			for(i = 9; i < width - 1; i++)
			{
				msgtext += "_";
			}
			msgtext += "\n\n";

			// draw Message Details.
			msgtext += mi -> msg;

			// set the buffer
			buf -> text(msgtext.c_str());
		}
		else
		{
			buf -> text("Select Message for Details!");
		}
		msg_item_ok = true;
	}

	if (!msg_dialog_ok)
	{
		// display the recommendation.
		PQFileItem *fi = get_recommend();
		if (fi == NULL)
		{
			ui -> msg_recommend -> value("");
		}
		else
		{
			ui -> msg_recommend -> value((fi -> name).c_str());
		}
		msg_dialog_ok = true;
	}

	// at the end here, we handle chats.
	if (server -> chatChanged.Changed(0))
	{
		// get the items from the list.
		std::list<ChatItem *> clist = server -> getChatQueue();
		std::list<ChatItem *>::iterator it;
		for(it = clist.begin(); it != clist.end(); it++)
		{
			if ((*it) -> p)
			{
				chatmsg((*it) -> msg, (*it) -> p -> Name());
			}
			else
			{
				/* ignore (from loopback device!) */
			}

			delete (*it);
		}
	}
	return 1;
}

bool isFileTransferItem(PQItem *item)
{
        if ((item -> type == PQI_ITEM_TYPE_FILEITEM) &&
        	(item -> subtype == PQI_FI_SUBTYPE_TRANSFER))
	                return true;
        return false;
}


int	fltkserver::update_transfer()
{
	/* only update this irregularly */
	if (transfer_ok)
		return 0;

	FileTransferItem *fti;
	std::list<FileTransferItem *>::iterator it, it_in;
	std::list<FileTransferItem *> trans_in = server -> getTransfers();

	Fl_Funky_Browser *tb = ui -> transfer_downloads;
	bool transfer_changed = false;

	for(it_in = trans_in.begin(); it_in != trans_in.end(); it_in = trans_in.erase(it_in))
	{
		fti = (*it_in);

		std::ostringstream out;

		out << "fltkserver::update_transfer() Received Info: " << fti -> name;
		out << std::endl;

		// update local list.
		for(it = transfers.begin(); (fti != NULL) && 
				(it != transfers.end()); it++)
		{
			out << "fltkserver::update_transfer() Checking Against: " << (*it) -> name;

			if ((fti -> sid == (*it) -> sid) &&
				(fti -> name == (*it) -> name))
			{
				// then the same update...
				out << " Match!: incoming crate: " << fti->crate;
				(*it) -> copy(fti);
				delete fti;
				fti = NULL;
			}

			out << std::endl;
		}
		if (fti != NULL)
		{
			out << "fltkserver::update_transfer() Adding: " << fti -> name;
			transfers.push_back(fti);
			transfer_ok = false;
		}
			
		// but still trigger details redraw.
		transfer_changed = true;

	        pqioutput(PQL_DEBUG_BASIC, fltksrvrzone, out.str());
	}

	if (!transfer_ok)
	{
		int inum = tb -> value();
		tb -> clear();

		for(it = transfers.begin(); (it != transfers.end()); it++)
		{
			FTDisItem *fdi = new FTDisItem(*it);
			tb -> addItemSeries(fdi);
		}

		tb -> ItemSeriesDone();
		tb -> value(inum);
		transfer_ok = true;
		transfer_changed = true;
	}
	else if (transfer_changed)
	{
		// speedup hack....
		tb -> drawList();
	}

	if ((transfer_changed) || (!transfer_item_ok))
	{
		Fl_Text_Buffer *buf = ui -> transfer_overview -> buffer();

		FTDisItem *fdi = (FTDisItem *) tb -> getCurrentItem();
		FileTransferItem *fti = NULL;

		if (fdi != NULL)
		{
			fti = fdi -> getItem();
		}
		if (fti != NULL)
		{
			std::string msgtext;

			if (fti -> in)
			{
				msgtext += "Downloading From: ";
			}
			else
			{
				msgtext += "Uploading To: ";
			}

			if (fti -> p != NULL)
			{
				msgtext += (fti -> p) -> Name();
			}
			else
			{
				msgtext += "Unknown";
			}


			msgtext += "\n";
			msgtext += "File: ";
			msgtext += fti -> name;
			msgtext += "\n";
			msgtext += "\n";
			msgtext += "Rate: ";
			msgtext += fdi -> txt(4);
			msgtext += "\n";
			msgtext += "Transferred: ";
			msgtext += fdi -> txt(3);
			msgtext += "\n";

			// set the buffer
			buf -> text(msgtext.c_str());

		}
		else
		{
			buf -> text("Select Transfer for Details!");
		}
		transfer_item_ok = true;
	}
	if (!transfer_rates_ok)
	{
		ui -> rate_total -> value(pqih -> getMaxRate(true));
		ui -> rate_indiv -> value(pqih -> getMaxIndivRate(true));
		transfer_rates_ok = true;
	}

	return 1;
}


int	fltkserver::update_search()
{
	if (server == NULL)
		return -1;

	bool search_changed = false;

	Fl_Funky_Browser *sb = ui -> srch_results;
	FileDisItem *fdi = NULL;
	PQFileItem *result_item = NULL;

	if ((server -> searchChanged.Changed(0)) || (!search_list_ok))
	{
		search_changed = true;
		update_search_browser();
		search_list_ok = true;
	}
	if ((search_changed) || (!result_item_ok))
	{
		fdi = (FileDisItem *) sb -> getCurrentItem();
		if (fdi != NULL)
		{
			result_item = fdi -> getItem();
		}

		if (result_item != NULL)
		{
			// instead of changing the label -> allow the 
			// activate the buttons.
			if (result_item -> flags & PQI_ITEM_FLAG_LOCAL)
			{
				//ui -> download_button -> label("Recommend");
				ui -> download_button -> deactivate();
				ui -> recommend_button -> activate();
			}
			else
			{
				//ui -> download_button -> label("Download");
				ui -> download_button -> activate();
				ui -> recommend_button -> deactivate();
			}
		}
		result_item_ok = true;
	}
	return 1;
}


int	fltkserver::update_config()
{
	// draw the Messages.
	std::list<std::string> &dirs = server -> getSearchDirectories();

	std::list<std::string>::iterator mit;
	
	Fl_Browser *br = ui -> config_search_dir;
	int itemnum = br -> value();
	bool dir_changed = false;

	if (!dir_list_ok)
	{
		dir_changed = true;
		br -> clear();

		for(mit = dirs.begin(); mit != dirs.end(); mit++)
		{
			br -> add((*mit).c_str());
		}

		br  -> value(itemnum);
		dir_list_ok = true;
	}
	if (!save_dir_ok)
	{
		ui -> config_save_dir -> value((server->getSaveDir()).c_str());
		save_dir_ok = true;
	}

	if (!server_set_ok)
	{
		cert *own = sslr -> getOwnCert();
		ui -> config_local_addr -> value(
				inet_ntoa(own -> localaddr.sin_addr));
		ui -> config_local_port -> value(
				ntohs(own -> localaddr.sin_port));

		ui -> config_firewall -> value(own -> Firewalled());
		ui -> config_forward -> value(own -> Forwarded());

		if (own -> Firewalled())
		{
			if (own -> Forwarded())
			{
				ui -> config_server_addr -> value(
					inet_ntoa(own -> serveraddr.sin_addr));
				ui -> config_server_port -> value(
					ntohs(own -> serveraddr.sin_port));
				ui -> config_server_addr -> readonly(0);
				//ui -> config_server_port -> readonly(0);
			}
			else
			{
				ui -> config_server_addr -> value("0.0.0.0");
				ui -> config_server_port -> value(0);
				ui -> config_server_addr -> readonly(1);
				//ui -> config_server_port -> readonly(1);
			}
		}
		else
		{
			ui -> config_server_addr -> value(
				inet_ntoa(own -> localaddr.sin_addr));
			ui -> config_server_port -> value(
				ntohs(own -> localaddr.sin_port));
			ui -> config_server_addr -> readonly(1);
			//ui -> config_server_port -> readonly(1);
		}

		server_set_ok = true;
	}

	return 1;
}

/********************** PQI SSL INTERFACE ************************/
int fltkserver::setuppqissl(filedexserver *fd, pqipersongrp *ph, sslroot *r, UserInterface *u)
{
	server = fd;
	pqih = ph;
	sslr = r;
	ui = u;

	return 1;
}

int fltkserver::search_new()
{
	// read text from new_search.
	// send to server.
	// clear search terms.
	// 
	int i;
	std::list<std::string> terms;
	std::list<std::string>::iterator it;

	const char *termlist = ui -> new_search -> value();
	int num_str_len = strlen(termlist) + 2;

	char numstr[num_str_len];
	for(i = 0; i < (signed) strlen(termlist); i++)
	{
		numstr[i] = termlist[i];
	}
	for(;i < num_str_len; i++)
	{
		numstr[i] = '\0';
	}

	std::ostringstream out;
	out << "Read in:" << numstr << std::endl;

	// now break it into terms.
	int sword = 0;
	for(i = 0; (i < num_str_len - 1) && (numstr[i] != '\0'); i++)
	{
		if (isspace(numstr[i]))
		{
			numstr[i] = '\0';
			if (i - sword > 0)
			{
				terms.push_back(std::string(&(numstr[sword])));
				out << "Found Search Term:" << &(numstr[sword]);
				out << std::endl;
			}
			sword = i+1;
		}
	}
	// do the final term.
	if (i - sword > 0)
	{
		terms.push_back(std::string(&(numstr[sword])));
		out << "Found Search Term:" << &(numstr[sword]);
		out << std::endl;
	}

	pqioutput(PQL_DEBUG_BASIC, fltksrvrzone, out.str());
	
	server -> doSearch(terms);
	return 1;
}

int fltkserver::search_remove()
{
	Fl_Funky_Browser *sb = ui -> srch_results;
	FileDisItem *fdi = NULL;
	SearchItem *sitem = NULL;
	fdi = (FileDisItem *) sb -> getCurrentItem();
	if (fdi != NULL)
	{
		sitem = fdi -> getSearchItem();
	}

	if (sitem != NULL)
	{
		server -> removeSearchResults(sitem);
		search_list_ok = false;
		// update straight away - so no possibility of bad data.
		update_search(); 
		return 1;
	}
	return 0;
}


/***** CERT COMMANDS *****/

/* Transfer from Neighbour to Cert list */
int fltkserver::cert_add_neighbour()
{
	/* get the current neighbour */
	cert *c = cert_get_current_neighbour();

	/* check auth code */
	if ((c == NULL) || (c->certificate == NULL))
	{
		std::cerr << "fltkserver::cert_add_neighbour() no cert";
		std::cerr << std::endl;
		return 1;
	}

	if (c == sslr -> getOwnCert())
	{
		std::cerr << "fltkserver::cert_add_neighbour() Own Cert";
		std::cerr << std::endl;
		return 1;
	}

	std::string inp_code = ui->neigh_authcode->value();
	std::string real_code = getXPGPAuthCode(c->certificate);
	bool validAuthCode = (inp_code == real_code);

	if (validAuthCode)
	{
		std::cerr << "fltkserver::cert_add_neighbour() Valid Auth Code";
		std::cerr << std::endl;
	}
	else
	{
		std::cerr << "fltkserver::cert_add_neighbour() No Auth Code";
		std::cerr << std::endl;
	}

	/* if correct -> sign */
	if (validAuthCode)
	{
	  	if (0 < validateCertificateIsSignedByKey(c->certificate, 
			sslr -> getOwnCert() -> certificate))
		{
			std::cerr << "fltkserver::cert_add_neighbour() Signed Already";
			std::cerr << std::endl;
		}
		else
		{
			/* if not signed already */
			std::cerr << "fltkserver::cert_add_neighbour() Signing Cert";
			std::cerr << std::endl;

			/* sign certificate */
			sslr -> signCertificate(c);
		}
	}

	/* check authentication */
	sslr -> checkAuthCertificate(c);
	if (c->trustLvl < TRUST_SIGN_AUTHEN)
	{
		/* do nothing */
		std::cerr << "fltkserver::cert_add_neighbour() Not Authed";
		std::cerr << std::endl;
		std::cerr << "fltkserver::cert_add_neighbour() Do Nothing";
		std::cerr << std::endl;
	}
	else
	{


		std::cerr << "fltkserver::cert_add_neighbour() Adding Cert";
		std::cerr << std::endl;

		/* add */
		sslr -> addUntrustedCertificate(c);


		std::cerr << "fltkserver::cert_add_neighbour() Auto Connecting";
		std::cerr << std::endl;
		/* auto connect */
		pqih -> cert_auto(c, true);

		// tell to redraw.
		cert_neighbour_list_ok = false;
		cert_neighbour_item_ok = false;

		cert_list_ok = false;
		cert_item_ok = false;
	}
	return 1;
}


int fltkserver::cert_allow_current()
{
	cert *c = cert_get_current();

	if ((c == NULL) || (c == sslr -> getOwnCert()))
	{
		return -1;
	}
	pqih -> cert_accept(c);
	return 1;
}

int fltkserver::cert_deny_current()
{
	cert *c = cert_get_current();

	if ((c == NULL) || (c == sslr -> getOwnCert()))
	{
		return -1;
	}
	pqih -> cert_deny(c);
	return 1;
}

int fltkserver::cert_listen_current()
{
	cert *c = cert_get_current();

	if ((c == NULL) || (c == sslr -> getOwnCert()))
	{
		return -1;
	}

	if (c -> WillListen())
	{
		c -> WillListen(false);
	}
	else
	{
		c -> WillListen(true);
	}
	return 1;
}

int fltkserver::cert_connect_current()
{
	cert *c = cert_get_current();

	if ((c == NULL) || (c == sslr -> getOwnCert()))
	{
		return -1;
	}
	if (c -> WillConnect())
	{
		c -> WillConnect(false);
	}
	else
	{
		c -> WillConnect(true);
	}
	return 1;
}


int fltkserver::cert_tag_current()
{
	cert *c = cert_get_current();
	// can change our tag.
	if (c == NULL)
	{
		return -1;
	}
	//c -> Name(std::string(ui -> cert_newtag -> value()));
	return 1;
}

int     fltkserver::cert_check_auto()
{
	std::ostringstream out;
	out << "fltkserver::cert_check_auto()";
	cert *c = cert_get_current();
	// don't check own here, as we want to change it back
	// if checked.
	if (c == NULL)
	{
		out << "NULL cert!" << std::endl;
		return -1;
	}

	Fl_Funky_Browser *cb = ui -> cert_list;
	bool ac = ((bool) cb -> getCheckState());
	if (c -> Manual() == ac) // ie mean different things.
	{
		out << " fltkserver::cert_check_auto() Changing State! (";
		out << cb -> value();
		out << ") cert -> Manual():" << c -> Manual();
		out << " Check Box: " << ac << std::endl;
		out << " Toggling Auto to: " << ac << std::endl;
		//cb -> toggleCheckBox(cb -> value());

		if ((c != sslr -> getOwnCert()) && 
			(c->trustLvl >= TRUST_SIGN_AUTHEN))
		{
			pqih -> cert_auto(c, ac);
		}
		else
		{
			// toggle to off.
			cb -> toggleCheckBox(cb->value());
		}
	}
	out << " fltkserver::cert_check_auto() Final State(";
	out << cb -> value();
	out << ") cert -> Manual():" << c -> Manual();
	out << " Check Box: " << cb -> getCheckState() << std::endl;
	pqioutput(PQL_DEBUG_BASIC, fltksrvrzone, out.str());
	return 1;
}

int     fltkserver::cert_toggle_auto()
{
	std::ostringstream out;
	out << "fltkserver::cert_toggle_auto()";
	cert *c = cert_get_current();
	if ((c == NULL) || (c == sslr -> getOwnCert()))
	{
		out << "NULL cert!" << std::endl;
		return -1;
	}

	Fl_Funky_Browser *cb = ui -> cert_list;
	bool ac = ((bool) cb -> getCheckState());
	out << " Toggling Auto:" << ac << " to " << !ac << std::endl;

	cb -> toggleCheckBox(cb -> value());
	pqih -> cert_auto(c,  !ac);

	pqioutput(PQL_DEBUG_BASIC, fltksrvrzone, out.str());
	return 1;
}



static const std::string gui_nb = "GUI_NB";
static const std::string gui_cb = "GUI_CB";
static const std::string gui_srb = "GUI_SRB";
static const std::string gui_mb = "GUI_MB";
static const std::string gui_ftb = "GUI_FTB";
static const std::string gui_chab = "GUI_CHAB";
static const std::string gui_chfb = "GUI_CHFB";

static const std::string gui_nb_default = "Acpt (0:60:0)\tTrust (1:70:0)\tLast_Seen (2:80:0)\tName (3:150:0)";

static const std::string gui_cb_default = "Person (1:110:0)\tStatus (0:120:0)\tAuto_Connect (2:150:0)\tTrust_Lvl (3:120:0)\tPeer_Address (4:150:0)";

static const std::string gui_srb_default = "Source (1:100:0)\tKeyword (0:110:1)\tFilename (2:340:0)\tSize (3:100:0)\tN/A (4:100:0)";

static const std::string gui_mb_default = "Source (0:100:1)\tMessage (2:150:0)\tDate (1:100:0)\tRecommendation (3:250:0)\tSize (4:100:0)";

static const std::string gui_ftb_default = "Source (0:110:0)\tStatus (1:120:0)\tFilename (2:250:0)\tCompleted (3:150:0)\tRate (4:100:0)";

// Channel Available and File browsers.
static const std::string gui_chab_default = "Mode (0:85:0)\tRank (1:80:0)\tName (2:250:0)\t#Msgs (3:90:0)\tHash (4:200:0)";
static const std::string gui_chfb_default = "Date (0:80:0)\tMsg (1:360:0)\t#Files (2:80:0)\tSize (3:85:0)\tHash (4:200:0)";


int fltkserver::cert_save_config()
{
	if (sslr == NULL)
		return -1;

	// First update the gui config.
	Fl_Funky_Browser *fb = ui -> cert_list;
	sslr -> setSetting(gui_cb, fb -> setup());

	fb = ui -> cert_neighbour_list;
	sslr -> setSetting(gui_nb, fb -> setup());

	fb = ui -> srch_results;
	sslr -> setSetting(gui_srb, fb -> setup());

        fb = ui -> msg_list;
	sslr -> setSetting(gui_mb, fb -> setup());

        fb = ui -> transfer_downloads;
	sslr -> setSetting(gui_ftb, fb -> setup());

	// save channel setting.
        fb = ui -> channel_list;
	sslr -> setSetting(gui_chab, fb -> setup());
        fb = ui -> channel_file_list;
	sslr -> setSetting(gui_chfb, fb -> setup());

	// also call save all the other parts.
	server -> save_config();
	ad -> save_configuration();
	pqih -> save_config();

	sslr -> saveCertificates();
	return 1;
}

int fltkserver::cert_load_gui_config()
{
	if (sslr == NULL)
		return -1;

	std::string empty("");
	std::string tmp("");

	// First update the gui config.
	Fl_Funky_Browser *fb = ui -> cert_list;
	if (empty != (tmp = sslr -> getSetting(gui_cb)))
	{
		fb -> setup(tmp);
	}
	else
	{
		fb -> setup(gui_cb_default);
	}

	fb = ui -> cert_neighbour_list;
	if (empty != (tmp = sslr -> getSetting(gui_nb)))
	{
		//fb -> setup(tmp);
		fb -> setup(gui_nb_default);
	}
	else
	{
		fb -> setup(gui_nb_default);
	}

	fb = ui -> srch_results;
	if (empty != (tmp = sslr -> getSetting(gui_srb)))
	{
		fb -> setup(tmp);
	}
	else
	{
		fb -> setup(gui_srb_default);
	}

        fb = ui -> msg_list;
	if (empty != (tmp = sslr -> getSetting(gui_mb)))
	{
		fb -> setup(tmp);
	}
	else
	{
		fb -> setup(gui_mb_default);
	}

        fb = ui -> transfer_downloads;
	if (empty != (tmp = sslr -> getSetting(gui_ftb)))
	{
		fb -> setup(tmp);
	}
	else
	{
		fb -> setup(gui_ftb_default);
	}

        fb = ui -> channel_list;
	if (empty != (tmp = sslr -> getSetting(gui_chab)))
	{
		fb -> setup(tmp);
	}
	else
	{
		fb -> setup(gui_chab_default);
	}

        fb = ui -> channel_file_list;
	if (empty != (tmp = sslr -> getSetting(gui_chfb)))
	{
		fb -> setup(tmp);
	}
	else
	{
		fb -> setup(gui_chfb_default);
	}

	// these are done externally...
	//pqih -> load_config();
	//server -> load_config();
	//ad -> load_configuration();
	return 1;
}


cert *fltkserver::cert_get_current()
{
	if (sslr == NULL)
		return NULL;

	Fl_Funky_Browser *cb = ui -> cert_list;
	PersonDisItem *pdi = (PersonDisItem *) cb -> getCurrentItem();
	Person *p = NULL;
	cert *c = NULL;
	if (pdi != NULL)
	{

		p = pdi -> getItem();
	}
	// dynamic to be save - shouldn't be needed.
	c = dynamic_cast<cert *>(p);
	return c;
}

cert *fltkserver::cert_get_current_neighbour()
{
	Fl_Funky_Browser *cb = ui -> cert_neighbour_list;
	PersonDisItem *pdi = (PersonDisItem *) cb -> getCurrentItem();
	Person *p = NULL;
	cert *c = NULL;
	if (pdi != NULL)
	{

		p = pdi -> getItem();
	}
	// dynamic to be save - shouldn't be needed.
	c = dynamic_cast<cert *>(p);
	return c;
}

int fltkserver::cert_remove_current()
{
	cert * c = cert_get_current();
	if (c != NULL)
	{
		if (c->Accepted())
		{
			std::ostringstream out;
			out << "Please Deactivate Person";
			out << " (by unticking the box) ";
			out << std::endl;
			out << "\tBefore attempting to remove them";
			alertmsg(0, 1, out.str(), "");
		}
		return sslr -> removeCertificate(c);
	}
	return -1;
}

int fltkserver::cert_save_servaddr()
{
	// read inet + port address from gui.

	struct sockaddr_in addr;

	// check if valid.
	cert *c = cert_get_current();
	if ((c == NULL) || (c == sslr -> getOwnCert()))
	{
		pqioutput(PQL_ALERT, fltksrvrzone, "NULL/Own cert!");
		return -1;
	}


/********************************** WINDOWS/UNIX SPECIFIC PART *******************/
#ifndef WINDOWS_SYS
	if (0 != inet_aton(ui -> cert_server -> value(), &(addr.sin_addr)))
#else
	addr.sin_addr.s_addr = inet_addr(ui -> cert_server -> value());
	if (1)
#endif
/********************************** WINDOWS/UNIX SPECIFIC PART *******************/
	{
		// get the server address.
		c -> serveraddr.sin_addr = addr.sin_addr;
		c -> serveraddr.sin_port = htons((short) ui -> cert_port -> value());
		return 1;
	}
	else
	{
		// invalid ... reset the text to indicate.
		ui -> cert_server -> value("Invalid Address");
		return 0;
	}
}

int fltkserver::cert_saveaddr_connect()
{
	if (0 < cert_save_servaddr())
	{
		cert *c = cert_get_current();
		c -> nc_timestamp = 0;
		// set Firewall to off -> so we 
		// will definitely have connect attempt.
		c -> Firewalled(false);

		c -> WillConnect(true);
		return 1;
	}
	return 0;
}


int fltkserver::msg_channel_select()
{
	return 1;
}



int fltkserver::msg_reply()
{
	return 1;
}


int fltkserver::msg_remove()
{
	Fl_Funky_Browser *mb = ui -> msg_list;
	MsgDisItem *mdi = NULL;
	MsgItem *mitem = NULL;
	mdi = (MsgDisItem *) mb -> getCurrentItem();
	if (mdi != NULL)
	{
		mitem = mdi -> getItem();
	}

	if (mitem != NULL)
	{
		server -> removeMsgItem(mitem);
		msg_list_ok = false;
		msg_item_ok = false;
		msg_dialog_ok = false;

		update_msgs(); 
		return 1;
	}
	return 0;
}


int fltkserver::transfer_select()
{
	return 1;
}


int fltkserver::transfer_cancel()
{
	PQFileItem *cancelled;

	/* get the current item */
	Fl_Funky_Browser *tfb = ui -> transfer_downloads;
	FTDisItem *tfdi = NULL;

	tfdi = (FTDisItem *) tfb -> getCurrentItem();
	if (tfdi != NULL)
	{
		cancelled = tfdi -> getItem();
	}

	if (cancelled == NULL)
	{
		return 0;
	}

	server -> cancelTransfer(cancelled -> clone());
	return 1;
}

// just clear the list - it'll be repopulated.
int fltkserver::transfer_clear()
{
	std::list<FileTransferItem *>::iterator it;
	for(it = transfers.begin(); (it != transfers.end());)
	{
		delete (*it);
		it = transfers.erase(it);
		pqioutput(PQL_DEBUG_BASIC, fltksrvrzone, 
			"fltkserver::transfer_clear() Deleting FT Info!");
	}
	/* now tell server to do the same */
	server -> clear_old_transfers();
	transfer_ok = false;
	return 1;
}


int fltkserver::transfer_rates()
{
	pqih -> setMaxRate(true, ui -> rate_total -> value());
	pqih -> setMaxRate(false, ui -> rate_total -> value());
	pqih -> setMaxIndivRate(true, ui -> rate_indiv -> value());
	pqih -> setMaxIndivRate(false, ui -> rate_indiv -> value());
	return 1;
}

int	fltkserver::set_recommend(PQFileItem *rec)
{
	msg_dialog_ok = false;
	if (rec == NULL)
	{
		recommend = NULL;
		return 1;
	}
	if (recommend != NULL)
	{
		delete recommend;
	}
	recommend = (PQFileItem *) rec -> clone();
	return 1;
}

PQFileItem *fltkserver::get_recommend()
{
	return recommend;
}

int fltkserver::search_download()
{
	if (server == NULL)
		return -1;

	Fl_Funky_Browser *sb = ui -> srch_results;
	FileDisItem *fdi = NULL;
	PQFileItem *result_item = NULL;

	fdi = (FileDisItem *) sb -> getCurrentItem();
	if (fdi != NULL)
	{
		result_item = fdi -> getItem();
	}

	if (result_item == NULL)
	{
		return 0;
	}

	// check if remote -> then download.
	if (result_item -> flags & PQI_ITEM_FLAG_LOCAL)
	{
		return -1;
	}

	// otherwise ... can get search item.
	server -> getSearchFile(result_item);
	return 1;
}


int fltkserver::search_recommend()
{
	if (server == NULL)
		return -1;

	Fl_Funky_Browser *sb = ui -> srch_results;
	FileDisItem *fdi = NULL;
	PQFileItem *result_item = NULL;

	fdi = (FileDisItem *) sb -> getCurrentItem();
	if (fdi != NULL)
	{
		result_item = fdi -> getItem();
	}

	if (result_item == NULL)
	{
		return 0;
	}

	// check if remote -> then download.
	if (result_item -> flags & PQI_ITEM_FLAG_LOCAL)
	{
		// Setup the recommend message....
		std::ostringstream out;
		out << "fltkserver::search_download() Recommending: ";
		out << result_item -> name << std::endl;
		pqioutput(PQL_DEBUG_BASIC, fltksrvrzone, out.str());

		// setup recommend gui.
		set_recommend(result_item);
		ui -> msg_dialog -> show();
		return 2;
	}
	return -1;
}

int fltkserver::msg_send()
{
	if (server == NULL)
	{
		return -1;
	}

	// get msg_text from dialog.
	Fl_Text_Buffer *buf = ui -> msg_text -> buffer();
	std::string msg = buf -> text();

	server -> sendRecommend(get_recommend(), msg);

	// clear the buffer, and the recommend.
	buf -> text("");
	set_recommend(NULL);

	return 1;
}


int fltkserver::download_recommend()
{
	if (server == NULL)
		return -1;

	Fl_Funky_Browser *mb = ui -> msg_list;
	MsgDisItem *mdi = NULL;
	MsgItem *msg_item = NULL;

	mdi = (MsgDisItem *) mb -> getCurrentItem();
	if (mdi != NULL)
	{
		msg_item = mdi -> getItem();
	}
	if (msg_item == NULL)
	{
		return 0;
	}

	pqioutput(PQL_DEBUG_BASIC, fltksrvrzone, 
		"fltkserver::download_recommend() passed ET -> getting file");
	return server -> getSearchFile(msg_item);
}


/* The new search regime.... for data handling.... */

int fltkserver::update_search_browser()
{
	if (server == NULL)
		return -1;

	Fl_Funky_Browser *sb = ui -> srch_results;

	std::map<SearchItem *, std::list<PQFileItem *> >::iterator it;
	std::list<PQFileItem *>::iterator fit;

	DisplayData *selected = sb -> getSelected();
	PQFileItem *selitem;
	if (selected != NULL)
	{
		selitem = ((FileDisItem *) selected) -> getItem();
	}

	if ((server->searchMajorChanged.Changed(0)) || (1 != sb->checkSort()))
	{
		std::map<SearchItem *, std::list<PQFileItem *> > 
				&results = server -> getResults();
		sb -> clear();
		for(it = results.begin(); it != results.end(); it++)
		{
		  for(fit = (it -> second).begin(); 
				fit != (it -> second).end(); fit++)
		  {
			FileDisItem *fdi = new FileDisItem(*fit, it -> first);
			sb -> addItemSeries(fdi);
		  }
		}
	}
	else
	{
		// else take it easy.
		// get new item - not reference
		std::map<SearchItem *, std::list<PQFileItem *> > 
				new_results = server -> getNewResults();
		for(it = new_results.begin(); it != new_results.end(); it++)
		{
		  for(fit = (it -> second).begin(); 
				fit != (it -> second).end(); fit++)
		  {
			FileDisItem *fdi = new FileDisItem(*fit, it -> first);
			sb -> addItemSeries(fdi);
		  }
		}
	}

	sb -> ItemSeriesDone();
	sb -> selectDD(selected);
	return 1;
}



std::string fltkserver::getHomePath()
{
	std::string home;
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS /* UNIX */

	home = getenv("HOME");

#else /* Windows */

	std::ostringstream out;
	char *h2 = getenv("HOMEDRIVE");
	out << "fltkpqi::basedir() -> $HOMEDRIVE = ";
	out << h2 << std::endl;
	char *h3 = getenv("HOMEPATH");
	out << "fltkpqi::basedir() -> $HOMEPATH = ";
	out << h3 << std::endl;

	if (h2 == NULL)
	{
		// Might be Win95/98
		// generate default.
		home = "C:\\Retro";
	}
	else
	{
		home = h2;
		home += h3;
		home += "\\Desktop";
	}

	out << "fltkserver::getHomePath() -> " << home << std::endl;
	pqioutput(PQL_DEBUG_BASIC, fltksrvrzone, out.str());

	// convert to FLTK desired format.
	home = make_path_unix(home);
#endif
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/
	return home;
}

	                        
int     fltkserver::file_select(int type)
{
	std::string home = getHomePath();
	// save the type then setup the selector.
	fselect_type = type;
	ui -> file_chooser -> directory(home.c_str());

	switch(type)
	{
	case FILE_CHOOSER_IMPORT:
		ui -> file_chooser -> type(Fl_File_Chooser::SINGLE);
		ui -> file_chooser -> filter("Certificate Files (*.{pem,pqi})");
		//ui -> file_chooser -> title("Import Certificate");
		break;
	case FILE_CHOOSER_EXPORT:
		ui -> file_chooser -> type(Fl_File_Chooser::CREATE);
		ui -> file_chooser -> filter("Certificate Files (*.{pem,pqi})");
		//ui -> file_chooser -> title("Export Certificate");
		break;
	case FILE_CHOOSER_DIR:
		ui -> file_chooser -> type(Fl_File_Chooser::DIRECTORY);
		ui -> file_chooser -> filter("Directories (*.*)");
		//ui -> file_chooser -> title("Add Share Directory");
		break;
	case FILE_CHOOSER_SAVEDIR:
		ui -> file_chooser -> type(Fl_File_Chooser::DIRECTORY);
		ui -> file_chooser -> filter("Directories (*.*)");
		//ui -> file_chooser -> title("Select Save Directory");
		break;
	default:
		break;
	}
	return 1;
}

int     fltkserver::file_updateName()
{
	return 1;
}

int     fltkserver::file_completeIO()
{
	// take filename from line.
	std::string nullhash(""); // empty hash - as signature not correct.

	if (NULL == ui -> file_chooser -> value())
	{
		pqioutput(PQL_ALERT, fltksrvrzone, 
			"filtserver::file_completeIO() NULL file selected!");
		return 1;
	}

	std::string fileio_name = std::string(ui -> file_chooser -> value());

	std::ostringstream out;
	out << "fltkserver::file_completeIO() supposed to ";

	cert *nc, *c;

	switch(fselect_type)
	{
	case FILE_CHOOSER_IMPORT:

		out << " IMPORT " << fileio_name << std::endl;
		// The load makes sure it is unique.
		nc = sslr -> loadcertificate(fileio_name.c_str(), nullhash);
		if (nc == NULL)
		{
			out << "File Import (" << fileio_name;
			out << ") Failed!" << std::endl;
			pqioutput(PQL_DEBUG_BASIC, fltksrvrzone, out.str());
			return -1;
		}
		else  if (0 > sslr -> addCollectedCertificate(nc))
		{
			out << "Imported Certificate....but no";
			out << " need for addition - As it exists!";
			out << std::endl;
			pqioutput(PQL_DEBUG_BASIC, fltksrvrzone, out.str());
			return 0;
		}
			
		/* ensure it gets to p3disc */
		if (ad)
		{
		  /* tick the p3disc to ensure the new one gets into
		   * the list before the next update.
		   */
		  ad -> distillData();
		  //ad->collectCerts();
		  cert_neighbour_list_ok = false;
		}

		pqioutput(PQL_DEBUG_BASIC, fltksrvrzone, out.str());
		return 1;
		break;

	case FILE_CHOOSER_EXPORT:

		/* before we save the file we need to ensure that
		 * it has the correct extension
		 */

		ensureExtension(fileio_name, "pqi");

		out << " EXPORT to " << fileio_name << std::endl;
		c = cert_get_current();
		if (c == NULL)
		{
			c = cert_get_current_neighbour();
		}

		if (c == NULL)
		{
			out << "File Export (" << fileio_name;
			out << ") Failed - Select A Cert!" << std::endl;
		pqioutput(PQL_DEBUG_BASIC, fltksrvrzone, out.str());
			return -1;
		}
		if (c -> certificate == NULL)
		{
			out << "File Export (" << fileio_name;
			out << ") Failed - X509 == NULL!" << std::endl;
		pqioutput(PQL_DEBUG_BASIC, fltksrvrzone, out.str());
			return -1;
		}


		pqioutput(PQL_DEBUG_BASIC, fltksrvrzone, out.str());
		return sslr -> savecertificate(c, fileio_name.c_str());
		break;

	case FILE_CHOOSER_DIR:
		// add the directory to the servers list.
		ui -> file_chooser -> type(Fl_File_Chooser::DIRECTORY);
		server -> addSearchDirectory(fileio_name.c_str());
		dir_list_ok = false;

		break;
	case FILE_CHOOSER_SAVEDIR:
		// change the save directory.
		save_dir_ok = false;
		ui -> file_chooser -> type(Fl_File_Chooser::DIRECTORY);
		server -> setSaveDir(fileio_name.c_str());
		break;
	default:
		break;
	}

	pqioutput(PQL_DEBUG_BASIC, fltksrvrzone, out.str());

	return 1;
	if (fileio_import)
	{
	}
	else // export
	{
	}
	return 0;
}

int     fltkserver::config_remove_dir()
{
	int i = ui -> config_search_dir -> value();
	if (i > 0)
	{
		server -> removeSearchDirectory(
					ui -> config_search_dir -> text(i));
		dir_list_ok = false;
	}
	return 1;
}


// this is call if the ticks are changed.
int fltkserver::config_server_update()
{
	cert *own = sslr -> getOwnCert();

	// can forward on if firewalled.
	if (ui -> config_forward -> value())
		ui -> config_firewall -> value(1);

	if (ui -> config_firewall -> value())
	{
		if (ui -> config_forward -> value())
		{
			ui -> config_server_addr -> value(
				inet_ntoa(own -> serveraddr.sin_addr));
			ui -> config_server_port -> value(
				ntohs(own -> serveraddr.sin_port));
			ui -> config_server_addr -> readonly(0);
			//ui -> config_server_port -> readonly(0);
		}
		else
		{
			ui -> config_server_addr -> value("0.0.0.0");
			ui -> config_server_port -> value(0);
			ui -> config_server_addr -> readonly(1);
			//ui -> config_server_port -> readonly(1);
		}
	}
	else
	{
		ui -> config_server_addr -> value(
			inet_ntoa(own -> localaddr.sin_addr));
		ui -> config_server_port -> value(
			ntohs(own -> localaddr.sin_port));
		ui -> config_server_addr -> readonly(1);
		//ui -> config_server_port -> readonly(1);
	}
	return 1;
}

int fltkserver::config_server_change()
{
	// check which settings have changed.
	// if localaddress is different. then restart server.
	//
	// all the other settings just get stored and sent to 
	// others for their info.
	
	struct in_addr inaddr_local, inaddr_server;
	if (0 == inet_aton(ui -> config_local_addr -> value(), &inaddr_local))
	{
		// bad address - reset.
		server_set_ok = false;
		return -1;
	}

	cert *c = sslr -> getOwnCert();
	bool local_changed = false;


	/* always change the address (checked by sslr->checkNetAddress()) */
	c -> localaddr.sin_addr = inaddr_local;
	c -> localaddr.sin_port = htons((short) ui -> config_local_port -> value());
	local_changed = true;

	if ((ui -> config_firewall -> value()) &&
	    (ui -> config_forward -> value()))
	{
		if (0 != inet_aton(ui -> config_server_addr -> value(), 
			&inaddr_server))
	   	{
		  c -> serveraddr.sin_addr = inaddr_server;
		  c -> serveraddr.sin_port = htons((short) ui -> config_server_port -> value());
		}
	}

	c -> Firewalled(ui -> config_firewall -> value());
	c -> Forwarded(ui -> config_forward -> value());

	if (local_changed)
	{
		sslr -> checkNetAddress();
		pqih -> restart_listener();
		sslr -> CertsChanged();
	}
	server_set_ok = false;
	return 1;
}

std::string make_path_unix(std::string path)
{
	for(unsigned int i = 0; i < path.length(); i++)
	{
		if (path[i] == '\\')
			path[i] = '/';
	}
	return path;
}



int     fltkserver::addAlerts(alertBox *m, alertBox *c)
{
	msg = m;
	chat = c;
	return 1;
}


int     fltkserver::ownchatmsg(std::string m)
{
	server -> sendChat(m);
	return 1;
}


int     fltkserver::chatmsg(std::string m, std::string source)
{
	chat -> sendMsg(0,2,m, source);
	return 1;
}


int     fltkserver::alertmsg(int type, int sev, std::string m, std::string src)
{
	msg -> sendMsg(type, sev,m, src);
	return 1;
}

int fltkserver::update_dirlist()
{
	// get the list of certificates.
	// get the dirlist.
	std::list<DirBase *> &dirs = server -> getDirList();
	std::list<DirBase *>::iterator dit;
	
	std::list<cert *>::iterator it;
	std::list<cert *> &certs = sslr -> getCertList();

	for(it = certs.begin(); it != certs.end(); it++)
	{
		cert *c = (*it);
		if ((c != NULL) && (c -> Connected()))
		{
			bool found = false;
			for(dit = dirs.begin(); dit != dirs.end(); dit++)
			{
				if ((*dit) -> p == c)
				{
					found = true;
				}
			}
			if (!found)
			{
				// send off searchitem....
				std::ostringstream out;
				out << "Generating FileDir Search Item for: " << *it << std::endl;
				pqioutput(PQL_DEBUG_BASIC, fltksrvrzone, out.str());

				SearchItem *si = new SearchItem(PQI_SI_SUBTYPE_SEARCH);
				si -> datatype = PQI_SI_DATATYPE_DIR;
				si -> data = "";
				// set the target, so it don't go to all!.
				si -> cid  = c -> cid;
				si -> p  = c;
				server -> handleDirectoryRequest(si);
			}
		}
	}

	server -> printDirectoryListings();
	check_dirlist2();
	//check_dirlist();
	return 1;
}

//int fltkserver::check_dirlist()
//int fltkserver::load_dir(std::string person, std::string dir)
//
int fltkserver::load_dir(std::string person, std::string dir)
{
	// get the list of certificates.
	// get the dirlist.

        cert *c = sslr -> findpeercert(person.c_str());
	std::ostringstream out;
	out << "fltkserver::load_dir() chk:" << person << std::endl;
	if ((c != NULL) && (c -> Connected()))
	{
		// send off searchitem....
		out << "Generating FileDir Search Item for: " << c -> certificate -> name << std::endl;
		SearchItem *si = new SearchItem(PQI_SI_SUBTYPE_SEARCH);
		si -> datatype = PQI_SI_DATATYPE_DIR;
		si -> data = dir;
		si -> cid  = c -> cid;
		si -> p    = c;
		server -> handleDirectoryRequest(si);
		pqioutput(PQL_DEBUG_BASIC, fltksrvrzone, out.str());
		return 1;
	}
	out << "fltkserver::load_dir() Failed to match request to person: " << person << std::endl;
	pqioutput(PQL_DEBUG_BASIC, fltksrvrzone, out.str());
	return 1;
}


int	process_subdir(DirNode * dir, Fl_Item *p);

int fltkserver::check_dirlist()
{
	// get the list of certificates.
	// get the dirlist.
	std::list<DirBase *> &dirs = server -> getDirList();
	std::list<DirBase *>::iterator dit;
	
	//std::list<std::string> certs = sslr -> listCertificates();
	//std::list<std::string>::iterator it;

	// clear browser.
	delete_all(0);

	Fl_Item *p = NULL;

	for(dit = dirs.begin(); dit != dirs.end(); dit++)
	{
		Fl_Item *di = Fl_Person_Item_type.make(p);
		if ((*dit)->p)
		{
			di->name((*dit)->p->Name().c_str());
			// put proper link in.
			((Fl_Person_Item *) di) -> person_hash = 
				((cert *) ((*dit)->p)) -> certificate -> name;
		}
		else
		{
			di->name("LOCAL");
			((Fl_Person_Item *) di) -> person_hash = 
				sslr -> getOwnCert() -> certificate -> name;
		}
		process_subdir(*dit, di);
	}
	return 1;
}

int	process_subdir(DirNode * dir, Fl_Item *p)
{
	std::list<PQFileItem *> &files = dir->files;
        std::list<DirNode *> &dirs = dir->subdirs;
	std::list<PQFileItem *>::iterator fit;
        std::list<DirNode *>::iterator dit;

	{
	  std::ostringstream out;
	  out << "process_subdir(" << dir -> name << ")";
	  pqioutput(PQL_DEBUG_BASIC, fltksrvrzone, out.str());
	}
	
	for(dit = dirs.begin(); dit != dirs.end(); dit++)
	{
		Fl_Item *di = Fl_Dir_Item_type.make(p);
		process_subdir(*dit, di);
		di -> name((*dit)->name.c_str());
	}

	for(fit = files.begin(); fit != files.end(); fit++)
	{
		std::ostringstream out;
		out << (*fit)->name;
		out << "   --->   " << (*fit)->size << " bytes";

		Fl_Item *fi = Fl_File_Item_type.make(p);
		fi -> name(out.str().c_str());
		((Fl_File_Item *) fi) -> filename = (*fit) -> name;
		((Fl_File_Item *) fi) -> size     = (*fit) -> size;
	}
	return 1;
}



int fltkserver::update_dirs()
{
	if (server == NULL)
		return -1;

	// draw the Messages.

	if ((server -> dirListChanged.Changed(0)) ||
		(server->dirListMajorChanged.Changed(0)))
	{
		// load lists.
		check_dirlist2();
		//check_dirlist();
	}
	return 1;
}

Person *getperson(Fl_Item *p)
{
	while(p && p->parent)
		p=p->parent;
	Fl_Person_Item *pi;
	if ((p) && (NULL != (pi = dynamic_cast<Fl_Person_Item *>(p))))
	{
        	return getSSLRoot() -> findpeercert(pi -> person_hash.c_str());
	}
	return NULL;
}

int fltkserver::dirlist_download()
{
	std::list<PQItem *> itemlist;

	// get the list of certificates.
	// get the dirlist.
	Fl_Item *it = Fl_Item::first;
	for(;it; it = it -> next)
	{
		if (it -> selected && it -> is_file())
		{
			Person *p = getperson(it);
	  		std::ostringstream out;
			out << "downloading:" << ((Fl_File_Item *) it) -> filename << std::endl;

			if (p)
			{
				out << "from :" << p -> Name() << std::endl;
			}
			else
			{
				out << "from NULL" << std::endl;
			}
	  		pqioutput(PQL_DEBUG_BASIC, fltksrvrzone, out.str());

			PQFileItem *fi = new PQFileItem();

			fi -> sid = getPQIsearchId();
			fi -> subtype = PQI_FI_SUBTYPE_REQUEST;
			fi -> name = ((Fl_File_Item *) it) -> filename;
			fi -> size = ((Fl_File_Item *) it) -> size;
			fi -> cid  = p -> cid;
			fi -> p = p;
			server -> getSearchFile(fi);
		}
	}
	return 1;
}

int fltkserver::dirlist_recommend()
{
	std::list<PQItem *> itemlist;

	// get the list of certificates.
	// get the dirlist.
	Fl_Item *it = Fl_Item::first;
	for(;it; it = it -> next)
	{
		if (it -> selected && it -> is_file())
		{
			Person *p = getperson(it);

			std::ostringstream out;
			out  << "recommending(NOT YET!): " << it -> name() << std::endl;
			if (p)
			{
				out << "from :" << p -> Name() << std::endl;
			}
			else
			{
				out << "from NULL" << std::endl;
			}
	  		pqioutput(PQL_DEBUG_BASIC, fltksrvrzone, out.str());

			//SearchItem *si = new SearchItem(PQI_SI_SUBTYPE_SEARCH);
			//si -> datatype = PQI_SI_DATATYPE_DIR;
			//si -> data = dir;
			//si -> cid  = c -> cid;
			//server -> handleDirectoryRequest(si);
			//return 1;
		}
	}
	return 1;

}


Fl_Item *process_subdir2(DirNode * dir, Fl_Item *p);
int 	add_person(DirBase *db, Fl_Item *next);
int	add_subdir(DirNode *dir, Fl_Item *p, Fl_Item *next);
int	add_file(PQFileItem *file, Fl_Item *p, Fl_Item *next);

/* must iterate through in parallel */

int fltkserver::check_dirlist2()
{
	// get the list of certificates.
	// get the dirlist.
	std::list<DirBase *> &dirs = server -> getDirList();
	std::list<DirBase *>::iterator dit;
	
	//Fl_Item *p = NULL;
	Fl_Item *c = Fl_Item::first;
	bool init = true; // needed to check first item (as items loop)

	/* cannot copy Fl_Item::last, as it can change with item addition! */
	for(dit = dirs.begin(); dit != dirs.end() && (c); dit++)
	{
		if (init)
			init = false;

		Person *prsn = (*dit)->p;
		std::string name1;
		if (prsn)
			name1 = prsn -> Name();
		else
			name1 = "LOCAL";

		std::string name2(c->name());

		{
			std::stringstream out;
			out << "Person: " << name1;
			out << " vs Item: " << name2 << std::endl;
	  		pqioutput(PQL_DEBUG_BASIC, fltksrvrzone, out.str());
		}

		if (name1 != name2)
		{
	  		pqioutput(PQL_DEBUG_BASIC, fltksrvrzone, "MISMATCH!");
			// more complete check ... see if its there.
			// search through all the next ones.... see if its there.
			Fl_Item *c2 = c;
			bool found = false;
			for(; (c2) && (!found); c2=c2->next)
			{
				std::ostringstream out;
				out << "Iterating through All People... " << c2->name();
	  			pqioutput(PQL_DEBUG_BASIC, fltksrvrzone, out.str());

				if ((c2->parent == NULL) &&
					(name1 == std::string(c2->name())))
				{
					found = true;
				}
			}
			if (found)
			{
				// remove current Fl_Item *....
				std::ostringstream out;
				out << "Found PERSON LATER - Should Delete Stuff!";
	  			pqioutput(PQL_DEBUG_BASIC, fltksrvrzone, out.str());

				// delete stuff inbetween. c -> before c2.
				Fl_Item *c3 = NULL;
				for(; c != c2; c = c3)
				{
					delete_children(c);
					c3 = c -> next;
					c->remove();
					delete c;
				}

				// delete subdir.
				// continue.
				c = process_subdir2(*dit, c2);
			}
			else
			{
				pqioutput(PQL_DEBUG_BASIC, fltksrvrzone, "Should Add PERSON");
				// add person before current item.
				// no increment of c.
				add_person(*dit, c); /* add before c */
			}
		}
		else
		{
			c = process_subdir2(*dit, c);
		}
	}
	if (dit != dirs.end())
	{
		/* add in extras */
		std::ostringstream out;
		out << "At end of Fl_Items" << std::endl;
		out << "Should Add Extra PERSONS" << std::endl;
	  	pqioutput(PQL_DEBUG_BASIC, fltksrvrzone, out.str());

		for(; dit != dirs.end(); dit++)
		{
			add_person(*dit, NULL); /* add at end! */
		}
	}
	else if (c)
	{
		/* should remove any extra files */
		std::ostringstream out;
		out << "At end of Persons, but still Fl_Items" << std::endl;
		out << "Should Remove an Extra People" << std::endl;
	  	pqioutput(PQL_DEBUG_BASIC, fltksrvrzone, out.str());

		Fl_Item *c2 = NULL;
		for(; c; c = c2)
		{
			delete_children(c);
			c2 = c -> next;
			c->remove();
			delete c;
		}
	}
	return 1;
}

Fl_Item	*process_subdir2(DirNode * dir, Fl_Item *p)
{
	std::list<PQFileItem *> &files = dir->files;
        std::list<DirNode *> &dirs = dir->subdirs;
	std::list<PQFileItem *>::iterator fit;
        std::list<DirNode *>::iterator dit;

	{
	  std::ostringstream out;
	  out  << "process_subdir2(" << dir -> name << ")" << std::endl;
	  pqioutput(PQL_DEBUG_BASIC, fltksrvrzone, out.str());
	}

	Fl_Item *c = p -> next;
	
	/* cannot copy Fl_Item::last, as it can change with item addition! */
	for(dit = dirs.begin(); dit != dirs.end() && (c); dit++)
	{
		{
	  	  std::ostringstream out;
		  out << "Directory: " << (*dit)->name;
		  out << " vs Item: " << c->name();
	   	  pqioutput(PQL_DEBUG_BASIC, fltksrvrzone, out.str());
		}

		if ((*dit)->name != std::string(c->name()))
		{
	   	  	pqioutput(PQL_DEBUG_BASIC, fltksrvrzone, "MISMATCH!");
			// more complete check ... see if its there.
			// search through all the next ones.... see if its there.
			// if we find dir further on....
				// delete Fl_Items.... (obviously deleted from Dirlist)
			// else
				// add Directory (missing)
			bool found = false;
			bool child = true;
			Fl_Item *c2 = c;
			for(; (c2) && (!found) && (child); c2=c2->next)
			{
				{
	  			  std::ostringstream out;
				  out << "Iterating through Directories... " << c2->name();
				  out << std::endl;
	   	  		  pqioutput(PQL_DEBUG_BASIC, fltksrvrzone, out.str());
				}

				// check if it's a child.
				Fl_Item *p2 = c2->parent;
				while((p2 != NULL) && (p2 != p))
				{
					p2 = p2 -> parent;
				}

				// if no longer children.
				if (p2 != p)
				{
	   	  		  	pqioutput(PQL_DEBUG_BASIC, fltksrvrzone, "End of Children ");
					child = false;
				}

				if ((c2->parent == p) &&
					((*dit)->name == std::string(c2->name())))
				{
					found = true;
				}
			}
			if (found)
			{
				// remove current Fl_Item *....
	   	  		pqioutput(PQL_DEBUG_BASIC, fltksrvrzone, 
					"Found DIR LATER - Should Delete Stuff!");
				// delete stuff inbetween. c -> before c2.
				Fl_Item *c3 = NULL;
				for(; c != c2; c = c3)
				{
					delete_children(c);
					c3 = c -> next;
					c->remove();
					delete c;
				}
				// continue.
				c = process_subdir2(*dit, c2);
			}
			else
			{
	   	  		pqioutput(PQL_DEBUG_BASIC, fltksrvrzone, 
					"Should Add DIR");
				// add subdir.... before c, and processdir.
				// no increment of c.

				if (c->parent==p)
				{
					add_subdir(*dit, p, c);
				}
				else
				{
					add_subdir(*dit, p, NULL);
				}
			}
		}
		else
		{
			c = process_subdir2(*dit, c);
		}
	}
	if (dit != dirs.end())
	{
		/* add in extras */
	   	pqioutput(PQL_DEBUG_BASIC, fltksrvrzone, 
			"At end of Fl_Items, Should Add Extra DIRS");
		for(; dit != dirs.end(); dit++)
		{
			add_subdir(*dit, p, NULL);
		}
	}
	// if same parent, and not a file....
	else if ((c) && (c->parent == p) && (!dynamic_cast<Fl_File_Item *>(c)))
	{
		
		/* should remove any extra files */
	   	pqioutput(PQL_DEBUG_BASIC, fltksrvrzone, 
			"At end of Dirs, but still some, Should Remove Extra DIRS");

		Fl_Item *c2 = NULL;
		for(; (c) && (c->parent==p) && (!dynamic_cast<Fl_File_Item *>(c)); c = c2)
		{
			delete_children(c);
			c2 = c -> next;
			c->remove();
			delete c;
		}
	}


	for(fit = files.begin(); fit != files.end() && (c); fit++)
	{
		std::ostringstream out;
		out << (*fit)->name;
		out << "   --->   " << (*fit)->size << " bytes";

		{
		  std::ostringstream out;
		  out << "File: " << out.str();
		  out << " vs Item: " << c->name() << std::endl;
	   	  pqioutput(PQL_DEBUG_BASIC, fltksrvrzone, out.str());
		}

		Fl_File_Item *fi = dynamic_cast<Fl_File_Item *>(c);
		if ((fi == NULL) || ((*fit)->name != fi->filename))
		{
	   		pqioutput(PQL_DEBUG_BASIC, fltksrvrzone, "MISMATCH!");
			// more complete check ... see if its there.
			
			// search through all the next ones.... see if its there.
			// if we find dir further on....
				// delete Fl_Items.... (obviously deleted from Dirlist)
			// else
				// add Directory (missing)
			bool found = false;
			bool child = true;
			Fl_Item *c2 = c;
			for(; (c2) && (!found) && (child); c2=c2->next)
			{
				{
	  			  std::ostringstream out;
				  out << "Iterating through Files... " << c2->name();
				  out << std::endl;
	   	  		  pqioutput(PQL_DEBUG_BASIC, fltksrvrzone, out.str());
				}

				// check if it's a child.
				Fl_Item *p2 = c2->parent;
				while((p2 != NULL) && (p2 != p))
				{
					p2 = p2 -> parent;
				}

				// if no longer children.
				if (p2 != p)
				{
	   	  		  	pqioutput(PQL_DEBUG_BASIC, fltksrvrzone, "End of Children ");
					child = false;
				}

				if ((c2->parent == p) &&
					((*fit)->name == std::string(c2->name())))
				{
					found = true;
				}
			}
			if (found)
			{
				// remove current Fl_Item *....
	   	  		pqioutput(PQL_DEBUG_BASIC, fltksrvrzone, 
					"Found File LATER - Should Delete Stuff!");

				// delete stuff inbetween. c -> before c2.
				Fl_Item *c3 = NULL;
				for(; c != c2; c = c3)
				{
					delete_children(c);
					c3 = c -> next;
					c->remove();
					delete c;
				}
				// continue.
				c = c2 -> next;
			}
			else
			{
				// add file before current.
				// no increment of c.
	   	  		pqioutput(PQL_DEBUG_BASIC, fltksrvrzone, 
					"Should Add FILE");
				if (c->parent==p)
				{
					add_file(*fit, p, c);
				}
				else
				{
					add_file(*fit, p, NULL);
				}
			}

		}
		else
		{
			// iterate.
			c=c->next;
		}
	}
	if (fit != files.end())
	{
		/* add in extras */
	   	pqioutput(PQL_DEBUG_BASIC, fltksrvrzone, 
			"At end of Fl_Items, Should Add Extra FILES");
		for(; fit != files.end(); fit++)
		{
			add_file(*fit, p, NULL);
		}
	}
	else if ((c) && (c->parent == p))
	{
		/* should remove any extra files */
	   	pqioutput(PQL_DEBUG_BASIC, fltksrvrzone, 
			"At end of Files, but still some, Should Remove Extra File");

		Fl_Item *c2 = NULL;
		for(; (c) && (c->parent==p) ; c = c2)
		{
			delete_children(c);
			c2 = c -> next;
			c->remove();
			delete c;
		}
	}

	return c;
}



int 	add_person(DirBase *db, Fl_Item *next)
{
	Fl_Item *p = NULL;

	Fl_Item *di = NULL;
	if (db->p)
	{
		di = Fl_Person_Item_type.make(p);
		{
		  std::ostringstream out;
		  out << "add_person() :" << db->p->Name().c_str();
	   	  pqioutput(PQL_DEBUG_BASIC, fltksrvrzone, out.str());
		}

		di->name(db->p->Name().c_str());
		// put proper link in.
		((Fl_Person_Item *) di) -> person_hash = 
			((cert *) (db->p)) -> certificate -> name;
	}
	else
	{
	   	pqioutput(PQL_DEBUG_BASIC, fltksrvrzone, 
			"add_person() : SHOULD ADD LOCAL");
	}

	if ((next) && (di))
	{
	   	pqioutput(PQL_DEBUG_BASIC, fltksrvrzone, 
			"add_person() : SHIFTING");

		di -> remove();
		di -> insert(next);
	}

	if (di)
	{
		process_subdir2(db, di);
	}
	return 1;
}

int	add_subdir(DirNode *dir, Fl_Item *p, Fl_Item *next)
{
	std::cerr << std::endl;
	{
	  std::ostringstream out;
	  out << "add_subdir() :" << dir->name;
	  pqioutput(PQL_DEBUG_BASIC, fltksrvrzone, out.str());
	}

	/* add subdir with parent p, before next */
	Fl_Item *di = Fl_Dir_Item_type.make(p);
	di -> name(dir->name.c_str());

	if (next)
	{
	   	pqioutput(PQL_DEBUG_BASIC, fltksrvrzone, 
			"add_subdir() : SHIFTING");

		/* must move */
		di->remove();
		di->insert(next);
	}

	/* then subprocess. */
	process_subdir2(dir, di);
	return 1;
}


std::string sizeToString(int size);

int	add_file(PQFileItem *file, Fl_Item *p, Fl_Item *next)
{
	{
	  std::ostringstream out;
	  out << "add_file() :" << file->name;
	  pqioutput(PQL_DEBUG_BASIC, fltksrvrzone, out.str());
	}

	/* add file with parent p, before next */
	std::ostringstream out;
	out << "[ " << sizeToString(file->size) << " ] " << file->name;
	//out << "   --->   " << file->size << " bytes";

	Fl_Item *fi = Fl_File_Item_type.make(p);
	fi -> name(out.str().c_str());
	((Fl_File_Item *) fi) -> filename = file -> name;
	((Fl_File_Item *) fi) -> size     = file -> size;

	if (next)
	{
	  	pqioutput(PQL_DEBUG_BASIC, fltksrvrzone, 
			"add_file() SHIFTING:");

		/* must move */
		fi->remove();
		fi->insert(next);
	}

	return 1;
}





int fltkserver::cert_sign_current()
{
	/* get the current cert, check that its
	 * in the allowed group.
	 */

	/* ask sslroot to sign certificate
	 */
	cert *c = cert_get_current();

	if ((c == NULL) || (c == sslr -> getOwnCert()))
	{
		return -1;
	}
	return sslr -> signCertificate(c);
}

int fltkserver::cert_trust_current_toggle()
{
	/* ensure that they are in the allowed group
	 */

	cert *c = cert_get_current();
	if ((c == NULL) || (c == sslr -> getOwnCert()))
	{
		return -1;
	}
	return sslr -> trustCertificate(c, !(c -> Trusted()));
}


#ifdef PQI_USE_CHANNELS

int fltkserver::update_channels()
{
	if (server == NULL)
		return -1;

	// draw the channels

	Fl_Funky_Browser *chb = ui -> channel_list;
	Fl_Funky_Browser *chmb = ui -> channel_file_list;
	Fl_Funky_Browser *chfb = ui -> chan_msgdetails_filelist;

	bool channels_changed = false;
	bool channel_msgs_changed = false;

	// save data on currently selected.
	DisplayData *selChannel = chb -> getCurrentItem(); // getSelected() returns NULL??? why;
	channelSign selChanSign;
	ChanDisItem *selCDI = NULL;

	if (selChannel)
	{
		selChanSign = ((ChanDisItem *) selChannel) -> cs;
	 	std::ostringstream out;
		out << "fltkserver::update_channels() Currently Selected is:";
		selChanSign.print(out);
	        pqioutput(PQL_DEBUG_BASIC, fltksrvrzone, out.str());
	}

	if ((server -> channelsChanged.Changed(0)) || (!channel_list_ok))
	{
		std::list<pqichannel *> chanlist;
		std::list<pqichannel *>::iterator cit;

	        pqioutput(PQL_DEBUG_BASIC, fltksrvrzone,
	 		"fltkserver::update_channels() (server->channelsChanged.Changed(0))");

#ifdef PQI_USE_CHANNELS
		server->getAvailableChannels(chanlist);
#endif
		chb -> clear();
		for(cit = chanlist.begin(); cit != chanlist.end(); ++cit)
		{
			ChanDisItem *cdi = new ChanDisItem(*cit);
			chb -> addItemSeries(cdi);

	 		std::ostringstream out;
			out << "fltkserver::update_channels() Added: ";
			cdi->cs.print(out);

			/* check if previously selected channel */
			if ((selChannel) && (selChanSign == cdi -> cs))
			{
				selCDI = cdi;
				out << " Matches Previous Selection!";
			}

	        	pqioutput(PQL_DEBUG_BASIC, fltksrvrzone, out.str());
		}

		chb -> ItemSeriesDone();
		if (selCDI) /* if channel still exists */
		{
	        	pqioutput(PQL_DEBUG_BASIC, fltksrvrzone, "Selecting Previous Channel");
			chb -> selectDD(selCDI);
		}
		channel_list_ok = true;
		channels_changed = true;
	}


	// save data on currently selected.
	DisplayData *selChanMsg = chmb -> getCurrentItem();
	MsgHash selMsgHash;
	ChanMsgDisItem *selCMDI = NULL;

	if (selChanMsg)
	{
		selMsgHash = ((ChanMsgDisItem *) selChanMsg) -> mh;
	}

	if ((channels_changed) || (!channel_msg_list_ok))
	{
	        pqioutput(PQL_DEBUG_BASIC, fltksrvrzone,
	 		"fltkserver::update_channels() (channel_changed) || (!channel_msg_list_ok)");

		if (!selCDI)
		{
			chmb->clear();
			// selCMDI == NULL as well (so empty Msg Display)
	        	pqioutput(PQL_DEBUG_BASIC, fltksrvrzone,
	 			"fltkserver::update_channels() No Channel Selected");
			ui->chan_createmsg_title->value("No Channel Selected!");
			ui->chan_msgdetails_title->value("No Channel Selected!");
		}
		else
		{
			// a channel is selected! 
			// signature is in selChanSign.
			std::list<chanMsgSummary> summarylist;
			std::list<chanMsgSummary>::iterator it;

			/* set the channel names */
			if (selCDI -> mode & 0x040)
			{
				ui->chan_createmsg_title->value(selCDI->name.c_str());
			}
			else
			{
				ui->chan_createmsg_title->value("Publisher Channel Not Selected!");
			}
			ui->chan_msgdetails_title->value(selCDI->name.c_str());

#ifdef PQI_USE_CHANNELS
			server->getChannelMsgList(selChanSign, summarylist);
#endif
			chmb -> clear();
			for(it = summarylist.begin(); it != summarylist.end(); ++it)
			{
				ChanMsgDisItem *cmdi = new ChanMsgDisItem(*it);
				chmb -> addItemSeries(cmdi);

				/* check if previously selected channel */
				if ((selChanMsg) && (selMsgHash == cmdi -> mh))
				{
					selCMDI = cmdi;
				}
			}

			chmb -> ItemSeriesDone();
			if (selCMDI)
			{
				chmb -> selectDD(selCMDI);
			}
		}

		channel_msg_list_ok = true;
		channel_msgs_changed = true;
	}


	/* Finally do the File List .... but need to do gui first */
	// save data on currently selected.
	DisplayData *selChanFile = chfb -> getCurrentItem();
	std::string selName;
	int 	    selSize;
	ChanFileDisItem *selCFDI = NULL;

	if (selChanFile)
	{
		selName = ((ChanFileDisItem *) selChanMsg) -> name;
		selSize = ((ChanFileDisItem *) selChanMsg) -> size;
	}

	if ((channel_msgs_changed) || (!channel_msg_item_ok))
	{
	        pqioutput(PQL_DEBUG_BASIC, fltksrvrzone,
	 		"fltkserver::update_channels() (channel_changed) || (!channel_msg_item_ok)");

		if ((!selCMDI) || (!selCDI))
		{
			chfb->clear();
			// selCMDI == NULL as well (so empty Msg Display)
	        	pqioutput(PQL_DEBUG_BASIC, fltksrvrzone,
	 			"fltkserver::update_channels() No Msg Selected");
		}
		else
		{
			// a channel is selected! 
			// signature is in selChanSign.

			channelMsg *cm = NULL;
#ifdef PQI_USE_CHANNELS
			cm = server->getChannelMsg(selChanSign,selMsgHash);
#endif
			if (!cm)
			{
				channel_msg_item_ok = true;
				chfb -> clear();
				/* should never get here! */
	        		pqioutput(PQL_DEBUG_ALERT, fltksrvrzone,
	 			  "fltkserver::update_channels() Channels not enabled");
				return 1;
			}

			// Fill in the details.

			// the msg.
			Fl_Text_Buffer *buf = ui -> chan_msgdetails_msg -> buffer();
			buf -> text(cm->msg->msg.c_str());

			// the files....
			PQChanItem::FileList::const_iterator it;

			chfb -> clear();
			for(it = cm->msg->files.begin(); it != cm->msg->files.end(); ++it)
			{
				ChanFileDisItem *cfdi = new ChanFileDisItem(it->name, it->size);
				chfb -> addItemSeries(cfdi);

				/* check if previously selected channel */
				if ((selChanFile) && 
					(selName == cfdi -> name) &&
					(selSize == cfdi -> size))
				{
					selCFDI = cfdi;
				}
			}

			chfb -> ItemSeriesDone();
			if (selCMDI)
			{
				chfb -> selectDD(selCFDI);
			}
		}

		channel_msg_item_ok = true;
		// not needed, channel_msg_item_changed = true;
	}

	return 1;
}

#endif


#include <iomanip>

std::string sizeToString(int size)
{
	std::ostringstream out;
	float fsize = size;
	int mag = 1;
	while(fsize > 1000)
	{
		fsize /= 1000;
		mag++;
	}

	out << std::setw(4) << std::setprecision(3) << fsize;
	switch(mag)
	{
		case 1:
			out << "  B";
			break;
		case 2:
			out << " kB";
			break;
		case 3:
			out << " MB";
			break;
		case 4:
			out << " GB";
			break;
		case 5:
			out << " TB";
			break;
		default:
			out << " ??";
			break;
	}
	return out.str();
}


int ensureExtension(std::string &name, std::string def_ext)
{
	/* if it has an extension, don't change */
	int len = name.length();
	int extpos = name.find_last_of('.');

	std::ostringstream out;
	out << "ensureExtension() name: " << name << std::endl;
	out << "\t\t extpos: " << extpos;
	out << " len: " << len << std::endl;

	/* check that the '.' has between 1 and 4 char after it (an extension) */
	if ((extpos > 0) && (extpos < len - 1) && (extpos + 6 > len))
	{
		/* extension there */
		std::string curext = name.substr(extpos, len);
		out << "ensureExtension() curext: " << curext << std::endl;
		std::cerr << out.str();
		return 0;
	}

	if (extpos != len - 1)
	{
		name += ".";
	}
	name += def_ext;

	out << "ensureExtension() added ext: " << name << std::endl;

	std::cerr << out.str();
	return 1;
}



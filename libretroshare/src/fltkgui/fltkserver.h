/*
 * "$Id: fltkserver.h,v 1.15 2007-02-18 21:46:49 rmf24 Exp $"
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




#ifndef MRK_FLTK_PQI_INTERFACE
#define MRK_FLTK_PQI_INTERFACE

#include "server/filedexserver.h"
#include "pqi/pqipersongrp.h"
#include "pqi/pqissl.h"

#include "fltkgui/Fl_Funky_Browser.h"
#include "fltkgui/pqistrings.h"


#include "fltkgui/alertbox.h"
#include "fltkgui/guitab.h"

#include "pqi/p3disc.h"

class fltkserver
{
	public:
	fltkserver();
	~fltkserver();

int	init();
int	run();

	// setup pqissl
int	setuppqissl(filedexserver *fd, pqipersongrp *ph, sslroot *r, UserInterface *u);
int	addAutoDiscovery(p3disc *a) {ad = a; return 1; }

int     addAlerts(alertBox *msg, alertBox *chat);
int 	ownchatmsg(std::string);
int     chatmsg(std::string msg, std::string source);
int     alertmsg(int type, int sev, std::string msg, std::string source);
	

	// flags to indicate if something needs updating.
	// This are public advisory flags that 
	// can be altered by callbacks.
	bool cert_list_ok;
	bool cert_item_ok;
	bool cert_neighbour_list_ok;
	bool cert_neighbour_item_ok;
	bool search_list_ok;
	bool result_list_ok;
	bool result_item_ok;
	bool msg_list_ok;
	bool msg_item_ok;
	bool msg_dialog_ok;

	bool transfer_ok;
	bool transfer_item_ok;
	bool dir_list_ok;
	bool save_dir_ok;
	bool server_set_ok;

	bool transfer_rates_ok;

	// Neighbour fns
cert *  cert_get_current_neighbour();

int	cert_add_neighbour();
int 	neigh_display(cert*);
int     neigh_update_auth();

cert *	cert_get_current();

int	cert_allow_current();
int	cert_deny_current();
int	cert_listen_current();
int	cert_connect_current();
int	cert_remove_current();
int	cert_tag_current();
/* int	cert_save_config(const char *fname); */
int	cert_save_config();
int 	cert_load_gui_config();
int	cert_save_servaddr();
int	cert_saveaddr_connect();
int	cert_check_auto();
int	cert_toggle_auto();
int 	cert_sign_current();
int 	cert_trust_current_toggle();
int     cert_update_auth();

std::string getHomePath();
int	config_remove_dir();
int 	file_select(int);
int	file_updateName();
int	file_completeIO();

int	config_server_update();
int	config_server_change();

int	fselect_type; // what type of file selection is in progress.


int	search_new(); // read words from keyboard.
int	search_download();
int	search_recommend();
int	search_remove();

	// new results display system.
int	update_search_browser();

int	set_recommend(PQFileItem *rec);
PQFileItem *get_recommend();

int 	getnsend_chat();
int	msg_send();
int msg_channel_select();
int 	msg_remove();
int 	msg_reply();
int 	download_recommend();

int transfer_select();
int transfer_cancel();
int transfer_clear();
int transfer_rates();

int 	load_dir(std::string person, std::string dir);
int 	dirlist_download();
int 	dirlist_recommend();

	private:

int	update();

int     update_quick_stats();
int     update_other_stats();

int	update_certs();
	// display certificate details.
int	cert_display(cert*, bool);

int	update_neighbour_certs();
int	update_search();
int	update_msgs();
int 	update_transfer();
int	update_config();

int 	update_dirs();
int 	update_dirlist();
int 	check_dirlist();
int 	check_dirlist2();

#ifdef PQI_USE_CHANNELS

int	update_channels();
bool	channel_list_ok;
bool	channel_msg_list_ok;
bool	channel_msg_item_ok;

#endif


std::list<FileTransferItem *> transfers;

	// pointers to pqissl stuff.
	filedexserver *server;
	pqipersongrp *pqih;
	sslroot *sslr;
	UserInterface *ui;

	p3disc *ad;

	alertBox *msg;
	alertBox *chat;

	// recommendation.
	PQFileItem *recommend;

	// File Import/Export Flags.
	bool fileio_import;

	int loop;  

	Fl_Funky_Browser *search_browser;
};


#define FILE_CHOOSER_IMPORT 	1
#define FILE_CHOOSER_EXPORT 	2
#define FILE_CHOOSER_DIR 	3
#define FILE_CHOOSER_SAVEDIR 	4
#define FILE_CHOOSER_KEY 	5
#define FILE_CHOOSER_CERT 	6

/* Helper function to convert windows paths
 * into unix (ie switch \ to /) for FLTK's file chooser
 */

std::string make_path_unix(std::string winpath);


#endif

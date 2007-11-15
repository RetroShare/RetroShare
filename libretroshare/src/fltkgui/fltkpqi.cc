/*
 * "$Id: fltkpqi.cc,v 1.34 2007-02-19 20:30:05 rmf24 Exp $"
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


#include "dbase/filedex.h"
#include "server/filedexserver.h"
#include "pqi/pqipersongrp.h"
#include "pqi/pqiloopback.h"

#include "fltkgui/guitab.h"
#include "fltkgui/fltkserver.h"

#include <list>
#include <string>
#include <sstream>

// Includes for directory creation.
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
// Conflicts with FLTK def - hope they are the same.
//#include <dirent.h>

// for blocking signals
#include <signal.h>

#include "fltkgui/alertbox.h"

#include "pqi/pqidebug.h"

// key callback functions.

void gui_update();
void update_connections();
void update_search();
void update_messages();
void update_about();
void update_files();


// Global Pointers for the callback functions.
fltkserver *fserv;
UserInterface *ui;

alertBox *chatbox;
alertBox *alertbox;

// initial configuration bootstrapping...
static const std::string config_init_file = "default_cert.txt";
static const std::string config_file = "config.rs";
static const std::string cert_dir = "friends";
static const std::string key_dir = "keys";
static const std::string ca_file = "cacerts.pem";

static std::string config_basedir;
static std::string load_cert;
static std::string load_key;

static std::string load_trustedpeer_file; 
static bool	   load_trustedpeer = false;

static bool	   firsttime_run = false;

int	create_configinit();
int	check_create_directory(std::string dir);
void 	load_check_basedir();
void 	clear_passwds();



/******************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS
static const char dirSeperator = '/'; // For unix.

// standard start for unix.
int main(int argc, char **argv)
{

	// setup debugging for desired zones.
	setOutputLevel(PQL_WARNING); // default to Warnings.

	// For Testing purposes.
	// We can adjust everything under Linux.
	//setZoneLevel(PQL_DEBUG_BASIC, 38422); // pqipacket.
	//setZoneLevel(PQL_DEBUG_BASIC, 96184); // pqinetwork;
	//setZoneLevel(PQL_DEBUG_BASIC, 82371); // pqiperson.
	//setZoneLevel(PQL_DEBUG_BASIC, 60478); // pqitunnel.
	//setZoneLevel(PQL_DEBUG_BASIC, 34283); // pqihandler.
	//setZoneLevel(PQL_DEBUG_BASIC, 44863); // discItems.
	//setZoneLevel(PQL_DEBUG_BASIC, 2482); // p3disc
	//setZoneLevel(PQL_DEBUG_BASIC, 1728); // pqi/p3proxy
	//setZoneLevel(PQL_DEBUG_BASIC, 1211); // sslroot.
	//setZoneLevel(PQL_DEBUG_BASIC, 37714); // pqissl.
	//setZoneLevel(PQL_DEBUG_BASIC, 8221); // pqistreamer.
	//setZoneLevel(PQL_DEBUG_BASIC,  9326); // pqiarchive
	//setZoneLevel(PQL_DEBUG_BASIC, 3334); // p3channel.
	//setZoneLevel(PQL_DEBUG_BASIC, 354); // pqipersongrp.
	//setZoneLevel(PQL_DEBUG_BASIC, 6846); // pqiudpproxy
	//setZoneLevel(PQL_DEBUG_BASIC, 3144); // pqissludp;
	//setZoneLevel(PQL_DEBUG_BASIC, 86539); // pqifiler.
	//setZoneLevel(PQL_DEBUG_BASIC, 91393); // Funky_Browser.
	//setZoneLevel(PQL_DEBUG_BASIC, 25915); // fltkserver
	//setZoneLevel(PQL_DEBUG_BASIC, 47659); // fldxsrvr
	//setZoneLevel(PQL_DEBUG_BASIC, 49787); // pqissllistener



/******************************** WINDOWS/UNIX SPECIFIC PART ******************/
#else

static const char dirSeperator = '\\'; // For windows.

// Include for the windows system tray...
// the reason for the alternative 
// program start.

#include "retroTray.h"

/* believe this fn exists! */
LPSTR * WINAPI CommandLineToArgvA(LPCSTR,int*);

const int MAX_ARGS = 32;

int WinMain(HINSTANCE hinst, HINSTANCE prevhinst, char *argv_crap, int argc_crap)
{
  int i,j;

  int argc;
  char *argv[MAX_ARGS];
  char *wholeline = GetCommandLine();
  int cmdlen = strlen(wholeline);
  // duplicate line, so we can put in spaces..
  char dupline[cmdlen+1];
  strcpy(dupline, wholeline);

  /* break wholeline down .... 
   * NB. This is very simplistic, and will not 
   * handle multiple spaces, or quotations etc, only for debugging purposes
   */
  argv[0] = dupline;
  for(i = 1, j = 0; (j + 1 < cmdlen) && (i < MAX_ARGS);)
  {
	/* find next space. */
	for(;(j + 1 < cmdlen) && (dupline[j] != ' ');j++);
	if (j + 1 < cmdlen)
	{
		dupline[j] = '\0';
		argv[i++] = &(dupline[j+1]);
	}
  }
  argc = i;
  for( i=0; i<argc; i++)
  {
    printf("%d: %s\n", i, argv[i]);
  }

  setOutputLevel(PQL_WARNING); // default to Warnings.

#endif
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/


	std::list<std::string> dirs;

	short port = 7812; // default port.
	bool forceLocalAddr = false;
	bool haveLogFile    = false;
	bool outStderr      = false;

	char inet[256] = "127.0.0.1";
	char passwd[256] = "";
	char logfname[1024] = "";

	int c;
	bool havePasswd     = false;
	bool haveDebugLevel = false;
	int  debugLevel	= PQL_WARNING;
	bool udpListenerOnly = false;

	while((c = getopt(argc, argv,"i:p:c:sw:l:d:u")) != -1)
	{
		switch (c)
		{
			case 'l':
				strncpy(logfname, optarg, 1024);
				std::cerr << "LogFile (" << logfname;
				std::cerr << ") Selected" << std::endl;
				haveLogFile = true;
				break;
			case 'w':
				strncpy(passwd, optarg, 256);
				std::cerr << "Password Specified(" << passwd;
				std::cerr << ") Selected" << std::endl;
				havePasswd = true;
				break;
			case 'i':
				strncpy(inet, optarg, 256);
				std::cerr << "New Inet Addr(" << inet;
				std::cerr << ") Selected" << std::endl;
				forceLocalAddr = true;
				break;
			case 'p':
				port = atoi(optarg);
				std::cerr << "New Listening Port(" << port;
				std::cerr << ") Selected" << std::endl;
				break;
			case 'c':
				config_basedir = optarg;
				std::cerr << "New Base Config Dir(";
				std::cerr << config_basedir;
				std::cerr << ") Selected" << std::endl;
				break;
			case 's':
				outStderr = true;
				haveLogFile = false;
				std::cerr << "Output to Stderr";
				std::cerr << std::endl;
				break;
			case 'd':
				haveDebugLevel = true;
				debugLevel = atoi(optarg);
				std::cerr << "Opt for new Debug Level";
				std::cerr << std::endl;
				break;
			case 'u':
				udpListenerOnly = true;
				std::cerr << "Opt for only udpListener";
				std::cerr << std::endl;
				break;
			default:
				std::cerr << "Unknown Option!";
				exit(1);
		}
	}


	// set the default Debug Level...
	if (haveDebugLevel)
	{
		if ((debugLevel > 0) && (debugLevel <= PQL_DEBUG_ALL))
		{
			std::cerr << "Setting Debug Level to: " << debugLevel;
			std::cerr << std::endl;
  			setOutputLevel(debugLevel);
		}
		else
		{
			std::cerr << "Ignoring Invalid Debug Level: ";
			std::cerr << debugLevel;
			std::cerr << std::endl;
		}
	}

	// set the debug file.
	if (haveLogFile) 
	{
		setDebugFile(logfname);
	}

/******************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#else
	// As windows doesn't have commandline options - we have to set them...
	// or make them part of prog config.

	/*
	char certloc[256] = "./keys/bobby_cert.pem";
	char pkeyloc[256] = "./keys/bobby_pk.pem";
	*/
	//char inet[256] = "10.0.0.2";

	// only the currnet directory for the moment.
	//dirs.push_back(".");

	// Windows Networking Init.
	WORD wVerReq = MAKEWORD(2,2);
	WSADATA wsaData;

	if (0 != WSAStartup(wVerReq, &wsaData))
	{
		std::cerr << "Failed to Startup Windows Networking";
		std::cerr << std::endl;
	}
	else
	{
		std::cerr << "Started Windows Networking";
		std::cerr << std::endl;
	}

#endif
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/

	// first check config directories, and set bootstrap values.
	load_check_basedir();

	// blank the trustedpeer loading variables.
        load_trustedpeer = false;
        load_trustedpeer_file = "";

	firsttime_run = false;

	// Next - Setup the GUI defaults......
	ui = new UserInterface();
	ui -> make_windows();

	// SWITCH off the SIGPIPE - kills process on Linux.
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS
	struct sigaction sigact;
	sigact.sa_handler = SIG_IGN;
	sigact.sa_flags = 0;

	if (0 == sigaction(SIGPIPE, &sigact, NULL))
	{
		std::cerr << "RetroShare:: Successfully Installed";
		std::cerr << "the SIGPIPE Block" << std::endl;
	}
	else
	{
		std::cerr << "RetroShare:: Failed to Install";
		std::cerr << "the SIGPIPE Block" << std::endl;
	}
#else
	// in windows case, install the system tray.
	std::string iconpath = config_basedir + dirSeperator;
	iconpath += "tst.ico";
	SetIconPath(iconpath.c_str());
	InitialiseRetroTray(hinst, NULL, ui);

#endif
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/

	// add the text_buffers to the correct displays...
	ui -> cert_details -> buffer(new Fl_Text_Buffer(10000));
	ui -> msg_details -> buffer(new Fl_Text_Buffer(10000));
	ui -> msg_text -> buffer(new Fl_Text_Buffer(10000));
	ui -> transfer_overview -> buffer(new Fl_Text_Buffer(10000));
	ui -> chan_createmsg_msg -> buffer(new Fl_Text_Buffer(10000));
	ui -> chan_msgdetails_msg -> buffer(new Fl_Text_Buffer(10000));

	ui -> alert_box -> buffer(new Fl_Text_Buffer(10000));
	ui -> chatter_box -> buffer(new Fl_Text_Buffer(10000));

	//ui -> cert_list -> Fl_Browser::handle(FL_PUSH);
	ui -> cert_list -> when(FL_WHEN_CHANGED);
	ui -> srch_results -> when(FL_WHEN_CHANGED);
	ui -> msg_list -> when(FL_WHEN_CHANGED);
	ui -> cert_list -> setCheck(2);

//	ui -> cert_list -> setTitle(0,"Status");
//	ui -> cert_list -> setTitle(1,"Person");
//	ui -> cert_list -> setTitle(2,"Auto Connect");
//	ui -> cert_list -> setTitle(3,"Server");
//
//
//	ui -> srch_results -> setTitle(0,"Source");
//	ui -> srch_results -> setTitle(1,"KeyWord");
//	ui -> srch_results -> setTitle(2,"FileName");
//	ui -> srch_results -> setTitle(3,"Size");
//
//
//	ui -> msg_list -> setTitle(0,"Source");
//	ui -> msg_list -> setTitle(1,"Message");
//	ui -> msg_list -> setTitle(2,"Date");
//
//	ui -> transfer_downloads -> setTitle(0,"Source");
//	ui -> transfer_downloads -> setTitle(1,"Direction");
//	ui -> transfer_downloads -> setTitle(2,"Filename");
//	ui -> transfer_downloads -> setTitle(3,"Completed");
//	ui -> transfer_downloads -> setTitle(4,"Rate");


	// disable the channel stuff (and recommend stuff) for now!
	ui ->file_channel_button->deactivate();
	ui ->search_channel_button->deactivate();
	//ui ->channels->deactivate();



	chatbox = new chatterBox(ui->chatter_window, ui->chatter_box);
	alertbox = new alertBox(ui->alert_window,ui->alert_box);

	//static int neighbour_widths[3] = { 150, 250, 0 };
	//ui -> cert_neighbour_list -> column_widths(neighbour_widths);
static int neighbour_widths[3] = { 150, 250, 0 };
	ui -> neigh_signers -> column_widths(neighbour_widths);

	ui -> file_chooser -> preview(0);

	// Next bring up the welcome window.....
	// load up the default welcome settings.
	//ui -> load_cert -> value(load_cert.c_str());
	//ui -> load_key -> value(load_key.c_str());
	
	std::string userName;
	bool existingUser = false;
	if (LoadCheckXPGPandGetName(load_cert.c_str(), userName))
	{
		ui -> load_name -> value(userName.c_str());
		existingUser = true;
	}
	else
	{
		ui -> load_name -> value("No Existing User -> Create New");
	}

	//ui -> gen_basename -> value("user");
	
	ui -> welcome_window -> show();
	if (existingUser)
	{
		ui -> load_passwd -> take_focus();
	}
	else
	{
		ui -> gen_name -> take_focus();
	}


	alertbox ->sendMsg(0, 10, "RetroShare GUI Loading.", "");
	alertbox ->sendMsg(0, 10, "RetroShare: Alert Msgs activated", "");

	/* do a null init to allow the SSL libray to startup! */
/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)
	/* do a null init to allow the SSL libray to startup! */
	getSSLRoot() -> initssl(NULL, NULL, NULL); 
#else /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/
	getSSLRoot() -> initssl(NULL, NULL, NULL, NULL); 
#endif /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/

/******************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS
	if (havePasswd)
	{
		ui -> load_passwd -> value(passwd);
		load_existing(NULL, NULL);
		// do we need to shut down the window?
	}
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/
#else // WINDOWS
	// can't load passwd from command line.
#endif

	// wait for this window to be finished....
	while(!(getSSLRoot() -> active()))
	{
		Fl::wait();
	}

	// ssl root is setup already.... by welcome_window.
	// this means that the cert/key + cacerts have been loaded.
	sslroot *sr = getSSLRoot();

/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)
	if (1 != sr -> initssl(NULL, NULL, NULL))
#else /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/
	if (1 != sr -> initssl(NULL, NULL, NULL, NULL))
#endif /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/
	{
		std::cerr << "main() - Fatal Error....." << std::endl;
		std::cerr << "Invalid Certificate configuration!" << std::endl;
		std::cerr << std::endl;
		exit(1);
	}

	/* set the debugging to crashMode */
	if ((!haveLogFile) && (!outStderr))
	{
		std::string crashfile = config_basedir + dirSeperator;
		crashfile += "retro.log";
		setDebugCrashMode(crashfile.c_str());
	}

	alertbox ->sendMsg(0, 10, "RetroShare: Loading", "");

	// set the directories for full configuration load.
	sr -> setConfigDirs(config_basedir.c_str(), cert_dir.c_str());
	sr -> loadCertificates(config_file.c_str());
	sr -> checkNetAddress();

	// filedex server.
	filedexserver server;


#ifdef USE_FILELOOK
        fileLook *fl = new fileLook();
	fl -> start(); /* background look thread */
	server.setFileLook(fl);
#else   /*********************************************************************/
        filedex *fd = new filedex();
        server.setFileDex(fd);
#endif

	server.setConfigDir(config_basedir.c_str());

	SecurityPolicy *none = secpolicy_create();

	if (forceLocalAddr)
	{
		struct sockaddr_in laddr;

		laddr.sin_family = AF_INET;
		laddr.sin_port = htons(port);

		// universal
		laddr.sin_addr.s_addr = inet_addr(inet);
		// unix specific
		//inet_aton(inet, &(laddr.sin_addr));

		cert *own = sr -> getOwnCert();
		if (own != NULL)
		{
			own -> localaddr = laddr;
		}
	}

	/* must load the trusted_peer before setting up the pqipersongrp */
	if (firsttime_run)
	{
		/* at this point we want to load and start the trusted peer -> if selected */
		if (load_trustedpeer)
		{
			/* sslroot does further checks */
        		sr -> loadInitialTrustedPeer(load_trustedpeer_file);
		}
	}

	unsigned long flags = 0;
	if (udpListenerOnly)
	{
		flags |= PQIPERSON_NO_SSLLISTENER;
	}

	pqipersongrp psg(none, sr, flags);
        psg.load_config();
	server.setSearchInterface(&psg, sr);

#ifdef PQI_USE_CHANNELS
	server.setP3Channel(psg.getP3Channel());
#endif

	// create loopback device, and add to pqisslgrp.

	SearchModule *mod = new SearchModule();
	pqiloopback *ploop = new pqiloopback();

	mod -> smi = 1;
	mod -> pqi = ploop;
	mod -> sp = secpolicy_create();

	psg.AddSearchModule(mod);

	//autoDiscovery ad(&psg, sr);
	//ad.load_configuration();
	//ad.localSetup();

	fltkserver ncs;
	fserv = &ncs;

	ncs.setuppqissl(&server, &psg, sr, ui);
	ncs.addAlerts(alertbox, chatbox);

	// Set Default Save Dir. pre-load
	// Load from config will overwrite...
	std::string hdefpath = fserv -> getHomePath();
	server.setSaveDir(hdefpath.c_str());

	// load the GUI browser settings from sslroot.
	ncs.cert_load_gui_config();

        server.load_config();

	ncs.addAutoDiscovery(psg.getP3Disc());

	// load up the help page
	std::string helppage = config_basedir + dirSeperator;
	//helppage += "html";
	//helppage += dirSeperator;
	//helppage += "retro_help.html";
	helppage += "retro.htm";
	//helppage += "index.html";
	//helppage = "/home/rmf24/prog/src/FLTK/fltk-1.1.6/documentation/index.html";

	// Temp remove about page.
	ui -> help_view -> load(helppage.c_str());
	ui -> help_view -> textsize(14);

	alertbox ->sendMsg(0, 10, "RetroShare: Loaded", "");


	/* finally show the main window */
	ui -> main_win -> show();

	if (firsttime_run)
	{
		/* change any gui options that should be setup for the initial run */

		/* set the help to be the default visible tab */
		ui -> gui_tabs->value(ui->about_help_tab);
	}
	else
	{
		/* make connect gui the initial tab */
		ui -> gui_tabs->value(ui->connect_tab);
	}

	ncs.init();
	ncs.run();

	return 1;
}


/* Neighbour Fns.
 */
void cert_neighbour_list_select(Fl_Funky_Browser*, void*)
{
	fserv -> cert_neighbour_item_ok = false;
	std::cerr << "cert_neighbour_list_select() callback!" << std::endl;
	return;
}

void cert_neigh_signers_select(Fl_Browser*, void*)
{
	std::cerr << "cert_neigh_signers_select() callback!" << std::endl;
}

void neigh_auth_callback(Fl_Input*, void*)
{
	fserv -> neigh_update_auth();
	return;
}

/* Cert fns */

void cert_list_select(Fl_Funky_Browser*, void*)
{
	fserv -> cert_item_ok = false;
	// check if this was called by ticking the box.
	fserv -> cert_check_auto();
	std::cerr << "cert_list_select() callback!" << std::endl;
	return;
}

void cert_move_to_friends(Fl_Button*, void*)
{
	fserv -> cert_add_neighbour();
	std::cerr << "cert_move_to_friend() callback!" << std::endl;
	return;
}


void cert_show_config(Fl_Button*, void*)
{
	ui -> cert_config -> show();
	return;
}

void cert_hide_config(Fl_Button*, void*)
{
	ui -> cert_config -> hide();
	return;
}

void cert_show_neighbour_config(Fl_Button*, void*)
{
	//ui -> cert_neighbour_config -> show();
	return;
}

void cert_hide_neighbour_config(Fl_Button*, void*)
{
	//ui -> cert_neighbour_config -> hide();
	return;
}

void cert_allow_change(Fl_Check_Button*, void*)
{
	if (ui -> cert_allow -> value() == 0)
	{
		fserv -> cert_deny_current();
	}
	else
	{
		fserv -> cert_allow_current();
	}
	fserv -> cert_list_ok = false;
	fserv -> cert_item_ok = false;
	return;
}

void cert_listen_change(Fl_Check_Button*, void*)
{
	fserv -> cert_listen_current();
	fserv -> cert_list_ok = false;
	fserv -> cert_item_ok = false;
	return;
}

void cert_connect_change(Fl_Check_Button*, void*)
{
	fserv -> cert_connect_current();
	fserv -> cert_list_ok = false;
	fserv -> cert_item_ok = false;
	return;
}

void cert_auth_callback(Fl_Input*, void*)
{
	fserv -> cert_update_auth();
	return;
}


void cert_save_servaddr(Fl_Button*, void*)
{
	// if success - update gui.
	// else leave for error message.
	if (0 != fserv -> cert_save_servaddr())
	{
		fserv -> cert_item_ok = false;
	}
	return;
}


void cert_save_n_connect(Fl_Button*, void*)
{
	if (0 != fserv -> cert_saveaddr_connect())
	{
		fserv -> cert_item_ok = false;
	}
	return;
}


void cert_save_config(Fl_Button*, void*)
{
	/*
	fserv -> cert_save_config(config_file.c_str());
	*/
	// To allow autosave.
	fserv -> cert_save_config();
	std::cerr << "fserv -> cert_save_config() Done!" << std::endl;
	return;
}


void cert_remove_cert(Fl_Button*, void*)
{
	fserv -> cert_remove_current();
	fserv -> cert_list_ok = false;
	fserv -> cert_item_ok = false;
	return;
}


void cert_trust_person_change(Fl_Check_Button*, void*)
{
	std::cerr << "cert_trust_person_change()" << std::endl;
	/* at this point we ask the sslroot->tosign(current *) */
	fserv -> cert_trust_current_toggle();
	return;
}

void cert_auto_change(Fl_Check_Button*, void*)
{
	std::cerr << "cert_auto_change()" << std::endl;
	fserv -> cert_toggle_auto();
	return;
}


void cert_local_change(Fl_Check_Button*, void*)
{
	std::cerr << "cert_local_change()" << std::endl;
	//fserv -> cert_toggle_auto();
	return;
}


void cert_sign(Fl_Button*, void*)
{
	std::cerr << "cert_sign()" << std::endl;

	/* at this point we ask the sslroot->tosign(current *) */
	fserv -> cert_sign_current();
	return;
}


void msg_send(Fl_Button*, void*)
{
	fserv -> msg_send();
	ui -> msg_dialog -> hide();
	return;
}


void msg_select(Fl_Funky_Browser*, void*)
{
	fserv -> msg_list_ok = false;
	fserv -> msg_item_ok = false;
	return;
}


void msg_toggle_select(Fl_Button*, void*)
{
	return;
}


void msg_dialog_hide(Fl_Button*, void*)
{
	ui -> msg_dialog -> hide();
	return;
}

void msg_dialog_show(Fl_Button*, void*)
{
	ui -> msg_dialog -> show();
	return;
}

void msg_remove(Fl_Button*, void*)
{
	fserv -> msg_remove();
	fserv -> msg_list_ok = false;
	fserv -> msg_item_ok = false;
	return;
}


void msg_dialog_reply(Fl_Button*, void*)
{
	fserv -> msg_reply();
	fserv -> msg_item_ok = false;
	ui -> msg_dialog -> show();
	return;
}


void msg_get_recommendation(Fl_Button*, void*)
{
	fserv -> download_recommend();
	return;
}


void channel_select(Fl_Browser*, void*)
{
	std::cerr << "Channel callback" << std::endl;
	fserv -> msg_channel_select();
}


void transfer_select(Fl_Funky_Browser*, void*)
{
	std::cerr << "Transfer callback" << std::endl;
	//fserv -> transfer_select();
}

void file_transfer_cancel(Fl_Button*, void*)
{
	std::cerr << "File Transfer Cancel callback" << std::endl;
	fserv -> transfer_cancel();
}

void file_transfer_clear(Fl_Button*, void*)
{
	std::cerr << "File Transfer Clear callback" << std::endl;
	fserv -> transfer_clear();
}

void file_transfer_total_rate(Fl_Counter*, void*)
{
	std::cerr << "File Transfer Total Rate callback" << std::endl;
	fserv -> transfer_rates();
}


void file_transfer_indiv_rate(Fl_Counter*, void*)
{
	std::cerr << "File Transfer Individual Rate callback" << std::endl;
	fserv -> transfer_rates();
}


void file_transfers_max(Fl_Counter*, void*)
{
	std::cerr << "File Transfer Max callback" << std::endl;
	fserv -> transfer_rates();
}


void do_new_search(Fl_Input *, void*)
{
	fserv -> search_new();
	return;
}

void do_search_button(Fl_Button *, void*)
{
	fserv -> search_new();
	return;
}

void search_remove(Fl_Button*, void*)
{
	fserv -> search_remove();
	return;
}

void search_result_select(Fl_Funky_Browser*, void*)
{
	fserv -> result_item_ok = false;
	std::cerr << "search_result_select() callback!" << std::endl;
	return;
}

void search_download(Fl_Button*, void*)
{
	fserv -> search_download();
	return;
}

void search_recommend(Fl_Button*, void*)
{
	fserv -> search_recommend();
	return;
}

void gui_hide(Fl_Button*, void*);

void gui_quit(Fl_Button*a, void*b)
{
	gui_hide(a,b);
	//exit(1);
	return;
}

void gui_hide(Fl_Button*, void*)
{
	// Temp use to shown chatter window.
	// ui -> chatter_window -> show();
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS
	// iconize.
	ui -> main_win -> iconize();
#else
	// windows we can hide (cos the system tray is still there)
	ui -> main_win -> hide();
#endif
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/
	return;
}

void file_import(Fl_Button*, void*)
{
	fserv -> file_select(FILE_CHOOSER_IMPORT);
	ui -> file_chooser -> show();
	return;
}


void file_export(Fl_Button*, void*)
{
	fserv -> file_select(FILE_CHOOSER_EXPORT);
	ui -> file_chooser -> show();
	return;
}

/* Have to fix buggy Fl_File_Chooser Code.....
 * Forcing the use of Enter Button.
 *
 *
 * if valid selection..... + Enter.... 
 * do I/O
 *
 * else add timeout.
 * 	and unhide.....
 */

void fc_timeout(void *);

void file_chooser_select(Fl_File_Chooser *, void*v)
{
	std::cerr << "file_chooser_select()" << std::endl;
	if ((Fl::event_key() == FL_Enter) && 
		(NULL != ui -> file_chooser -> value()))
	{
		std::cerr << "file_chooser_select() Hit Enter (With Val)!" << std::endl;
		fserv -> file_completeIO();
		ui -> file_chooser -> hide();
		return;
	}

	// Else add timeout callback ....
	Fl::add_timeout(0.5, fc_timeout);

	//fserv -> file_completeIO();
	//ui -> file_chooser -> hide();
	

}


void fc_timeout(void *)
{
	std::cerr << "fc_timeout()" << std::endl;

	// ignore timeout if still shown...
	if (ui -> file_chooser -> shown())
	{
		std::cerr << "Visible - Ignoring fc_timeout()" << std::endl;
		return;
	}

	// if valid selection.... do it.
	if (NULL != ui -> file_chooser -> value())
	{
		std::cerr << "Hidden + Value - Do I/O" << std::endl;
		fserv -> file_completeIO();
		return;
	}
	// else reshow the window,
	std::cerr << "Hidden + No Value - re-Show" << std::endl;
	ui -> file_chooser -> show();
	return;
}


void config_server_update(Fl_Check_Button*, void*)
{
	// save the settings.
	fserv -> config_server_update();
	return;
}


void config_server_change(Fl_Button*, void*)
{
	// save the settings.
	fserv -> config_server_change();
	return;
}


void config_save_dir_change(Fl_Button*, void*)
{
	fserv -> file_select(FILE_CHOOSER_SAVEDIR);
	ui -> file_chooser -> show();
	// save the settings.
	return;
}


void config_add_dir(Fl_Button*, void*)
{
	fserv -> file_select(FILE_CHOOSER_DIR);
	ui -> file_chooser -> show();
	// save the settings.
	return;
}

void config_remove_dir(Fl_Button*, void*)
{
	fserv -> config_remove_dir();
	return;
}




void load_passwd_callback(Fl_Input*, void*)
{
	load_existing(NULL, NULL);
	return;
}


void load_existing(Fl_Button*, void*)
{
	std::string cert_loc = load_cert; //ui -> load_cert -> value();
	std::string key_loc = load_key; //ui -> load_key -> value();
	std::string passwd = ui -> load_passwd -> value();

	// hack to use the generate passwd.
	if (passwd == "")
	{
		passwd =  ui -> gen_passwd -> value();
	}

	if (cert_loc == "")
	{
	  alertbox ->sendMsg(0, 0, "RetroShare needs a certificate", "");
	  return;
	}
	if (key_loc == "")
	{
	  alertbox ->sendMsg(0, 0, "RetroShare needs a Key", "");
	  return;
	}
	if (passwd == "")
	{
	  alertbox ->sendMsg(0, 0, "RetroShare needs a passwd", "");
	  return;
	}

	std::string std_key_dir = config_basedir + dirSeperator;
	std_key_dir += key_dir;

	// check that the key file exists....., 
	// and if it is not in the configuration directory - copy it there.


	std::string ca_loc = std_key_dir + dirSeperator;
	ca_loc += ca_file;

	sslroot *sr = getSSLRoot();

/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)
	if (0 < sr -> initssl(cert_loc.c_str(), key_loc.c_str(), 
				passwd.c_str()))
#else /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/
	if (0 < sr -> initssl(cert_loc.c_str(), key_loc.c_str(), 
				ca_loc.c_str(), passwd.c_str()))
#endif /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/

	{
		ui -> welcome_window -> hide();
		clear_passwds();

		// success..... save the cert + key locations.
		// as the keys are good we can create a initfile
		load_cert = cert_loc;
		load_key = key_loc;

		create_configinit();

		return;
	}

	// else clean up ....
	alertbox ->sendMsg(0, 0, "RetroShare Failed To Start!","");
	alertbox ->sendMsg(0, 0, "Please Check File Names/Password","");

	return;
}

void generate_certificate(Fl_Button *button, void *nuffing)
{
	// In the XPGP world this is easy...
	// generate the private_key / certificate.
	// save to file.
	//
	// then load as if they had entered a passwd.

	// check basename exists.
	// check that passwords match.
	if ((strlen(ui -> gen_passwd -> value()) < 4) ||	
		(0 != strcmp(ui -> gen_passwd -> value(), 
			ui -> gen_passwd2 -> value())))
	{
		std::ostringstream out;
		out << "RetroShare cannot generate your";
		out << "Certficate because:" << std::endl;
		out << "\tPassword is Unsatisfactory..." << std::endl;
		out << "\tMust be 4+ chars and entered twice" << std::endl;
		alertbox ->sendMsg(0, 0, out.str(), "");
		return;
	}

	if (strlen(ui -> gen_name -> value()) < 3) 	
	{
		std::ostringstream out;
		out << "RetroShare cannot generate your";
		out << "Certficate because:" << std::endl;
		out << "\tThe Name is bad..." << std::endl;
		out << "\tMust be 3+ chars" << std::endl;
		alertbox ->sendMsg(0, 0, out.str(), "");
		return;
	}

	int nbits = 2048;

	// Create the filename.
	std::string basename = config_basedir;
	basename += dirSeperator;
	basename += key_dir + dirSeperator;
	basename += "user"; //ui -> gen_basename -> value();

	std::string key_name = basename + "_pk.pem";
	std::string cert_name = basename + "_cert.pem";


/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)
	if (!generate_xpgp(cert_name.c_str(), key_name.c_str(), 
			ui -> gen_passwd2 -> value(),
			ui -> gen_name -> value(), 
			"", //ui -> gen_email -> value(), 
			ui -> gen_org -> value(), 
			ui -> gen_loc -> value(), 
			"", //ui -> gen_state -> value(), 
			ui -> gen_country -> value(), 
			nbits))
#else /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/
	/* UNTIL THIS IS FILLED IN CANNOT GENERATE X509 REQ */
#endif /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/
	{
		std::ostringstream out;
		out << "RetroShare cannot generate your";
		out << "Certficate: UNKNOWN REASON!" << std::endl;
		alertbox ->sendMsg(0, 0, out.str(), "");
		return;
	}


	/* set the load passwd to the gen version 
	 * and try to load it!
	 */

	//ui -> load_cert -> value(cert_name.c_str());
	//ui -> load_key -> value(key_name.c_str());
	load_cert = cert_name.c_str();
	load_key  = key_name.c_str();
	//ui -> load_passwd -> value(ui -> load_passwd -> value());

	{
		std::ostringstream out;
		out << "RetroShare has Successfully generated";
		out << "a Certficate/Key" << std::endl;
		out << "\tCert Located: " << cert_name << std::endl;
		out << "\tLocated: " << key_name << std::endl;
		alertbox ->sendMsg(0, 10, out.str(), "");
	}

	/* now if the user has selected to load a trusted certificate
	 * we must save the details to we can load it in a second.
	 *
	 * load_trustedpeer is taken from the gen_trusted_tick_box,
	 * while load_trusted_cert(Fl_Button*, void*) sets the load_trustedpeer_file string.
	 */

	if (1 == ui -> gen_trusted_tick_box -> value())
	{
		load_trustedpeer = true;
	}
	else
	{
		load_trustedpeer = false;
	}

	// Also set the first time flag.
	firsttime_run = true;

	/* call standard load ! */
	load_existing(button, nuffing);

	return;
}


void load_trusted_cert(Fl_Button*, void*)
{
	Fl_File_Chooser *fc = new Fl_File_Chooser("",
		"Certificate File (*.{pqi})", 0, "Trusted Friend Selection");
	fc -> show();
	while(fc -> shown())
		Fl::wait();

	/* if we are successful, 
	 * 1) load certificate.... if that is successful...
	 * 2) display name, 
	 * 3) and tick box..
	 */

	bool valid = false;
	bool trustedFriend = false;

	std::string userName;
	if (fc->value())
	{
		valid = true;
		load_trustedpeer_file = fc -> value();
	}

	if ((valid) && (LoadCheckXPGPandGetName(load_trustedpeer_file.c_str(), userName)))
	{
		ui -> gen_trusted_peer -> value(userName.c_str());
		ui -> gen_trusted_tick_box -> value(1);
		trustedFriend = true;
	}
	else
	{
		load_trustedpeer_file = "";
		if (valid)
		{
			ui -> gen_trusted_peer -> value("-Bad-File-");
		}
		else
		{
			ui -> gen_trusted_peer -> value("");
		}
		ui -> gen_trusted_tick_box -> value(0);
	}

	delete fc;
	return;
}


void gen_trusted_tick_callback(Fl_Check_Button*, void*)
{
	std::cerr << "gen_trusted_tick_callback()";
	std::cerr << std::endl;
	if (ui -> gen_trusted_tick_box -> value())
	{
		load_trusted_cert(NULL, NULL);
	}
	else
	{
		ui -> gen_trusted_peer -> value("");
	}

	return;
}

void gen_load_trusted(Fl_Button*, void*)
{
	std::cerr << "gen_load_trusted()";
	std::cerr << std::endl;

	load_trusted_cert(NULL, NULL);
	return;
}


void load_cert_change(Fl_Button*, void*)
{
	std::string std_key_dir = config_basedir + dirSeperator;
	std_key_dir += key_dir;
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifdef WINDOWS_SYS /* ONLY FOR WINDOWS */
	std_key_dir = make_path_unix(std_key_dir);
#endif
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/


	Fl_File_Chooser *fc = new Fl_File_Chooser(std_key_dir.c_str(),
		"Certificate File (*.{pem})", 0, "Certificate Selection");
	fc -> show();
	while(fc -> shown())
		Fl::wait();

	//ui -> load_cert -> value(fc -> value());
	delete fc;
	return;
}

void load_key_change(Fl_Button*, void*)
{
	std::string std_key_dir = config_basedir + dirSeperator;
	std_key_dir += key_dir;
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifdef WINDOWS_SYS /* ONLY FOR WINDOWS */
	std_key_dir = make_path_unix(std_key_dir);
#endif
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/

	Fl_File_Chooser *fc = new Fl_File_Chooser(std_key_dir.c_str(),
		"Private Key File (*.{pem})", 0, "Key Selection");
	fc -> show();
	while(fc -> shown())
		Fl::wait();


	//ui -> load_key -> value(fc -> value());
	delete fc;
	return;
}



void clear_passwds()
{
	ui -> load_passwd -> value("UNKNOWN.....0");
	ui -> gen_passwd ->  value("UNKNOWN.....1");
	ui -> gen_passwd2 -> value("UNKNOWN.....2");
}


void load_check_basedir()
{
	// get the default configuration location.
	
	if (config_basedir == "")
	{
		// if unix. homedir + /.pqiPGPrc
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS
		char *h = getenv("HOME");
		std::cerr << "fltkpqi::basedir() -> $HOME = ";
		std::cerr << h << std::endl;
		if (h == NULL)
		{
			std::cerr << "load_check_basedir() Fatal Error --";
		  	std::cerr << std::endl;
			std::cerr << "\tcannot determine $HOME dir" <<std::endl;
			exit(1);
		}
		config_basedir = h;
		config_basedir += "/.pqiPGPrc";
#else
		char *h = getenv("APPDATA");
		std::cerr << "fltkpqi::basedir() -> $APPDATA = ";
		std::cerr << h << std::endl;
		char *h2 = getenv("HOMEDRIVE");
		std::cerr << "fltkpqi::basedir() -> $HOMEDRIVE = ";
		std::cerr << h2 << std::endl;
		char *h3 = getenv("HOMEPATH");
		std::cerr << "fltkpqi::basedir() -> $HOMEPATH = ";
		std::cerr << h3 << std::endl;
		if (h == NULL)
		{
			// generating default
			std::cerr << "load_check_basedir() getEnv Error --Win95/98?";
		  	std::cerr << std::endl;

			config_basedir="C:\\Retro";

		}
		else
		{
			config_basedir = h;
		}

		check_create_directory(config_basedir);
		config_basedir += "\\RetroShare";
#endif
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/
	}


	std::string subdir1 = config_basedir + dirSeperator;
	std::string subdir2 = subdir1;
	subdir1 += key_dir;
	subdir2 += cert_dir;

	// fatal if cannot find/create.
	std::cerr << "Checking For Directories" << std::endl;
	check_create_directory(config_basedir);
	check_create_directory(subdir1);
	check_create_directory(subdir2);

	// have a config directories.
	
	// Check for config file.
	std::string initfile = config_basedir + dirSeperator;
	initfile += config_init_file;

	// open and read in the lines.
	FILE *ifd = fopen(initfile.c_str(), "r");
	char path[1024];
	int i;

	if (ifd != NULL)
	{
		if (NULL != fgets(path, 1024, ifd))
		{
			for(i = 0; (path[i] != '\0') && (path[i] != '\n'); i++);
			path[i] = '\0';
			load_cert = path;
		}
		if (NULL != fgets(path, 1024, ifd))
		{
			for(i = 0; (path[i] != '\0') && (path[i] != '\n'); i++);
			path[i] = '\0';
			load_key = path;
		}
		fclose(ifd);
	}

	// we have now 
	// 1) checked or created the config dirs.
	// 2) loaded the config_init file - if possible.
	return;
}

int	create_configinit()
{
	// Check for config file.
	std::string initfile = config_basedir + dirSeperator;
	initfile += config_init_file;

	// open and read in the lines.
	FILE *ifd = fopen(initfile.c_str(), "w");
/*
	char path[1024];
	int i;
*/

	if (ifd != NULL)
	{
		fprintf(ifd, "%s\n", load_cert.c_str());
		fprintf(ifd, "%s\n", load_key.c_str());
		fclose(ifd);

		std::cerr << "Creating Init File: " << initfile << std::endl;
		std::cerr << "\tLoad Cert: " << load_cert << std::endl;
		std::cerr << "\tLoad Key: " << load_key << std::endl;

		return 1;
	}
	std::cerr << "Failed To Create Init File: " << initfile << std::endl;
	return -1;
}


int	check_create_directory(std::string dir)
{
	struct stat buf;
	int val = stat(dir.c_str(), &buf);
	if (val == -1)
	{
		// directory don't exist. create.
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS // UNIX
		if (-1 == mkdir(dir.c_str(), 0777))
#else // WIN
		if (-1 == mkdir(dir.c_str()))
#endif
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/

		{
		  std::cerr << "check_create_directory() Fatal Error --";
		  std::cerr <<std::endl<< "\tcannot create:" <<dir<<std::endl;
		  exit(1);
		}

		std::cerr << "check_create_directory()";
		std::cerr <<std::endl<< "\tcreated:" <<dir<<std::endl;
	} 
	else if (!S_ISDIR(buf.st_mode))
	{
		// Some other type - error.
		std::cerr<<"check_create_directory() Fatal Error --";
		std::cerr<<std::endl<<"\t"<<dir<<" is nor Directory"<<std::endl;
		exit(1);
	}
	std::cerr << "check_create_directory()";
	std::cerr <<std::endl<< "\tDir Exists:" <<dir<<std::endl;
	return 1;
}


/******* ChatterBox *************************
 * Only One, and it's static.
 *
 ************************/

void chatterbox_message(Fl_Input *inp, void *v)
{
	// get the message from the line.
	std::string m(inp ->value());
	std::string s("LOCL USR");
	inp -> value("");

	//fserv -> chatmsg(m,s);
	fserv -> ownchatmsg(m);
	std::cerr << "Wish we could talk!" << std::endl;
	return;
}

void alert_okay_msg(Fl_Return_Button*, void*)
{
	ui -> alert_window -> hide();
	std::cerr << "Alert Okay" << std::endl;
	return;
}


void alert_cancel_msg(Fl_Button*, void*)
{
	ui -> alert_window -> hide();
	std::cerr << "Abort Cancel" << std::endl;
	return;
}

void file_result_select(Fl_File_Browser*, void*)
{
	std::cerr << "FL_FILE_BROWSER::file_select!!!!" << std::endl;
}


void file_download(Fl_Button*, void*)
{
	std::cerr << "FL_BUTTON::file_download!!!!" << std::endl;
	fserv->dirlist_download();
}

void file_recommend(Fl_Button*, void*)
{
	std::cerr << "FL_BUTTON::file_recommend!!!!" << std::endl;
	fserv->dirlist_recommend();
}


void file_requestdir(std::string person, std::string dir)
{
	std::cerr << "Request Dir Listing:" << person << ":" << dir << std::endl;
	fserv->load_dir(person, dir);
}



/* New Callback Fns for Channels *****************************/

/* add to channel broadcast (from dir_listing) */
void file_channel_broadcast(Fl_Button*, void*)
{
	std::cerr << "file_channel_broadcast()";
	std::cerr << std::endl;

	return;
}

/* add to channel broadcast (from search) */
void search_channel_broadcast(Fl_Button*, void*)
{
	std::cerr << "search_channel_broadcast()";
	std::cerr << std::endl;

	return;
}

/* callback for own channel select */
void channel_own_list_select(Fl_Browser*, void*)
{
	std::cerr << "channel_own_list_select()";
	std::cerr << std::endl;

	return;
}

/* callback for others channel select */
void channel_list_select(Fl_Funky_Browser*, void*)
{
	std::cerr << "channel_list_select()";
	std::cerr << std::endl;

	return;
}

/* callback for channel file listing select */
void channel_file_list_select(Fl_Funky_Browser*, void*)
{
	std::cerr << "channel_file_list_select()";
	std::cerr << std::endl;

	return;
}


/* button press to create channel */
void channel_create(Fl_Button*, void*)
{
	std::cerr << "channel_create()";
	std::cerr << std::endl;
	ui->channel_create_window->show();
	return;
}

/* button press to delete selected own channel */
void channel_delete(Fl_Button*, void*)
{
	std::cerr << "channel_delete()";
	std::cerr << std::endl;

	return;
}

/* open/close chat window */
void chat_open_callback(Fl_Button*, void*)
{
	std::cerr << "chat_open()";
	std::cerr << std::endl;
	if (ui->chatter_window->shown())
	{
		ui->chatter_window->hide();
	}
	else
	{
		ui->chatter_window->show();
	}
	return;
}

void channel_show_callback(Fl_Button*, void*)
{
	std::cerr << "channel_show_callback()";
	std::cerr << std::endl;
	ui->channel_details_window->show();

	return;
}

void channel_delete_callback(Fl_Button*, void*)
{
	std::cerr << "channel_delete_callback()";
	std::cerr << std::endl;

	return;
}


void chan_createmsg_list_select(Fl_Funky_Browser*, void*)
{
	std::cerr << "chan_createmsg_list_select()";
	std::cerr << std::endl;

	return;
}

void chan_createmsg_sendmsg_callback(Fl_Button*, void*)
{
	std::cerr << "chan_createmsg_sendmsg_callback()";
	std::cerr << std::endl;

	return;
}

void chan_createmsg_postpone_callback(Fl_Button*, void*)
{
	std::cerr << "chan_createmsg_postpone_callback()";
	std::cerr << std::endl;

	ui->channel_create_window->hide();
	return;
}

void chan_createmsg_cancel_callback(Fl_Button*, void*)
{
	std::cerr << "chan_createmsg_cancel_callback()";
	std::cerr << std::endl;

	ui->channel_create_window->hide();
	return;
}

void chan_createmsg_remove_callback(Fl_Button*, void*)
{
	std::cerr << "chan_createmsg_remove_callback()";
	std::cerr << std::endl;

	return;
}

void chan_createmsg_newname_callback(Fl_Input*, void*)
{
	std::cerr << "chan_createmsg_newname_callback()";
	std::cerr << std::endl;

	return;
}

void chan_createmsg_newname_button_callback(Fl_Round_Button*, void*)
{
	std::cerr << "chan_createmsg_newname_button_callback()";
	std::cerr << std::endl;

	return;
}

void chan_createmsg_title_button_callback(Fl_Round_Button*, void*)
{
	std::cerr << "chan_createmsg_button_callback()";
	std::cerr << std::endl;

	return;
}


void chan_msgdetails_list_select(Fl_Funky_Browser*, void*)
{
	std::cerr << "chan_msgdetails_list_select()";
	std::cerr << std::endl;

	return;
}

void chan_msgdetails_download_callback(Fl_Button*, void*)
{
	std::cerr << "chan_msgdetails_download_callback()";
	std::cerr << std::endl;

	return;
}

void chan_msgdetails_subscribe_callback(Fl_Button*, void*)
{
	std::cerr << "chan_msgdetails_subscribe_callback()";
	std::cerr << std::endl;

	return;
}

void chan_msgdetails_close_callback(Fl_Button*, void*)
{
	std::cerr << "chan_msgdetails_close_callback()";
	std::cerr << std::endl;
	ui->channel_details_window->hide();

	return;
}


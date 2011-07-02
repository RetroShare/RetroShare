

#include "peernet.h"
#include <stdio.h>
#include <QtGui/QApplication>
#include "mainwindow.h"
#include "dhtwindow.h"
#include "dhtquery.h"

#ifdef _WIN32
#include <getopt.h>
#endif

	/* for static PThreads under windows... we need to init the library...  
	 * Not sure if this is needed?
	 */
#ifdef PTW32_STATIC_LIB
	#include <pthread.h>
#endif

int main(int argc, char *argv[])
{

	bool showGUI = true;

	bool doConfig = false;
	
#ifdef __APPLE__
	std::string configPath = "../..";
#else
	std::string configPath = ".";
#endif

	bool doRestricted = false;
	std::list<std::string> restrictions;

	bool doProxyRestricted = false;
	std::list<std::string> proxyrestrictions;

	bool doFixedPort = false;
	int portNumber = 0;

	bool doLocalTesting = false;

	int c;
	while((c = getopt(argc, argv,"r:R:p:c:nl")) != -1)
	{
		switch (c)
		{
			case 'r':
				std::cerr << "Adding Port Restriction: " << optarg << std::endl;
				doRestricted = true;
				restrictions.push_back(optarg);
				break;
			case 'R':
				std::cerr << "Adding Proxy Restriction: " << optarg << std::endl;
				doProxyRestricted = true;
				proxyrestrictions.push_back(optarg);
				break;
			case 'p':
				std::cerr << "Setting Fixed Port: " << optarg << std::endl;
				doFixedPort = true;
				portNumber = atoi(optarg);
				break;
			case 'c':
				std::cerr << "Switching default Config Location: " << optarg << std::endl;
				doConfig = true;
				configPath = std::string(optarg);
				break;
			case 'n':
				std::cerr << "Disabling GUI" << std::endl;
				showGUI = false;
				break;
			case 'l':
				std::cerr << "Enabling Local Testing" << std::endl;
				doLocalTesting = true;
				break;
		}
	}


	/****************** WINDOWS  SPECIFIC INITIALISATION ****************/
#if defined(_WIN32) || defined(__MINGW32__)

	/* for static PThreads under windows... we need to init the library...  */
#ifdef PTW32_STATIC_LIB
	pthread_win32_process_attach_np();
#endif

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





	PeerNet *pnet = new PeerNet("", configPath, portNumber); 

	if (doRestricted)
	{
		std::list<std::pair<uint16_t, uint16_t> > portRestrictions;
		std::list<std::string>::iterator sit;

		for(sit = restrictions.begin(); sit != restrictions.end(); sit++)
		{
			/* parse the string */
			unsigned int lport, uport;
			if (2 == sscanf(sit->c_str(), "%u-%u", &lport, &uport))
			{
				std::cerr << "Adding Port Restriction (" << lport << "-" << uport << ")";
				std::cerr << std::endl;
				portRestrictions.push_back(std::make_pair<uint16_t, uint16_t>(lport, uport));
			}
		}

		if (portRestrictions.size() > 0)
		{
			pnet->setUdpStackRestrictions(portRestrictions);
		}
	}

	if (doProxyRestricted)
	{
		std::list<std::pair<uint16_t, uint16_t> > portRestrictions;
		std::list<std::string>::iterator sit;

		for(sit = proxyrestrictions.begin(); sit != proxyrestrictions.end(); sit++)
		{
			/* parse the string */
			unsigned int lport, uport;
			if (2 == sscanf(sit->c_str(), "%u-%u", &lport, &uport))
			{
				std::cerr << "Adding Port Restriction (" << lport << "-" << uport << ")";
				std::cerr << std::endl;
				portRestrictions.push_back(std::make_pair<uint16_t, uint16_t>(lport, uport));
			}
		}

		if (portRestrictions.size() > 0)
		{
			pnet->setProxyUdpStackRestrictions(portRestrictions);
		}
	}

	if (doLocalTesting)
	{
		pnet->setLocalTesting();
	}


	pnet->init();

	if (showGUI)
	{	
    		QApplication a(argc, argv);
    		MainWindow w;
    		w.show();
		DhtWindow  dw;
		dw.hide();
		DhtQuery  qw;
		qw.hide();
	
		w.setPeerNet(pnet);
		w.setDhtWindow(&dw);
		dw.setPeerNet(pnet);
		dw.setDhtQuery(&qw);
		qw.setPeerNet(pnet);
		qw.setQueryId("");
	
    		return a.exec();
	}
	else
	{
		while(1)
		{
			sleep(1);
			pnet->tick();
		}
	}
}

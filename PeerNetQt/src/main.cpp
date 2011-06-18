#include <stdio.h>
#include <QtGui/QApplication>
#include "mainwindow.h"
#include "peernet.h"

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

	bool doFixedPort = false;
	int portNumber = 0;

	bool doLocalTesting = false;

	int c;
	while((c = getopt(argc, argv,"r:p:c:nl")) != -1)
	{
		switch (c)
		{
			case 'r':
				std::cerr << "Adding Port Restriction: " << optarg << std::endl;
				doRestricted = true;
				restrictions.push_back(optarg);
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
	
		w.setPeerNet(pnet);
	
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

#include <fenv.h>

#include "Network.h"
#include "NetworkSimulatorGUI.h"
#include "MonitoredRsPeers.h"
#include <QApplication>
#include <argstream.h>

int main(int argc, char *argv[])
{
	feenableexcept(FE_INVALID) ;
	feenableexcept(FE_DIVBYZERO) ;

#ifndef DEBUG
	try
	{
#endif
		argstream as(argc,argv) ;
		bool show_gui = false;

		as >> option('i',"gui",show_gui,"show gui (vs. do the pipeline automatically)")
			>> help() ;

		as.defaultErrorHandling() ;

		// 2 - call the full pipeline

		Network network ;

		network.initRandom(20,0.2) ;

		rsPeers = new MonitoredRsPeers(network) ;

		if(show_gui)
		{
			QApplication app(argc,argv) ;

			NetworkSimulatorGUI pgui(network);
			pgui.show() ;

			return app.exec() ;
		}

		return 0 ;
#ifndef DEBUG
	}
	catch(std::exception& e)
	{
		std::cerr << "Unhandled exception: " << e.what() << std::endl;
		return 1 ;
	}
#endif
}


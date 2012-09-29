#include "ui_NetworkSimulatorGUI.h"

class TurtleRouterStatistics ;
class NetworkViewer ;
class Network ;

class NetworkSimulatorGUI: public QMainWindow, public Ui::NetworkSimulatorGUI
{
	Q_OBJECT

	public:
		NetworkSimulatorGUI(Network& net) ;

	public slots:
		void updateSelectedNode(int) ;
		void toggleNetworkTraffic(bool) ;

		virtual void timerEvent(QTimerEvent *e) ;

	private:
		NetworkViewer *_viewer ;
		TurtleRouterStatistics *_turtle_router_statistics ;

		int tickTimerId ;
};


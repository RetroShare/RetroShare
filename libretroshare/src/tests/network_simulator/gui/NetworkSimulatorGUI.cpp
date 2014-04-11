#include <QFrame>
#include <QObject>

#include "NetworkSimulatorGUI.h"
#include "NetworkViewer.h"
#include "TurtleRouterStatistics.h"
#include "GlobalRouterStatistics.h"

NetworkSimulatorGUI::NetworkSimulatorGUI(Network& net)
{
	setupUi(this) ;
	tickTimerId = 0 ;

	QVBoxLayout *layout = new QVBoxLayout(networkViewFrame) ;
	layout->addWidget(_viewer = new NetworkViewer(networkViewFrame,net)) ;

	QObject::connect(_viewer,SIGNAL(nodeSelected(int)),this,SLOT(updateSelectedNode(int))) ;
	QObject::connect(flow_CB,SIGNAL(toggled(bool)),this,SLOT(toggleNetworkTraffic(bool))) ;

    QVBoxLayout *layout2 = new QVBoxLayout(inspectorFrame) ;
    QTabWidget *tabwidget = new QTabWidget() ;

    layout2->addWidget(tabwidget) ;

    tabwidget->addTab(_turtle_router_statistics = new TurtleRouterStatistics(),"Turtle router" ) ;
    tabwidget->addTab(_global_router_statistics = new GlobalRouterStatistics(),"Global router" ) ;
}

void NetworkSimulatorGUI::updateSelectedNode(int node_id)
{
	const RsTurtle *turtle = _viewer->network().node(node_id).turtle_service() ;
	const RsGRouter *grouter = _viewer->network().node(node_id).global_router_service() ;

    _turtle_router_statistics->setTurtleRouter( turtle ) ;
    _global_router_statistics->setGlobalRouter( grouter ) ;

	 std::cerr << "Selected objects: turtle=" << std::hex <<  turtle << ", grouter=" << grouter << std::dec << std::endl;
}

void NetworkSimulatorGUI::toggleNetworkTraffic(bool b)
{
	if(!b && tickTimerId > 0)
	{
		killTimer(tickTimerId) ;
		tickTimerId = 0 ;
		return ;
	}

	if(b && tickTimerId == 0)
	{
		tickTimerId = startTimer(1000) ;
		return ;
	}

	std::cerr << "ERROR !!" << std::endl;
}

void NetworkSimulatorGUI::timerEvent(QTimerEvent *event)
{
	Q_UNUSED(event) ;

	//std::cerr << "timer event!" << std::endl;

	_viewer->network().tick() ;

}


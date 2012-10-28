#include <QPoint>
#include <QGLViewer/qglviewer.h>

// The network simulator GUI has the following functionalities:
//
// 1 - show the network graph
//			* the graph should spread in space automatically. We should use the code from the NetworkView for that.
//			* each edge will be drawn with a color that displays used bandwidth along the edge, or TR sent to the edge, etc.
//			* individual tunnels should be shown, in order to see what shape they have
//
// 2 - show info about each node. One needs a node widget that gets updated with whatever is to be read from the current node
//
// 3 - buttons to re-initiate a new network, or reset the network.
//
// 4 - buttons to inject information into the network:
// 		* shared files in some nodes. Should be handled by derivign the component that the turtle router accesses for local searches.
// 		* file requests in other nodes. Eazy: one just needs to ask the turtle router of the node to handle the hash.
//
// 5 - perturbate the network 
// 		* change the load of each node, and the delay when forwarding requests.
//
#include "Network.h"

class NetworkViewer: public QGLViewer
{
	Q_OBJECT

	public:
		NetworkViewer(QWidget *parent,Network& network) ;

		virtual void draw() ;
		virtual void keyPressEvent(QKeyEvent *) {}
		virtual void mousePressEvent(QMouseEvent *) ;
		virtual void mouseReleaseEvent(QMouseEvent *) ;
		virtual void mouseMoveEvent(QMouseEvent *) ;

		const Network& network() const { return _network ; }
		Network& network() { return _network ; }

   signals:
		void nodeSelected(int) ;

	public slots:
		void timerEvent(QTimerEvent *) ;
		void contextMenu(QPoint) ;
		void actionManageHash() ;
		void actionProvideHash() ;

	private:
		void calculateForces(const Network::NodeId& node_id,const double *map,int W,int H,float x,float y,float /*speedf*/,float& new_x, float& new_y) ;

		typedef struct 
		{
			float x ;
			float y ;
		} NodeCoord ;

		Network& _network ;

		std::vector<NodeCoord> _node_coords ;
		std::vector<NodeCoord> _node_speeds ;

		static const float MASS_FACTOR      = 10 ;
		static const float FRICTION_FACTOR  = 15.8 ;
		static const float REPULSION_FACTOR = 8  ;
		static const float NODE_DISTANCE    = 30.0 ;

		int timerId ;

		int _current_selected_node ;
		int _current_displayed_node ;
		int _current_acted_node ;
		bool _dragging ;
		bool _nodes_need_recomputing ;

		QAction *action_ManageHash ;
		QAction *action_ProvideHash ;
};


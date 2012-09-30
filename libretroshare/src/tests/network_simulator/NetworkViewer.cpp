#include <QMouseEvent>
#include <QMenu>
#include <QAction>

#include <util/rsid.h>

#include "Network.h"
#include "NetworkViewer.h"

NetworkViewer::NetworkViewer(QWidget *parent,Network&net)
	:  QGLViewer(parent),_network(net) , timerId(0)
{
	_current_selected_node = -1 ;
	_dragging = false ;

	_node_coords.resize(net.n_nodes()) ;
	_node_speeds.resize(net.n_nodes()) ;

	for(int i=0;i<_node_coords.size();++i)
	{
		_node_coords[i].x = drand48()*width() ;
		_node_coords[i].y = drand48()*height() ;
		_node_speeds[i].x = 0 ;
		_node_speeds[i].y = 0 ;
	}

	timerId = startTimer(1000/25) ;

	connect(this,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(contextMenu(QPoint)));

	action_ManageHash = new QAction(QString("Manage hash"),this) ;
	QObject::connect(action_ManageHash,SIGNAL(triggered()),this,SLOT(actionManageHash())) ;
}

void NetworkViewer::draw() 
{
	glDisable(GL_DEPTH_TEST) ;
	glClear(GL_COLOR_BUFFER_BIT) ;

	// for now, view is fixed.

	glMatrixMode(GL_MODELVIEW) ;
	glPushMatrix() ;
	glLoadIdentity() ;
	glMatrixMode(GL_PROJECTION) ;
	glPushMatrix() ;
	glLoadIdentity() ;
	glOrtho(0,width(),0,height(),1,-1) ;

	glEnable(GL_BLEND) ;
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA) ;

	// Now, draw all edges

	glEnable(GL_LINE_SMOOTH) ;
	glColor3f(0.4f,0.4f,0.4f) ;
	glBegin(GL_LINES) ;

	for(uint32_t i=0;i<_network.n_nodes();++i)
	{
		const std::set<uint32_t>& neighs( _network.neighbors(i) ) ;

		for(std::set<uint32_t>::const_iterator it(neighs.begin());it!=neighs.end();++it)
			if( i < *it )
			{
				glVertex2f(_node_coords[  i].x, _node_coords[  i].y) ;
				glVertex2f(_node_coords[*it].x, _node_coords[*it].y) ;
			}
	}

	glEnd() ;

	// Draw all nodes.
	//
	glEnable(GL_POINT_SMOOTH) ;
	glPointSize(10.0f) ;
	glBegin(GL_POINTS) ;

	for(uint32_t i=0;i<_network.n_nodes();++i)
	{
		if((int)i == _current_selected_node)
			glColor3f(1.0f,0.2f,0.1f) ;
		else
			glColor3f(0.8f,0.8f,0.8f) ;

		glVertex2f(_node_coords[i].x, _node_coords[i].y) ;
	}

	glEnd() ;


	glMatrixMode(GL_MODELVIEW) ;
	glPopMatrix() ;
	glMatrixMode(GL_PROJECTION) ;
	glPopMatrix() ;
}

#define SWAP(a,b) tempr=(a);(a)=(b);(b)=tempr

void fourn(double data[],unsigned long nn[],unsigned long ndim,int isign)
{
	int i1,i2,i3,i2rev,i3rev,ip1,ip2,ip3,ifp1,ifp2;
	int ibit,idim,k1,k2,n,nprev,nrem,ntot;
	double tempi,tempr;
	double theta,wi,wpi,wpr,wr,wtemp;

	ntot=1;
	for (idim=1;idim<=(long)ndim;idim++)
		ntot *= nn[idim];
	nprev=1;
	for (idim=ndim;idim>=1;idim--) {
		n=nn[idim];
		nrem=ntot/(n*nprev);
		ip1=nprev << 1;
		ip2=ip1*n;
		ip3=ip2*nrem;
		i2rev=1;
		for (i2=1;i2<=ip2;i2+=ip1) {
			if (i2 < i2rev) {
				for (i1=i2;i1<=i2+ip1-2;i1+=2) {
					for (i3=i1;i3<=ip3;i3+=ip2) {
						i3rev=i2rev+i3-i2;
						SWAP(data[i3],data[i3rev]);
						SWAP(data[i3+1],data[i3rev+1]);
					}
				}
			}
			ibit=ip2 >> 1;
			while (ibit >= ip1 && i2rev > ibit) {
				i2rev -= ibit;
				ibit >>= 1;
			}
			i2rev += ibit;
		}
		ifp1=ip1;
		while (ifp1 < ip2) {
			ifp2=ifp1 << 1;
			theta=isign*6.28318530717959/(ifp2/ip1);
			wtemp=sin(0.5*theta);
			wpr = -2.0*wtemp*wtemp;
			wpi=sin(theta);
			wr=1.0;
			wi=0.0;
			for (i3=1;i3<=ifp1;i3+=ip1) {
				for (i1=i3;i1<=i3+ip1-2;i1+=2) {
					for (i2=i1;i2<=ip3;i2+=ifp2) {
						k1=i2;
						k2=k1+ifp1;
						tempr=wr*data[k2]-wi*data[k2+1];
						tempi=wr*data[k2+1]+wi*data[k2];
						data[k2]=data[k1]-tempr;
						data[k2+1]=data[k1+1]-tempi;
						data[k1] += tempr;
						data[k1+1] += tempi;
					}
				}
				wr=(wtemp=wr)*wpr-wi*wpi+wr;
				wi=wi*wpr+wtemp*wpi+wi;
			}
			ifp1=ifp2;
		}
		nprev *= n;
	}
}

#undef SWAP

static void convolveWithGaussian(double *forceMap,int S,int /*s*/)
{
	static double *bf = NULL ;

	if(bf == NULL)
	{
		bf = new double[S*S*2] ;

		for(int i=0;i<S;++i)
			for(int j=0;j<S;++j)
			{
				int x = (i<S/2)?i:(S-i) ;
				int y = (j<S/2)?j:(S-j) ;
//				int l=2*(x*x+y*y);
				bf[2*(i+S*j)] = log(sqrtf(0.1 + x*x+y*y)); // linear -> derivative is constant
				bf[2*(i+S*j)+1] = 0 ;
			}

		unsigned long nn[2] = {S,S};
		fourn(&bf[-1],&nn[-1],2,1) ;
	}

	unsigned long nn[2] = {S,S};
	fourn(&forceMap[-1],&nn[-1],2,1) ;

	for(int i=0;i<S;++i)
		for(int j=0;j<S;++j)
		{
			float a = forceMap[2*(i+S*j)]*bf[2*(i+S*j)] - forceMap[2*(i+S*j)+1]*bf[2*(i+S*j)+1] ;
			float b = forceMap[2*(i+S*j)]*bf[2*(i+S*j)+1] + forceMap[2*(i+S*j)+1]*bf[2*(i+S*j)] ;

			forceMap[2*(i+S*j)]   = a ;
			forceMap[2*(i+S*j)+1] = b ;
		}

	fourn(&forceMap[-1],&nn[-1],2,-1) ;

	for(int i=0;i<S*S*2;++i)
		forceMap[i] /= S*S;
}

void NetworkViewer::timerEvent(QTimerEvent *event)
{
    Q_UNUSED(event);

	 if(!isVisible())
		 return ;

	 static const int S = 256 ;
	 static double *forceMap = new double[2*S*S] ;

	 memset(forceMap,0,2*S*S*sizeof(double)) ;

	 for(uint32_t i=0;i<_network.n_nodes();++i)
	 {
		 float x = S*_node_coords[i].x/width() ;
		 float y = S*_node_coords[i].y/height() ;

		 int i=(int)floor(x) ;
		 int j=(int)floor(y) ;

		 float di = x-i ;
		 float dj = y-j ;

		 if( i>=0 && i<S-1 && j>=0 && j<S-1)
		 {
			 forceMap[2*(i  +S*(j  ))] += (1-di)*(1-dj) ;
			 forceMap[2*(i+1+S*(j  ))] +=    di *(1-dj) ;
			 forceMap[2*(i  +S*(j+1))] += (1-di)*dj ;
			 forceMap[2*(i+1+S*(j+1))] +=    di *dj ;
		 }
	 }

	 // compute convolution with 1/omega kernel.
	 //
	 convolveWithGaussian(forceMap,S,20) ;
	 
	 static float speedf=1.0f;

	 std::vector<NodeCoord> new_coords(_node_coords) ;

	 for(uint32_t i=0;i<_network.n_nodes();++i)
		 if(i != _current_selected_node || !_dragging)
		 {
			 float x = _node_coords[i].x ;
			 float y = _node_coords[i].y ;

			 calculateForces(i,forceMap,S,S,x,y,speedf,new_coords[i].x,new_coords[i].y);
		 }

    bool itemsMoved = false;
	 for(uint32_t i=0;i<_node_coords.size();++i)
	 {
		 if( fabsf(_node_coords[i].x - new_coords[i].x) > 0.2 || fabsf(_node_coords[i].y - new_coords[i].y) > 0.2)
            itemsMoved = true;

		 //std::cerr << "Old i = " << _node_coords[i].x << ", new = " << new_coords[i].x << std::endl;
		 _node_coords[i] = new_coords[i] ;
	 }

    if (!itemsMoved) {
        killTimer(timerId);
//#ifdef DEBUG_ELASTIC
		  std::cerr << "Killing timr" << std::endl ;
//#endif
        timerId = 0;
    }
	 else
	 {
		 updateGL() ;
		 usleep(2000) ;
	 }

}

void NetworkViewer::mouseMoveEvent(QMouseEvent *e)
{
	if(_dragging && _current_selected_node >= 0)
	{
		_node_coords[_current_selected_node].x = e->x() ;
		_node_coords[_current_selected_node].y = height() - e->y() ;

		if(timerId == 0)
			timerId = startTimer(1000/25) ;

		updateGL() ;
	}
}

void NetworkViewer::mouseReleaseEvent(QMouseEvent *e)
{
	_dragging = false ;
}

void NetworkViewer::mousePressEvent(QMouseEvent *e)
{
	float x = e->x() ;
	float y = height() - e->y() ;

	// find which node is selected

	for(uint32_t i=0;i<_node_coords.size();++i)
		if( pow(_node_coords[i].x - x,2)+pow(_node_coords[i].y - y,2) < 10*10 )
		{
			_current_selected_node = i ;
			_dragging = true ;
			updateGL() ;

			emit nodeSelected(i) ;

			if(e->button() == Qt::RightButton)
				emit customContextMenuRequested(QPoint(e->x(),e->y())) ;

			return ;
		}

	_dragging = false ;
	_current_selected_node = -1 ;
}

void NetworkViewer::calculateForces(const Network::NodeId& node_id,const double *map,int W,int H,float x,float y,float /*speedf*/,float& new_x, float& new_y)
{
#ifdef A_FAIRE
	if (mouseGrabberItem() == this) 
	{
		new_x = x ;
		new_y = y ;
		return;
	}
#endif

	// Sum up all forces pushing this item away
	qreal xforce = 0;
	qreal yforce = 0;

	float dei=0.0f ;
	float dej=0.0f ;

	static float *e = NULL ;
	static const int KS = 5 ;

	if(e == NULL)
	{
		e = new float[(2*KS+1)*(2*KS+1)] ;

		for(int i=-KS;i<=KS;++i)
			for(int j=-KS;j<=KS;++j)
				e[i+KS+(2*KS+1)*(j+KS)] = exp( -(i*i+j*j)/30.0 ) ;	// can be precomputed
	}

	for(int i=-KS;i<=KS;++i)
		for(int j=-KS;j<=KS;++j)
		{
			int X = std::min(W-1,std::max(0,(int)rint(x/(float)width()*W))) ;
			int Y = std::min(H-1,std::max(0,(int)rint(y/(float)height()*H))) ;

			float val = map[2*((i+X+W)%W + W*((j+Y+H)%H))] ;

			dei += i * e[i+KS+(2*KS+1)*(j+KS)] * val ;
			dej += j * e[i+KS+(2*KS+1)*(j+KS)] * val ;
		}

	xforce = REPULSION_FACTOR * dei/25.0;
	yforce = REPULSION_FACTOR * dej/25.0;

	// Now subtract all forces pulling items together
	//
	const std::set<Network::NodeId>& neighbs(_network.neighbors(node_id)) ;
	double weight = neighbs.size() + 1 ;

	for(std::set<Network::NodeId>::const_iterator it(neighbs.begin());it!=neighbs.end();++it) 
	{
		NodeCoord pos;
		double w2 ;	// This factor makes the edge length depend on connectivity, so clusters of friends tend to stay in the
						// same location.
						//
						
		pos.x = _node_coords[*it].x - x  ; //mapFromItem(edge->destNode(), 0, 0);
		pos.y = _node_coords[*it].y - y  ; //mapFromItem(edge->destNode(), 0, 0);

		w2 = sqrtf(std::min(neighbs.size(),_network.neighbors(*it).size())) ;

		float dist = sqrtf(pos.x*pos.x + pos.y*pos.y) ;
		float val = dist - NODE_DISTANCE * w2 ;

		xforce += 0.01*pos.x * val / weight;
		yforce += 0.01*pos.y * val / weight;
	}

	xforce -= FRICTION_FACTOR * _node_speeds[node_id].x ;
	yforce -= FRICTION_FACTOR * _node_speeds[node_id].y ;

	// This term drags nodes away from the sides.
	//
	if(x < 15) xforce += 100.0/(x+0.1) ;
	if(y < 15) yforce += 100.0/(y+0.1) ;
	if(x > width()-15) xforce -= 100.0/(width()-x+0.1) ;
	if(y > height()-15) yforce -= 100.0/(height()-y+0.1) ;

	// now time filter:

	_node_speeds[node_id].x += xforce / MASS_FACTOR;
	_node_speeds[node_id].y += yforce / MASS_FACTOR;

	if(_node_speeds[node_id].x > 10) _node_speeds[node_id].x = 10.0f ;
	if(_node_speeds[node_id].y > 10) _node_speeds[node_id].y = 10.0f ;
	if(_node_speeds[node_id].x <-10) _node_speeds[node_id].x =-10.0f ;
	if(_node_speeds[node_id].y <-10) _node_speeds[node_id].y =-10.0f ;

	new_x = x + _node_speeds[node_id].x ;
	new_y = y + _node_speeds[node_id].y ;

	new_x = std::min(std::max(new_x, 10.0f),  width() - 10.0f);
	new_y = std::min(std::max(new_y, 10.0f), height() - 10.0f);
}

void NetworkViewer::contextMenu(QPoint p)
{
	std::cerr << "Context menu request at point " << p.x() << " " << p.y() << std::endl;

	QMenu contextMnu ;//= ui.msgText->createStandardContextMenu(matrix.map(point));

	contextMnu.addAction(action_ManageHash);
	contextMnu.exec(mapToGlobal(p));
}

void NetworkViewer::actionManageHash()
{
	if(_current_selected_node < 0)
		return ;

	std::cerr << "Managing hash..." << std::endl;

	unsigned char hash_bytes[20] ;
	for(int i=0;i<20;++i)
		hash_bytes[i] = lrand48() & 0xff ;

	std::string hash = t_RsGenericIdType<20>(hash_bytes).toStdString(false) ;;

	std::cerr << "   current node = " << _current_selected_node << std::endl ;
	std::cerr << "   adding random hash = " << hash << std::endl;

	_network.node(_current_selected_node).manageFileHash(hash) ;

	updateGL() ;
}



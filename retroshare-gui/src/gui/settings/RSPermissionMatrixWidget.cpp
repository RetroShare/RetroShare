/*******************************************************************************
 * gui/settings/RSPermissionMatrixWidget.cpp                                   *
 *                                                                             *
 * Copyright (c) 2014 Retroshare Team <retroshare.project@gmail.com>           *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#ifndef WINDOWS_SYS
#include <sys/times.h>
#endif

#include <iostream>
#include <math.h>
#include <QtGlobal>
#include <QPainter>
#include <QDateTime>
#include <QTimer>
#include <QMouseEvent>

#include "RSPermissionMatrixWidget.h"
#include "gui/settings/ServicePermissionsPage.h"
#include <retroshare/rsstatus.h>
#include <retroshare/rspeers.h>
#include <retroshare/rsservicecontrol.h>

#define NOT_IMPLEMENTED std::cerr << __PRETTY_FUNCTION__ << ": not yet implemented." << std::endl;

// The behavior of the widget is the following:
// - when default is changed, switch all icons to the default position. Then every icon switch back by the user will be
//   in the white list.
// - each peer/service slot has a switch. If the peer doesn't allow the service, the  switch is shown with some
//   indication. When both the current and adverse permissions are granted, the switch should show it as well.
// - we should use tooltips

const float RSPermissionMatrixWidget::fICON_SIZE_X    = 40/16.0 ;
const float RSPermissionMatrixWidget::fICON_SIZE_Y    = 40/16.0 ;
const float RSPermissionMatrixWidget::fROW_SIZE       = 42/16.0 ;
const float RSPermissionMatrixWidget::fCOL_SIZE       = 42/16.0 ;
const float RSPermissionMatrixWidget::fMATRIX_START_X =  5/16.0 ;
const float RSPermissionMatrixWidget::fMATRIX_START_Y =100/16.0 ;

/** Default contructor */
RSPermissionMatrixWidget::RSPermissionMatrixWidget(QWidget *parent)
  :QFrame(parent)
{
  _painter = new QPainter();

  setMouseTracking(true) ;

  _timer = new QTimer ;
  QObject::connect(_timer,SIGNAL(timeout()),this,SLOT(updateDisplay())) ;
  _timer->start(5000);

  _max_width = 400 ;
  _max_height = 0 ;

  mHideOffline = false;
}

void RSPermissionMatrixWidget::setHideOffline(bool hide)
{
    if (mHideOffline == hide) {
        return;
    }

    mHideOffline = hide;
    repaint();
}

void RSPermissionMatrixWidget::updateDisplay()
{
    if(isHidden())
        return ;

    setMinimumWidth(_max_width) ;
    setMinimumHeight(_max_height) ;

    update() ;
}

void RSPermissionMatrixWidget::mousePressEvent(QMouseEvent *e)
{
    std::cerr << "mouse pressed at x=" << e->x() << ", y=" << e->y() << std::endl;

    uint32_t service_id ;
    RsPeerId peer_id ;

    if(computeServiceAndPeer(e->x(),e->y(),service_id,peer_id))
    {
        std::cerr << "Peer id: " << peer_id << ", service: " << service_id << std::endl;

        // Make sure the service is not globally disabled

        RsServicePermissions serv_perms;
        rsServiceControl->getServicePermissions(service_id,serv_perms);

        if (!serv_perms.mDefaultAllowed) return;

        switchPermission(service_id,peer_id) ;
        update() ;
    }
    else if(computeServiceGlobalSwitch(e->x(),e->y(),service_id))
    {
        switchPermission(service_id) ;
        update();
    }
    else
        QFrame::mousePressEvent(e) ;
}

void RSPermissionMatrixWidget::switchPermission(uint32_t service,const RsPeerId& pid)
{
    RsServicePermissions serv_perms ;

    if(!rsServiceControl->getServicePermissions(service,serv_perms))
        return ;

    if(serv_perms.peerHasPermission(pid))
        serv_perms.resetPermission(pid) ;
    else
        serv_perms.setPermission(pid) ;

    rsServiceControl->updateServicePermissions(service,serv_perms);
}

void RSPermissionMatrixWidget::switchPermission(uint32_t service)
{
    RsServicePermissions serv_perms ;

    if(!rsServiceControl->getServicePermissions(service,serv_perms))
        return ;

    if(serv_perms.mDefaultAllowed)
    {
        serv_perms.mPeersAllowed.clear() ;
    serv_perms.mDefaultAllowed = false ;
    }
    else
    {
        serv_perms.mDefaultAllowed = true ;
        serv_perms.mPeersDenied.clear() ;
    }

    rsServiceControl->updateServicePermissions(service,serv_perms);
}

void RSPermissionMatrixWidget::mouseMoveEvent(QMouseEvent *e)
{
    uint32_t service_id ;
    RsPeerId peer_id ;

    if(computeServiceAndPeer(e->x(),e->y(),service_id,peer_id))
    {
        if(_current_service_id != service_id || _current_peer_id != peer_id)
        {
            _current_service_id = service_id ;
            _current_peer_id = peer_id ;

            // redraw!
            update() ;
        }
    }
    else if(computeServiceGlobalSwitch(e->x(),e->y(),service_id))
    {
        peer_id.clear() ;

    if(_current_service_id != service_id || _current_peer_id != peer_id)
    {
        _current_service_id = service_id ;
        _current_peer_id = peer_id ;

        // redraw!
        update() ;
    }
    }
    else
    {
        service_id = ~0 ;
        peer_id.clear() ;
    }
}

/** Default destructor */
RSPermissionMatrixWidget::~RSPermissionMatrixWidget()
{
    _timer->stop() ;

    delete _timer ;
    delete _painter;
}

bool sortRsPeerIdByNameLocation(const RsPeerId &a, const RsPeerId &b)
{
	RsPeerDetails detailsA, detailsB;
	rsPeers->getPeerDetails(a, detailsA);
	rsPeers->getPeerDetails(b, detailsB);
	QString stringA, stringB;

	// name
	stringA += QString::fromUtf8(detailsA.name.c_str());
	stringB += QString::fromUtf8(detailsB.name.c_str());

	// location
	stringA += QString::fromUtf8(detailsA.location.c_str());
	stringB += QString::fromUtf8(detailsB.location.c_str());

	return stringA.toLower() < stringB.toLower();
}

/** Overloads default QWidget::paintEvent. Draws the actual
 * bandwidth graph. */
void RSPermissionMatrixWidget::paintEvent(QPaintEvent *)
{
    //std::cerr << "In paint event!" << std::endl;

    int S = QFontMetricsF(font()).height();

  /* Set current graph dimensions */
  _rec = this->frameRect();

  /* Start the painter */
  _painter->begin(this);

  /* We want antialiased lines and text */
  _painter->setRenderHint(QPainter::Antialiasing);
  _painter->setRenderHint(QPainter::TextAntialiasing);

  /* Fill in the background */
  _painter->fillRect(_rec, QBrush(BACK_COLOR));
  _painter->drawRect(_rec);

  // draw one line per friend.
  std::list<RsPeerId> ssllist ;
  rsPeers->getFriendList(ssllist) ;

  // sort list
  {
      // sort out offline peers
      if(mHideOffline) {
          RsPeerDetails peerDetails;
          for(std::list<RsPeerId>::iterator it = ssllist.begin(); it != ssllist.end();) {
              rsPeers->getPeerDetails(*it, peerDetails);

              switch (peerDetails.connectState) {
              case RS_PEER_CONNECTSTATE_OFFLINE:
              case RS_PEER_CONNECTSTATE_TRYING_TCP:
              case RS_PEER_CONNECTSTATE_TRYING_UDP:
                  it = ssllist.erase(it);
                  break;
              default:
                  it++;
                  break;
              }
          }
      }

      // sort by name
      ssllist.sort(sortRsPeerIdByNameLocation);
  }

  RsPeerServiceInfo ownServices;
  rsServiceControl->getOwnServices(ownServices);

  // Display friend names at the beginning of each column

  const QFont& font(_painter->font()) ;
  QFontMetrics fm(font);
  int peer_name_size = 0 ;
  float line_height = 2 + fm.height() ;

  std::vector<QString> names ;

  for(std::list<RsPeerId>::const_iterator it(ssllist.begin());it!=ssllist.end();++it)
  {
      RsPeerDetails details ;
      rsPeers->getPeerDetails(*it,details) ;

      QString name = QString::fromUtf8(details.name.c_str()) + " (" + QString::fromUtf8(details.location.c_str()) + ")";
      // TODO does not work correctly with hieroglyphs
      if(name.length() > 20 + 3)
          name = name.left(20)+"..." ;

      peer_name_size = std::max(peer_name_size, fm.width(name)) ;
      names.push_back(name) ;
  }

  QPen pen ;
  pen.setWidth(2) ;
  pen.setBrush(Qt::black) ;

  _painter->setPen(pen) ;
  int i=0;
  //int x=5/14.0*S ;
  int y=S*fMATRIX_START_Y ;

  for(std::list<RsPeerId>::const_iterator it(ssllist.begin());it!=ssllist.end();++it,++i)
  {
      float X = S*fMATRIX_START_X + peer_name_size - fm.width(names[i]) ;
      float Y = S*fMATRIX_START_Y + (i+0.5)*S*fROW_SIZE + line_height/2.0f-2 ;

      _painter->drawText(QPointF(X,Y),names[i]) ;

      if(*it == _current_peer_id)
          _painter->drawLine(QPointF(X,Y+3),QPointF(X+fm.width(names[i]),Y+3)) ;

      y += line_height ;
  }

  matrix_start_x = S*5/14.0 + S*fMATRIX_START_X + peer_name_size ;

  // now draw the service names

  i=0 ;
  std::vector<int> last_width(10,0) ;

  for(std::map<uint32_t, RsServiceInfo>::const_iterator it(ownServices.mServiceList.begin());it!=ownServices.mServiceList.end();++it,++i)
  {
      QString name = QString::fromUtf8(it->second.mServiceName.c_str()) ;
      int text_width = fm.width(name) ;

      int X = matrix_start_x + S*fCOL_SIZE/2 - 2 + i*S*fCOL_SIZE - text_width/2;

      int height_index = 0 ;
      while(last_width[height_index] > X-5 && height_index < ((int)last_width.size()-1) )
          ++height_index ;

      int Y = S*fMATRIX_START_Y - S*fICON_SIZE_Y - 2 - line_height * height_index;

      last_width[height_index] = X + text_width ;
       // draw a half-transparent rectangle

      QBrush brush ;
      brush.setColor(QColor::fromHsvF(0.0f,0.0f,1.0f,0.8f));
      brush.setStyle(Qt::SolidPattern) ;

      QPen pen ;
      pen.setWidth(2) ;

      if(_current_service_id == it->second.mServiceType)
          pen.setBrush(Qt::black) ;
      else
          pen.setBrush(Qt::gray) ;

      _painter->setPen(pen) ;

      QRect info_pos( X-5,Y-line_height-2, text_width + 10, line_height + 5) ;

      //_painter->fillRect(info_pos,brush) ;
      //_painter->drawRect(info_pos) ;

      _painter->drawLine(QPointF(X,Y+3),QPointF(X+text_width,Y+3)) ;
      _painter->drawLine(QPointF(X+text_width/2, Y+3), QPointF(X+text_width/2,S*fMATRIX_START_Y+peer_ids.size()*S*fROW_SIZE - S*fROW_SIZE+5)) ;

      pen.setBrush(Qt::black) ;
      _painter->setPen(pen) ;

      _painter->drawText(QPointF(X,Y),name);
  }

  // Now draw the global switches.

  peer_ids.clear() ;
  for(std::list<RsPeerId>::const_iterator it(ssllist.begin());it!=ssllist.end();++it)
      peer_ids.push_back(*it) ;
  service_ids.clear() ;
  for(std::map<uint32_t, RsServiceInfo>::const_iterator sit(ownServices.mServiceList.begin());sit!=ownServices.mServiceList.end();++sit)
      service_ids.push_back(sit->first) ;

  static const std::string global_switch[2] = { ":/icons/global_switch_off_128.png",
                                                ":/icons/global_switch_on_128.png" } ;

  for(uint32_t i=0;i<service_ids.size();++i)
  {
      RsServicePermissions serv_perm ;
      rsServiceControl->getServicePermissions(service_ids[i],serv_perm) ;

      QPixmap pix(global_switch[serv_perm.mDefaultAllowed].c_str()) ;
      QRect position = computeNodePosition(0,i,false) ;

      position.setY(position.y() - S*fICON_SIZE_Y + 8/14.0*S) ;
      position.setX(position.x() + 3/14.0*S) ;
      position.setHeight(30/14.0*S) ;
      position.setWidth(30/14.0*S) ;

      _painter->drawPixmap(position,pix.scaledToHeight(S*fICON_SIZE_Y*0.9,Qt::SmoothTransformation),QRect(0,0,S*fICON_SIZE_X,S*fICON_SIZE_Y)) ;
  }

  // We draw for each service.

  static const std::string pixmap_names[4] = { ":/icons/switch00_128.png",
                                               ":/icons/switch01_128.png",
                                               ":/icons/switch10_128.png",
                                               ":/icons/switch11_128.png" } ;

  int n_col = 0 ;
  int n_col_selected = -1 ;
  int n_row_selected = -1 ;

  for(std::map<uint32_t, RsServiceInfo>::const_iterator sit(ownServices.mServiceList.begin());sit!=ownServices.mServiceList.end();++sit,++n_col)
  {
      RsServicePermissions service_perms ;

      rsServiceControl->getServicePermissions(sit->first,service_perms) ;

      // draw the default switch.


      // draw one switch per friend.

      int n_row = 0 ;

      for(std::list<RsPeerId>::const_iterator it(ssllist.begin());it!=ssllist.end();++it,++n_row)
      {
          RsPeerServiceInfo local_service_perms ;
          RsPeerServiceInfo remote_service_perms ;

          rsServiceControl->getServicesAllowed (*it, local_service_perms) ;
          rsServiceControl->getServicesProvided(*it,remote_service_perms) ;

          bool  local_allowed =  local_service_perms.mServiceList.find(sit->first) !=  local_service_perms.mServiceList.end() ;
          bool remote_allowed = remote_service_perms.mServiceList.find(sit->first) != remote_service_perms.mServiceList.end() ;

          QPixmap pix(pixmap_names[(local_allowed << 1) + remote_allowed].c_str()) ;

          bool selected = (sit->first == _current_service_id && *it == _current_peer_id) ;
          QRect position = computeNodePosition(n_row,n_col,selected) ;

          if(selected)
          {
              n_row_selected = n_row ;
              n_col_selected = n_col ;
          }
          _painter->drawPixmap(position,pix.scaledToHeight(S*fICON_SIZE_X,Qt::SmoothTransformation),QRect(0,0,S*fICON_SIZE_X,S*fICON_SIZE_Y)) ;
      }
  }

  // now display some info about current node.

  if(n_row_selected >= 0 && n_col_selected >= 0 && n_row_selected < (int)peer_ids.size() && n_col_selected < (int)service_ids.size())
  {
      QRect position = computeNodePosition(n_row_selected,n_col_selected,false) ;

      // draw text info

      RsServicePermissions service_perms ;

      rsServiceControl->getServicePermissions(service_ids[n_col_selected],service_perms) ;

      QString service_name    = tr("Service name:")+" "+QString::fromUtf8(service_perms.mServiceName.c_str()) ;
      QString service_default = service_perms.mDefaultAllowed?tr("Allowed by default"):tr("Denied by default");
      QString peer_name = tr("Peer name:")+" " + names[n_row_selected] ;
      QString peer_id = tr("Peer Id:")+" "+QString::fromStdString(_current_peer_id.toStdString()) ;

      RsPeerServiceInfo pserv_info ;
      rsServiceControl->getServicesAllowed(_current_peer_id,pserv_info) ;

      bool locally_allowed = pserv_info.mServiceList.find(_current_service_id) != pserv_info.mServiceList.end();
      bool remotely_allowed = false ; // default, if the peer is offline

      if(rsServiceControl->getServicesProvided(_current_peer_id,pserv_info))
          remotely_allowed = pserv_info.mServiceList.find(_current_service_id) != pserv_info.mServiceList.end();

      QString local_status  = locally_allowed ?tr("Enabled for this peer") :tr("Disabled for this peer") ;
      QString remote_status = remotely_allowed?tr("Enabled by remote peer"):tr("Disabled by remote peer") ;

      if(!service_perms.mDefaultAllowed)
          local_status = tr("Globally switched Off") ;

      const QFont& font(_painter->font()) ;
      QFontMetrics fm(font);

      int text_size_x = 0 ;
      text_size_x = std::max(text_size_x,fm.width(service_name));
      text_size_x = std::max(text_size_x,fm.width(peer_name));
      text_size_x = std::max(text_size_x,fm.width(peer_id));
      text_size_x = std::max(text_size_x,fm.width(local_status));
      text_size_x = std::max(text_size_x,fm.width(remote_status));

       // draw a half-transparent rectangle

      QBrush brush ;
      brush.setColor(QColor::fromHsvF(0.0f,0.0f,1.0f,0.8f));
      brush.setStyle(Qt::SolidPattern) ;

      QPen pen ;
      pen.setWidth(2) ;
      pen.setBrush(Qt::black) ;

      _painter->setPen(pen) ;

      QRect info_pos( position.x() + 50*S/14.0, position.y() - 10*S/14.0, text_size_x + 10*S/14.0, line_height * 5 + 5*S/14.0) ;

      _painter->fillRect(info_pos,brush) ;
      _painter->drawRect(info_pos) ;

      // draw the text

      float x = info_pos.x()               + 5*S/14.0 ;
      float y = info_pos.y() + line_height + 1*S/14.0 ;

      _painter->drawText(QPointF(x,y), service_name)  ; y += line_height ;
      _painter->drawText(QPointF(x,y), peer_name)     ; y += line_height ;
      _painter->drawText(QPointF(x,y), peer_id)       ; y += line_height ;
      _painter->drawText(QPointF(x,y), remote_status) ; y += line_height ;
      _painter->drawText(QPointF(x,y), local_status)  ; y += line_height ;
  }

  _max_height = S*fMATRIX_START_Y + (peer_ids.size()+3) * S*fROW_SIZE ;
  _max_width  = matrix_start_x + (service_ids.size()+3) * S*fCOL_SIZE ;

  /* Stop the painter */
  _painter->end();
}

QRect RSPermissionMatrixWidget::computeNodePosition(int n_row,int n_col,bool selected) const
{
    float fact = selected?1.2f:1.0f;

    float S = QFontMetricsF(font()).height();

    return QRect(matrix_start_x + n_col * S*fCOL_SIZE + (S*fCOL_SIZE-S*fICON_SIZE_X*fact)/2,
                 S*fMATRIX_START_Y + n_row * S*fROW_SIZE + (S*fROW_SIZE-S*fICON_SIZE_Y*fact)/2,
                 S*fICON_SIZE_X*fact,
                 S*fICON_SIZE_Y*fact) ;
}

// This function is the inverse of the previous function. Given a mouse position, it
// computes the peer/service switch that is under the mouse. The zoom factor is not
// accounted for, on purpose.

bool RSPermissionMatrixWidget::computeServiceAndPeer(int x,int y,uint32_t& service_id,RsPeerId& peer_id) const
{
    // 1 - make sure that x and y are on a widget

    float S = QFontMetricsF(font()).height();

    x -= matrix_start_x ;
    y -= S*fMATRIX_START_Y ;

    if(x < 0 || x >= service_ids.size() * S*fCOL_SIZE) return false ;
    if(y < 0 || y >= peer_ids.size()    * S*fROW_SIZE) return false ;

    if( (x % (int)(S*fCOL_SIZE)) < (S*fCOL_SIZE - S*fICON_SIZE_X)/2) return false ;
    if( (x % (int)(S*fCOL_SIZE)) > (S*fCOL_SIZE + S*fICON_SIZE_X)/2) return false ;

    if( (y % (int)(S*fROW_SIZE)) < (S*fROW_SIZE - S*fICON_SIZE_Y)/2) return false ;
    if( (y % (int)(S*fROW_SIZE)) > (S*fROW_SIZE + S*fICON_SIZE_Y)/2) return false ;

    // 2 - find which widget, by looking into the service perm matrix

    service_id = service_ids[x / (int)(S*fCOL_SIZE)] ;
    peer_id = peer_ids[y / (int)(S*fCOL_SIZE)] ;

    return true ;
}

bool RSPermissionMatrixWidget::computeServiceGlobalSwitch(int x,int y,uint32_t& service_id) const
{
    // 1 - make sure that x and y are on a widget

    float S = QFontMetricsF(font()).height();

    x -= matrix_start_x ;
    y -= S*fMATRIX_START_Y ;

    if(x < 0 || x >= service_ids.size() * S*fCOL_SIZE) return false ;

    if( (x % (int)(S*fCOL_SIZE)) < (S*fCOL_SIZE - S*fICON_SIZE_X)/2) return false ;
    if( (x % (int)(S*fCOL_SIZE)) > (S*fCOL_SIZE + S*fICON_SIZE_X)/2) return false ;

    if( y < -S*fROW_SIZE ) return false ;
    if( y >  0        ) return false ;

    // 2 - find which widget, by looking into the service perm matrix

    service_id = service_ids[x / (int)(S*fCOL_SIZE)] ;

    return true ;
}

void RSPermissionMatrixWidget::defaultPermissionSwitched(uint32_t /* ServiceId */,bool /* b */)
{
    NOT_IMPLEMENTED ;
}

void RSPermissionMatrixWidget::userPermissionSwitched(uint32_t /* ServiceId */,const RsPeerId& /* friend_id */,bool /* b */)
{
    NOT_IMPLEMENTED ;
}



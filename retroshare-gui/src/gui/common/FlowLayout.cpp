/*******************************************************************************
 * gui/common/FlowLayout.cpp                                                   *
 *                                                                             *
 * Copyright (C) 2011, Retroshare Team <retroshare.project@gmail.com>          *
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

#include "gui/common/FlowLayout.h"
#include <QApplication>
#include <QtGui>
#include <QScrollBar>
#include <QDebug>

//*** FlowLayoutItem **********************************************************

void FlowLayoutItem::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton) {
		m_startPos = event->pos();
	}//if (event->button() == Qt::LeftButton)
	QWidget::mousePressEvent(event);
}

void FlowLayoutItem::mouseMoveEvent(QMouseEvent *event)
{
	if (event->buttons() & Qt::LeftButton) {
		int distance = (event->pos() - m_startPos).manhattanLength();
		if (distance >= QApplication::startDragDistance())
			performDrag();
	}//if (event->buttons() & Qt::LeftButton)
	QWidget::mouseMoveEvent(event);
}

void FlowLayoutItem::performDrag()
{
	QMimeData *mimeData = new QMimeData;
	mimeData->setText(m_myName);

	QDrag *drag = new QDrag(this);
	drag->setMimeData(mimeData);
	QPixmap pixmap=getDragImage();

	drag->setPixmap(pixmap.scaled(50,50,Qt::KeepAspectRatio, Qt::SmoothTransformation));
	/// Warning On Windows, Drag Pixmap size cannot exceed 50*50. ///
	drag->setHotSpot(QPoint(0, 0));
	Qt::DropAction dropAction = drag->exec(Qt::CopyAction);
	qDebug()<<dropAction;
}

void FlowLayoutItem::mouseReleaseEvent(QMouseEvent *event)
{
	Q_UNUSED(event);
	QWidget::mouseReleaseEvent(event);
}

void FlowLayoutItem::dragEnterEvent(QDragEnterEvent *event)
{
	FlowLayoutItem *source =
	    qobject_cast<FlowLayoutItem *>(event->source());
	if (source && source != this) {
		event->setDropAction(Qt::CopyAction);
		event->acceptProposedAction();
		return;
	}//if (source && source != this)
	QWidget *wid =
	    qobject_cast<QWidget *>(event->source());//QT5 return QObject
	FlowLayout *layout = 0;
	if (wid) layout =
	    qobject_cast<FlowLayout *>(wid->layout());
	if (layout) {
		event->setDropAction(Qt::CopyAction);
		event->acceptProposedAction();
		return;
	}//if (layout)
}

void FlowLayoutItem::dragMoveEvent(QDragMoveEvent *event)
{
	FlowLayoutItem *source =
	    qobject_cast<FlowLayoutItem *>(event->source());
	if (source && source != this) {
		event->setDropAction(Qt::CopyAction);
		event->acceptProposedAction();
		return;
	}//if (source && source != this)
	QWidget *wid =
	    qobject_cast<QWidget *>(event->source());//QT5 return QObject
	FlowLayout *layout = 0;
	if (wid) layout =
	    qobject_cast<FlowLayout *>(wid->layout());
	if (layout) {
		event->setDropAction(Qt::CopyAction);
		event->acceptProposedAction();
		return;
	}//if (layout)
}

void FlowLayoutItem::dropEvent(QDropEvent *event)
{
	QList <FlowLayoutItem*> list;
	FlowLayoutItem *source =
	    qobject_cast<FlowLayoutItem *>(event->source());
	if (source && source != this) {
		event->setDropAction(Qt::CopyAction);
		list << source;
	} else {//if (source && source != this)
		QWidget *wid =
		    qobject_cast<QWidget *>(event->source());//QT5 return QObject
		FlowLayout *layout = NULL;
		if (wid) layout =
		    qobject_cast<FlowLayout *>(wid->layout());
		if (layout) {
			QList<QLayoutItem *> listSel = layout->selectionList();
			int count = listSel.count();
			for (int curs = 0; curs < count; ++curs){
				QLayoutItem *layoutItem = listSel.at(curs);
				FlowLayoutItem *flItem = 0;
				if (layoutItem) flItem = qobject_cast<FlowLayoutItem*>(layoutItem->widget());
				if (flItem) list << flItem;
			}//for (int curs = 0; curs < count; ++curs)
		}//if (layout)
	}//else (source && source != this)
	if (!list.isEmpty()) {
		event->setDropAction(Qt::CopyAction);
		bool bAccept=true;
		emit flowLayoutItemDropped(list, bAccept);
		if (bAccept) event->acceptProposedAction();
	}//if (!list.empty())
}

//*** FlowLayoutWidget **********************************************************
FlowLayoutWidget::FlowLayoutWidget(QWidget *parent, int margin/*=-1*/, int hSpacing/*=-1*/, int vSpacing/*=-1*/)
  : QWidget(parent)
{
	FlowLayoutWidget(margin, hSpacing, vSpacing);
}

FlowLayoutWidget::FlowLayoutWidget(int margin/*=-1*/, int hSpacing/*=-1*/, int vSpacing/*=-1*/)
{
	FlowLayout *fl = new FlowLayout(this, margin, hSpacing, vSpacing);
	Q_UNUSED(fl)
	this->installEventFilter(this);
	this->setMouseTracking(true);
	this->setAcceptDrops(true);
	m_saParent = 0;
	m_sbVertical = 0;
}

FlowLayoutWidget::~FlowLayoutWidget()
{
}

bool FlowLayoutWidget::eventFilter(QObject *obj, QEvent *event)
{
	qDebug() << "FlowLayoutWidget:: obj type:" << obj->metaObject()->className() << " event type:" << event->type();
	updateParent();
	if (event->type() == QEvent::DragEnter) {
		QDragEnterEvent *dragEnterEvent = static_cast<QDragEnterEvent *>(event);
		if (dragEnterEvent){
			dragEnterEvent->setDropAction(Qt::IgnoreAction);
			dragEnterEvent->accept();
		}//if (dragEnterEvent)
	}//if (event->type() == QEvent::DragEnter)

	if (event->type() == QEvent::DragMove) {
		QDragMoveEvent *dragMoveEvent = static_cast<QDragMoveEvent *>(event);
		const int border = 20;
		if (obj==this && dragMoveEvent){
			if (m_sbVertical){
				int maxY = m_sbVertical->maximum();
				int currentY = m_sbVertical->value();
				int height = m_saParent->height();
				int topBorder = currentY + border;
				int bottomBorder = currentY + height - border;
				int dragY = dragMoveEvent->pos().y();
				qDebug() <<"Drag event:" << dragY ;
				int dY = (dragY<topBorder)?dragY - topBorder:((dragY>bottomBorder)?dragY - bottomBorder:0);
				qDebug() << "dY:" << dY << " m_lastYPos:" << m_lastYPos << "(dragY-m_lastYPos)*dY:" << (dragY-m_lastYPos)*dY;
				int newValue=currentY+dY;
				if ((abs(dY)<border) && ((dragY-m_lastYPos)*dY >= 0)){
					if (newValue>maxY) newValue=maxY;
					if (newValue<0) newValue=0;
					m_sbVertical->setValue(newValue);
				} else {
					newValue=currentY;
				}//if ((abs(dY)<border) && ((dragY-m_lastYPos)*dY >= 0))
				m_lastYPos = dragY+(newValue-currentY);
			}//if (sbVertical)
		}//if (obj==this && dragMoveEvent)
		if (obj==m_sbVertical && dragMoveEvent){
			if (m_sbVertical->isVisible()){
				int maxY = m_sbVertical->maximum();
				double height = m_sbVertical->height()*1.0;
				double scale = (maxY/height);
				int dragY = dragMoveEvent->pos().y();
				qDebug() << "Drag maxY:" << maxY << "height:" << height  << "scale:" << scale << "dragY:" << dragY;
				m_sbVertical->setValue(dragY*scale);
			}//if (sbVertical->isVisible())
		}//if (obj==this && dragMoveEvent)
	}//if (event->type() == QEvent::DragMove)

	// standard event processing
	return QObject::eventFilter(obj, event);
}

void FlowLayoutWidget::updateParent()
{
	if (parent()){
		if (!m_saParent){
			if (parent()->objectName() == "qt_scrollarea_viewport"){
				m_saParent = qobject_cast<QScrollArea*>(parent()->parent());
			} else {
				m_saParent = qobject_cast<QScrollArea*>(parent());
			}//if (parent()->objectName() == "qt_scrollarea_viewport")
			if (m_saParent){
				m_saParent->installEventFilter(this);
				m_saParent->setAcceptDrops(true);
				m_saParent->setMouseTracking(true);
			}//if (saParent)
		}//if (!saParent)
		if (m_saParent && !m_sbVertical){
			m_sbVertical= m_saParent->verticalScrollBar();
			if (m_sbVertical){
				m_sbVertical->installEventFilter(this);
				m_sbVertical->setAcceptDrops(true);
				m_sbVertical->setMouseTracking(true);
			}//if (sbVertical)
		}//if (saParent && !sbVertical)
	}//if (parent())
}

//*** FlowLayout **********************************************************

FlowLayout::FlowLayout(QWidget *parent, int margin/*=-1*/, int hSpacing/*=-1*/, int vSpacing/*=-1*/)
  : QLayout(parent), m_hSpace(hSpacing), m_vSpace(vSpacing)
{
	setContentsMargins(margin, margin, margin, margin);
	this->installEventFilter(this);
}

FlowLayout::FlowLayout(int margin/*=-1*/, int hSpacing/*=-1*/, int vSpacing/*=-1*/)
  : m_hSpace(hSpacing), m_vSpace(vSpacing)
{
	setContentsMargins(margin, margin, margin, margin);
	this->installEventFilter(this);
}

FlowLayout::~FlowLayout()
{
	QLayoutItem *item;
	while ((item = takeAt(0)))
		delete item;
}

bool FlowLayout::eventFilter(QObject *obj, QEvent *event)
{
	qDebug() << "FlowLayout::obj type:" << obj->metaObject()->className() << " event type:" << event->type();

	if (event->type() == QEvent::MouseButtonPress) {
		m_startPos = QCursor::pos();
	}//if (event->type() == QEvent::MouseButtonPress)

	if (event->type() == QEvent::MouseButtonRelease) {
		QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
		int distance = (QCursor::pos() - m_startPos).manhattanLength();
		if (distance < QApplication::startDragDistance()){
			unsetCurrent();
			m_currentIndex=indexAtGlobal(QCursor::pos());
			setCurrent();
			bool invert = (mouseEvent->modifiers() &= Qt::ControlModifier);
			if (mouseEvent->modifiers() &= Qt::ShiftModifier){
				m_selStartIndex=(m_selStartIndex<m_currentIndex)?m_selStartIndex:m_currentIndex;
				m_selStopIndex=(m_selStopIndex>m_currentIndex)?m_selStopIndex:m_currentIndex;
			} else {
				m_selStartIndex=m_selStopIndex=m_currentIndex;
				if (mouseEvent->modifiers() != Qt::ControlModifier){
					foreach (QLayoutItem *item, m_selectionList) {
						FlowLayoutItem *fli = qobject_cast<FlowLayoutItem *>(item->widget());
						if (fli) fli->setIsSelected(false);
						m_selectionList.removeOne(item);
					}
				}//if (mouseEvent->modifiers() != Qt::ControlModifier)
			}//if (mouseEvent->modifiers()
			addSelection(invert);
			setCurrent();
		}//if (distance < QApplication::startDragDistance())
		doLayout(geometry(),false);
	}//if (event->type() == QEvent::MouseButtonRelease)

	if (event->type() == QEvent::MouseMove) {
		QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
		if (mouseEvent->buttons() & Qt::LeftButton) {
			int distance = (QCursor::pos() - m_startPos).manhattanLength();
			if (distance >= QApplication::startDragDistance()){
				if (!m_selectionList.isEmpty()) {
					QLayoutItem *item=itemAtGlobal(m_startPos);
					if (item) {
						if (m_selectionList.contains(item)){
							performDrag();
							return true; // eat event
						}//if (m_selectionList.contains(item))
					}//if (item)
				}//if (!m_selectionList.isEmpty())
			}//if (distance >= QApplication::startDragDistance())
		}//if (mouseEvent->buttons() & Qt::LeftButton)
	}//if (event->type() == QEvent::MouseMove)

	if (event->type() == QEvent::KeyRelease) {
		QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

		if ((keyEvent->key()==Qt::Key_A) && (keyEvent->modifiers() &= Qt::ControlModifier)){
			int count = m_itemList.count();
			int selected = m_selectionList.count();
			if (count != selected){
				for (int curs=0; curs<count; ++curs){
					QLayoutItem *item=itemAt(curs);
					if (item) {
						if (!m_selectionList.contains(item)){
							FlowLayoutItem *fli = qobject_cast<FlowLayoutItem *>(item->widget());
							if (fli) fli->setIsSelected(true);
							m_selectionList.append(item);
						}//if (!m_selectionList.contains(item))
					}//if (item)
				}//for (int curs=0; curs<count; ++curs)
			} else {
				foreach (QLayoutItem *item, m_selectionList) {
					FlowLayoutItem *fli = qobject_cast<FlowLayoutItem *>(item->widget());
					if (fli) fli->setIsSelected(false);
					m_selectionList.removeOne(item);
				}
			}//if (count != selected)

			doLayout(geometry(),false);
			event->accept();
		}//if ((keyEvent->key()==Qt::Key_A) && (keyEvent->modifiers() &= Qt::ControlModifier))

		if ((keyEvent->key()==Qt::Key_Space)){
			if ((keyEvent->modifiers() &= Qt::ShiftModifier) || (keyEvent->modifiers() &= Qt::ControlModifier)){
				addSelection((keyEvent->modifiers() &= Qt::ControlModifier));
			}//if ((keyEvent->modifiers() &= Qt::ShiftModifier) || (keyEvent->modifiers() &= Qt::ControlModifier))
			doLayout(geometry(),false);
			m_selStartIndex = m_selStopIndex = m_currentIndex;
			event->accept();
		}//if ((keyEvent->key()==Qt::Key_Space))

		if ((keyEvent->key()==Qt::Key_Left)){
			unsetCurrent();
			if (m_currentIndex>0) m_currentIndex-=1;
			if ((keyEvent->modifiers() &= Qt::ShiftModifier) || (keyEvent->modifiers() &= Qt::ControlModifier)){
				m_selStartIndex=m_currentIndex;
				addSelection((keyEvent->modifiers() &= Qt::ControlModifier));
			}//if ((keyEvent->modifiers() &= Qt::ShiftModifier) || (keyEvent->modifiers() &= Qt::ControlModifier))
			doLayout(geometry(),false);
			m_selStartIndex = m_selStopIndex = m_currentIndex;
			event->accept();
			setCurrent();
		}//if ((keyEvent->key()==Qt::Key_Left))

		if ((keyEvent->key()==Qt::Key_Right)){
			unsetCurrent();
			if (m_currentIndex<(m_itemList.count()-1)) m_currentIndex+=1;
			if ((keyEvent->modifiers() &= Qt::ShiftModifier) || (keyEvent->modifiers() &= Qt::ControlModifier)){
				m_selStopIndex=m_currentIndex;
				addSelection((keyEvent->modifiers() &= Qt::ControlModifier));
			}//if ((keyEvent->modifiers() &= Qt::ShiftModifier) || (keyEvent->modifiers() &= Qt::ControlModifier))
			doLayout(geometry(),false);
			m_selStartIndex = m_selStopIndex = m_currentIndex;
			event->accept();
			setCurrent();
		}//if ((keyEvent->key()==Qt::Key_Right))

		if ((keyEvent->key()==Qt::Key_Up)){
			unsetCurrent();
			QLayoutItem* item = currentItem();
			if (item) {
				QRect loc = item->geometry();
				int vSpace = verticalSpacing();
				QPoint pos = QPoint(loc.left()+loc.width()/2, loc.top()-vSpace-2);
				int index = -1;
				while ((pos.y()>0) && (index < 0)){
					index = indexAtParent(pos);
					pos.setY(pos.y()-2);
				}//while ((pos.y()>0) && (index < 0))
				m_currentIndex = (index>0)?index:0;
				m_currentIndex = (index<=m_itemList.count())?index:m_itemList.count();
				if ((keyEvent->modifiers() &= Qt::ShiftModifier) || (keyEvent->modifiers() &= Qt::ControlModifier)){
					m_selStopIndex = m_currentIndex;
					addSelection((keyEvent->modifiers() &= Qt::ControlModifier));
				}//if ((keyEvent->modifiers() &= Qt::ShiftModifier) || (keyEvent->modifiers() &= Qt::ControlModifier))
				doLayout(geometry(),false);
				m_selStartIndex = m_selStopIndex = m_currentIndex;
				event->accept();
			}//if (wid)
			setCurrent();
		}//if ((keyEvent->key()==Qt::Key_Right))

		if ((keyEvent->key()==Qt::Key_Down)){
			unsetCurrent();
			QLayoutItem* item = currentItem();
			if (item){
				QRect loc = item->geometry();
				int vSpace = verticalSpacing();
				QPoint pos = QPoint(loc.left()+loc.width()/2, loc.bottom()+vSpace+2);
				int index = -1;
				while ((pos.y()<geometry().bottom()) && (index < 0)){
					index = indexAtParent(pos);
					pos.setY(pos.y()+2);
				}//while ((pos.y()<geometry().bottom()) && (index < 0))
				m_currentIndex = (index>0)?index:0;
				m_currentIndex = (index<=m_itemList.count())?index:m_itemList.count();
				if ((keyEvent->modifiers() &= Qt::ShiftModifier) || (keyEvent->modifiers() &= Qt::ControlModifier)){
					m_selStopIndex = m_currentIndex;
					addSelection((keyEvent->modifiers() &= Qt::ControlModifier));
				}//if ((keyEvent->modifiers() &= Qt::ShiftModifier) || (keyEvent->modifiers() &= Qt::ControlModifier))
				doLayout(geometry(),false);
				m_selStartIndex = m_selStopIndex = m_currentIndex;
				event->accept();
			}//if (wid)
			setCurrent();
		}//if ((keyEvent->key()==Qt::Key_Right))

	}//if (event->type() == QEvent::KeyRelease)

	// standard event processing
	return QObject::eventFilter(obj, event);
}

void FlowLayout::unsetCurrent()
{
	QLayoutItem *oldItem = currentItem();
	if (oldItem) {
		FlowLayoutItem *fli = qobject_cast<FlowLayoutItem *>(oldItem->widget());
		if (fli) fli->setIsCurrent(false);
	}//if (oldItem)
}

void FlowLayout::setCurrent()
{
	QLayoutItem *newItem = currentItem();
	if (newItem) {
		FlowLayoutItem *fli = qobject_cast<FlowLayoutItem *>(newItem->widget());
		if (fli) fli->setIsCurrent(true);
	}//if (newItem)
}

void FlowLayout::addItem(QLayoutItem *item)
{
	m_itemList.append(item);
	item->widget()->installEventFilter(this);
}

void FlowLayout::addItem(FlowLayoutItem *item)
{
	QWidget *widget = qobject_cast<QWidget *>(item);
	addWidget(widget);
}

int FlowLayout::horizontalSpacing() const
{
	if (m_hSpace >= 0) {
		return m_hSpace;
	} else {
		return smartSpacing(QStyle::PM_LayoutHorizontalSpacing);
	}//if (m_hSpace >= 0)
}
void FlowLayout::setHorizontalSpacing(int &h)
{
	if (h>=0) {
		m_hSpace = h;
	} else {
		m_hSpace = -1;
	}//if (h>=0)
	doLayout(geometry(), false);
}

int FlowLayout::verticalSpacing() const
{
	if (m_vSpace >= 0) {
		return m_vSpace;
	} else {
		return smartSpacing(QStyle::PM_LayoutVerticalSpacing);
	}//if (m_vSpace >= 0)
}
void FlowLayout::setVerticalSpacing(int &v)
{
	if (v>=0) {
		m_vSpace = v;
	} else {
		m_vSpace = -1;
	}//if (v>=0)
	doLayout(geometry(), false);
}

int FlowLayout::count() const
{
	return m_itemList.size();
}

QLayoutItem *FlowLayout::itemAt(int index) const
{
	return m_itemList.value(index);
}

QLayoutItem *FlowLayout::itemAtGlobal(const QPoint &p) const
{
	return m_itemList.value(indexAtGlobal(p));
}

QLayoutItem *FlowLayout::itemAtParent(const QPoint &p) const
{
	return m_itemList.value(indexAtParent(p));
}

int FlowLayout::indexAtGlobal(const QPoint &p) const
{
	int count = m_itemList.size();
	for(int curs=0; curs<count; ++curs){
		QWidget *widget = m_itemList.value(curs)->widget();
		if(widget) {
			QPoint pos = widget->mapFromGlobal(p);
			if((pos.x()>0) && (pos.x()<widget->width()))
				if((pos.y()>0) && (pos.y()<widget->height()))
					return curs;
		}//if(widget)
	}//for(int curs=0; curs<count; ++curs)
	return -1;
}

int FlowLayout::indexAtParent(const QPoint &p) const
{
	int count = m_itemList.size();
	for(int curs=0; curs<count; ++curs){
		QWidget *widget = m_itemList.value(curs)->widget();
		if(widget) {
			QPoint pos = widget->mapFromParent(p);
			if((pos.x()>=0) && (pos.x()<=widget->width()))
				if((pos.y()>=0) && (pos.y()<=widget->height()))
					return curs;

			if (pos.y()<0) break;//In line below so don't check other item.
		}//if(widget)
	}//for(int curs=0; curs<count; ++curs)
	return -1;
}

QLayoutItem *FlowLayout::takeAt(int index)
{
	if (index >= 0 && index < m_itemList.size())
		return m_itemList.takeAt(index);
	else
		return 0;
}

Qt::Orientations FlowLayout::expandingDirections() const
{
	return Qt::Horizontal;
}

bool FlowLayout::hasHeightForWidth() const
{
	return true;
}

int FlowLayout::heightForWidth(int width) const
{
	int height = doLayout(QRect(0, 0, width, 0), true);
	return height;
}

void FlowLayout::setGeometry(const QRect &rect)
{
	QLayout::setGeometry(rect);
	doLayout(rect, false);
}

QSize FlowLayout::sizeHint() const
{
	return minimumSize();
}

QSize FlowLayout::minimumSize() const
{
	QSize size;
	QLayoutItem *item;
	foreach (item, m_itemList)
		size = size.expandedTo(item->minimumSize());

	size += QSize(2*margin(), 2*margin());
	return size;
}

void FlowLayout::addSelection(bool invert){
	for (int curs=m_selStartIndex; curs<=m_selStopIndex; ++curs){
		QLayoutItem *item=itemAt(curs);
		if (item) {
			FlowLayoutItem *fli = qobject_cast<FlowLayoutItem *>(item->widget());
			if (invert && m_selectionList.contains(item)){
				if (fli) fli->setIsSelected(false);
				m_selectionList.removeOne(item);
			} else {
				if (fli) fli->setIsSelected(true);
				m_selectionList.append(item);
			}//if (m_selectionList.contains(item))
		}//if (item)
	}//for (int curs=m_selStartIndex; curs<=m_selStopIndex; ++curs)
}

void FlowLayout::performDrag()
{
	QPixmap dragPixmap;
	bool atLeastOnePixmap = false;
	int count = m_selectionList.count();
	for (int curs=0; curs<count; ++curs){
		QLayoutItem *layoutItem =  m_selectionList.at(curs);
		if (layoutItem){
			QWidget *widget = layoutItem->widget();
			if (widget) {
				QPixmap itemPixmap;
				FlowLayoutItem *item = qobject_cast<FlowLayoutItem *>(widget);
				if (item){
					itemPixmap = item->getDragImage();
				} else {
#if QT_VERSION >= QT_VERSION_CHECK (5, 0, 0)
					itemPixmap = widget->grab();//QT5
#else
					itemPixmap = QPixmap::grabWidget(widget);
#endif
				}//if (item)

				itemPixmap = itemPixmap.scaled(50,50,Qt::KeepAspectRatio, Qt::SmoothTransformation);
				/// Warning On Windows, Drag Pixmap size cannot exceed 50*50. ///
				if (curs==0) dragPixmap = itemPixmap;
				QPixmap oldPixmap = dragPixmap;
				if (curs!=0) dragPixmap = QPixmap(oldPixmap.width() + 20 , oldPixmap.height());
				dragPixmap.fill(widget->palette().background().color());
				QPainter painter(&dragPixmap);
				painter.drawPixmap(0, 0, oldPixmap);
				if (curs!=0) painter.drawPixmap((20 * curs), 0, itemPixmap);

				atLeastOnePixmap = true;
			}//if (widget)
		}//if (layoutItem)
	}//for (int curs=0; curs<count; ++curs)
	if (atLeastOnePixmap) {
		QMimeData *mimeData = new QMimeData;
		mimeData->setText("");
		QDrag *drag = new QDrag(this->parentWidget());
		drag->setMimeData(mimeData);

		drag->setPixmap(dragPixmap);
		drag->setHotSpot(QPoint(0, 0));
		drag->exec(Qt::CopyAction);
	}//if (atLeastOnePixmap)
}

int FlowLayout::doLayout(const QRect &rect, bool testOnly) const
{
	int left, top, right, bottom;
	getContentsMargins(&left, &top, &right, &bottom);
	QRect effectiveRect = rect.adjusted(+left, +top, -right, -bottom);
	int x = effectiveRect.x();
	int y = effectiveRect.y();
	int lineHeight = 0;

	int count = m_itemList.size();
	for (int curs=0; curs<count; ++curs) {
		QLayoutItem *item=m_itemList.value(curs);
		QWidget *wid = item->widget();
		int spaceX = horizontalSpacing();
		if (spaceX == -1)
			spaceX = wid->style()->layoutSpacing(
			      QSizePolicy::PushButton, QSizePolicy::PushButton, Qt::Horizontal);
		int spaceY = verticalSpacing();
		if (spaceY == -1)
			spaceY = wid->style()->layoutSpacing(
			      QSizePolicy::PushButton, QSizePolicy::PushButton, Qt::Vertical);

		int nextX = x + item->sizeHint().width() + spaceX;
		if (nextX - spaceX > effectiveRect.right() && lineHeight > 0) {
			x = effectiveRect.x();
			y = y + lineHeight + spaceY;
			nextX = x + item->sizeHint().width() + spaceX;
			lineHeight = 0;
		}//if (nextX - spaceX > effectiveRect.right() && lineHeight > 0)

		if (!testOnly){
			item->setGeometry(QRect(QPoint(x, y), item->sizeHint()));
			bool selected = m_selectionList.contains(item);
			bool isCurrent = (curs==currentIndex());
			QString solid = isCurrent?"dot-dash":"inset";
			QString color = selected?"blue":isCurrent?"gray":"";
			QString border = (selected||isCurrent)?QString("border: 1px %1 %2").arg(solid).arg(color):"";
			QString widName = wid->objectName();
			QString style;
			if (widName.isEmpty()){
				style=border;
			} else {
				//For Custom QWidget, change paintEvent as FlowLayoutItem. cf:http://qt-project.org/doc/qt-4.8/stylesheet-reference.html
				style = QString("QWidget#%1\n{\n%2\n}\n").arg(wid->objectName(),border);
			}//if (widName.isEmpty())
			///Warning: Test if != to not ask a new redraw all time ///
			if (wid->styleSheet()!=style) wid->setStyleSheet(style);
		}//if (!testOnly)

		x = nextX;
		lineHeight = qMax(lineHeight, item->sizeHint().height());
	}//for (curs=0; curs<count; ++curs)

	return y + lineHeight - rect.y() + bottom;
}

int FlowLayout::smartSpacing(QStyle::PixelMetric pm) const
{
	QObject *parent = this->parent();
	if (!parent) {
		return -1;
	} else if (parent->isWidgetType()) {
		QWidget *pw = static_cast<QWidget *>(parent);
		return pw->style()->pixelMetric(pm, 0, pw);
	} else {
		return static_cast<QLayout *>(parent)->spacing();
	}//if (!parent)
}

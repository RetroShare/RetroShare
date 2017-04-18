/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/
/** WARNING: 	QT Designer don't accept Custom Layout, maybe on QT5
 *You can get widget's children like this:
 *	FlowLayout *flowLayout = new FlowLayout;
 *	//First Get Item in Qt Designer
 *	int count = ui->id->children().count();
 *	for (int curs = 0; curs < count; ++curs){
 *		QObject *obj = ui->id->children().at(curs);
 *		QWidget *wid = qobject_cast<QWidget *>(obj);
 *		if (wid) flowLayout->addWidget(wid);
 *	}//for (int curs = 0; curs < count; ++curs)
 */



#ifndef FLOWLAYOUT_H
#define FLOWLAYOUT_H

#include <QKeyEvent>
#include <QLayout>
#include <QPainter>
#include <QRect>
#include <QScrollArea>
#include <QStyle>
#include <QStyleOption>
#include <QWidget>
#include <QWidgetItem>

/// \class FlowLayoutItem
/// \brief The FlowLayoutItem class
///FlowLayoutItem represents FlowLayout item.
///Derivatives from it to make a custom widget
///and to get Drag and Drop better.
class FlowLayoutItem : public QWidget
{
	Q_OBJECT

public:
	FlowLayoutItem(QString name=QString(), QWidget *parent=0)
	  : QWidget(parent), m_myName(name), m_isSelected(false), m_isCurrent(false)
	{
		setFocusPolicy(Qt::StrongFocus);
		setAcceptDrops(true);
	}
	~FlowLayoutItem(){}

	/// \brief getImage
	/// \return Image to represent your widget (not necessary all the widget).
	virtual const QPixmap getImage() =0;
	/// \brief getDragImage
	/// \return Image to represent your widget when dragged (not necessary all the widget).
	virtual const QPixmap getDragImage() =0;
	/// \brief setName
	/// \param value
	virtual void setName(QString value) {m_myName=value;}
	/// \brief getName
	/// \return the name of your widget;
	virtual const QString getName() const {return m_myName;}
	/// \brief setIsSelected
	/// \param value
	virtual void setIsSelected(bool value) {m_isSelected=value;}
	/// \brief getSelected
	/// \return if item is selected
	virtual bool isSelected() const {return m_isSelected;}
	/// \brief setCurrent
	/// \param value
	virtual void setIsCurrent(bool value) {m_isCurrent=value;}
	/// \brief isCurrent
	/// \return if item is the current one
	virtual bool isCurrent() const {return m_isCurrent;}

	/// \brief paintEvent
	///To get Style working on widget and not on all children.
	void paintEvent(QPaintEvent *)
	{
		QStyleOption opt;
		opt.init(this);
		QPainter p(this);
		style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
	}

signals:
	/// \brief flowLayoutItemDropped
	/// \param listItem: QList with all item dropped.
	/// \param bAccept: set it to true to accept drop event.
	///Signales when the widget is dropped.
	void flowLayoutItemDropped(QList <FlowLayoutItem*> listItem, bool &bAccept);

	/// \brief updated
	///Signales when the image (getImage) is updated.
	void imageUpdated();

protected:
	void keyPressEvent(QKeyEvent *event){event->ignore();}
	void keyReleaseEvent(QKeyEvent *event){event->ignore();}
	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	void dragEnterEvent(QDragEnterEvent *event);
	void dragMoveEvent(QDragMoveEvent *event);
	void dropEvent(QDropEvent *event);

	QString m_myName;
	bool m_isSelected;
	bool m_isCurrent;

private:
	void performDrag();

private:
	QPoint m_startPos;
};

/// \class FlowLayout
/// \brief The FlowLayout class
///Class FlowLayout arranges child widgets from left to right
///and top to bottom in a top-level widget.
///The items are first laid out horizontally and
///then vertically when each line in the layout runs out of space.
class FlowLayout : public QLayout
{
	Q_OBJECT
	Q_PROPERTY(int horizontalSpacing READ horizontalSpacing WRITE setHorizontalSpacing)
	Q_PROPERTY(int verticalSpacing READ verticalSpacing WRITE setVerticalSpacing)
	Q_PROPERTY(Qt::Orientations expandingDirections READ expandingDirections)
	Q_PROPERTY(int count READ count)
	Q_PROPERTY(QSize minimumSize READ minimumSize)
	Q_PROPERTY(QLayoutItem *currentItem READ currentItem)
	Q_PROPERTY(int currentIndex READ currentIndex)

public:
	FlowLayout(QWidget *parent, int margin = -1, int hSpacing = -1, int vSpacing = -1);
	FlowLayout(int margin = -1, int hSpacing = -1, int vSpacing = -1);
	~FlowLayout();

	///
	/// \brief addItem QLayoutItem
	/// \param item
	///to add a new item. (normally called by addWidget)
	void addItem(QLayoutItem *item);
	///
	/// \brief addItem FlowLayoutItem
	/// \param item
	///To add a new FlowLayoutItem item.
	void addItem(FlowLayoutItem *item);
	///
	/// \brief horizontalSpacing
	/// \return int
	///Returns the horizontal spacing of items in the layout.
	int horizontalSpacing() const;
	///
	/// \brief setHorizontalSpacing
	/// \param h
	///To set the horizontal spacing of items in the layout.
	void setHorizontalSpacing(int &h);
	///
	/// \brief verticalSpacing
	/// \return int
	///Returns the vertical spacing of items in the layout.
	int verticalSpacing() const;
	///
	/// \brief setVerticalSpacing
	/// \param v
	///To set the horizontal spacing of items in the layout.
	void setVerticalSpacing(int &v);
	///
	/// \brief expandingDirections
	/// \return Qt::Orientations
	///Returns the Qt::Orientations in which the layout can make
	/// use of more space than its sizeHint().
	Qt::Orientations expandingDirections() const;
	///
	/// \brief hasHeightForWidth
	/// \return bool
	///Indicates if heightForWidth() is implemented.
	bool hasHeightForWidth() const;
	///
	/// \brief heightForWidth
	/// \return int
	///To adjust to widgets of which height is dependent on width.
	int heightForWidth(int) const;
	///
	/// \brief count
	/// \return int
	///Returns items count.
	int count() const;
	///
	/// \brief itemAt
	/// \param index
	/// \return QLayoutItem*
	///Returns item at index position.
	QLayoutItem *itemAt(int index) const;
	///
	/// \brief itemAtGlobal
	/// \param p
	/// \return QLayoutItem*
	///Returns item at position indicate with p. This position is on global screen coordinates.
	QLayoutItem *itemAtGlobal(const QPoint &p) const;
	///
	/// \brief itemAtParent
	/// \param p
	/// \return QLayoutItem*
	///Returns item at position indicate with p. This position is on parent screen coordinates.
	QLayoutItem *itemAtParent(const QPoint &p) const;
	///
	/// \brief indexAtGlobal
	/// \param p
	/// \return int
	///Returns index of item at position indicate with p. This position is on global screen coordinates.
	int indexAtGlobal(const QPoint &p) const;
	///
	/// \brief indexAtParent
	/// \param p
	/// \return int
	///Returns index of item at position indicate with p. This position is on parent screen coordinates.
	int indexAtParent(const QPoint &p) const;
	///
	/// \brief minimumSize
	/// \return QSize
	///Returns the minimum size of all child items.
	QSize minimumSize() const;
	///
	/// \brief setGeometry
	/// \param rect
	///Set the geometry of the layout relative to its parent and excluding the window frame.
	///It's for redraw item list when it was resized.
	void setGeometry(const QRect &rect);
	///
	/// \brief sizeHint
	/// \return QSize
	///Returns recommended (minimum) size.
	QSize sizeHint() const;
	///
	/// \brief takeAt
	/// \param index
	/// \return QLayoutItem*
	///Take an item in list (erase it).
	QLayoutItem *takeAt(int index);
	///
	/// \brief selectionList
	/// \return QList<QLayoutItem *>
	///Returns the list of selected items.
	QList<QLayoutItem *> selectionList() const { return m_selectionList;}
	///
	/// \brief currentItem
	/// \return QLayoutItem *
	///Returns the current (only one) item.
	QLayoutItem *currentItem() const { return m_itemList.value(m_currentIndex);}
	///
	/// \brief currentIndex
	/// \return int
	///Returns index of current item.
	int currentIndex() const { return m_currentIndex;}

protected:
	bool eventFilter(QObject *obj, QEvent *event);
	void unsetCurrent();
	void setCurrent();

private:
	// Redraw all the layout
	int doLayout(const QRect &rect, bool testOnly) const;
	//To get the default spacing for either the top-level layouts or the sublayouts.
	//The default spacing for top-level layouts, when the parent is a QWidget,
	//will be determined by querying the style.
	//The default spacing for sublayouts, when the parent is a QLayout,
	//will be determined by querying the spacing of the parent layout.
	int smartSpacing(QStyle::PixelMetric pm) const;
	//Execute drag action.
	void performDrag();
	//Update selection with m_selStartIndex and m_selStopIndex.
	//If invert, item selection is toggled.
	void addSelection(bool invert);

	QList<QLayoutItem *> m_itemList;
	QList<QLayoutItem *> m_selectionList;
	int m_selStartIndex;
	int m_selStopIndex;
	int m_currentIndex;

	QPoint m_startPos;

	int m_hSpace;
	int m_vSpace;
};

///
/// \brief The FlowLayoutWidget class
///Class FlowLayoutWidget provide a simple widget with FlowLayout as layout.
///This could be integrate on QtDesigner
class FlowLayoutWidget : public QWidget
{
	Q_OBJECT

public:
	FlowLayoutWidget(QWidget *parent, int margin = -1, int hSpacing = -1, int vSpacing = -1);
	FlowLayoutWidget(int margin = -1, int hSpacing = -1, int vSpacing = -1);
	~FlowLayoutWidget();

protected:
	bool eventFilter(QObject *obj, QEvent *event);

private:
	void updateParent();

	QScrollArea *m_saParent;
	QScrollBar *m_sbVertical;
	int m_lastYPos;

};

#endif

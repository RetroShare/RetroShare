/***************************************************************************
 *   Copyright (C) 2007 by Balázs Béla                                     *
 *   balazsbela@gmail.com                                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation version 2 of the License                 *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


#ifndef DELEGATES_H
#define DELEGATES_H


#include <QItemDelegate>
#include <QModelIndex>
#include <QObject>
#include <QSize>
#include <QSpinBox>
#include <QDateTimeEdit>
#include <QKeyEvent>
#include <QApplication>
#include <QComboBox>
#include <QLineEdit>
#include <QDateEdit>
#include <QTimeEdit>

//The delegate for the absence table
class absenceDelegate : public QItemDelegate
{
 Q_OBJECT
 public:
   absenceDelegate(QObject *parent = 0);
   QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,const QModelIndex &index) const;
   void setEditorData(QWidget *editor, const QModelIndex &index) const;
   void setModelData(QWidget *editor, QAbstractItemModel *model,const QModelIndex &index) const;
   void updateEditorGeometry(QWidget *editor,const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

//The delegate for the absence table
class markDelegate : public QItemDelegate
{
 Q_OBJECT
 public:
   markDelegate(QObject *parent = 0);
   QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,const QModelIndex &index) const;
   void setModelData(QWidget *editor, QAbstractItemModel *model,const QModelIndex &index) const;
   void updateEditorGeometry(QWidget *editor,const QStyleOptionViewItem &option, const QModelIndex &index) const;
};


//Delegate for the to-do list
class todoDelegate: public QItemDelegate
{
 Q_OBJECT
 public:
   todoDelegate(QObject *parent = 0);
   void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
   QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,const QModelIndex &index) const;
   void setModelData(QWidget *editor, QAbstractItemModel *model,const QModelIndex &index) const;
   void updateEditorGeometry(QWidget *editor,const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

//Delegate for the schedule

class scheduleDelegate : public QItemDelegate
{
 Q_OBJECT
 public:
   scheduleDelegate(QObject *parent = 0);
   QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,const QModelIndex &index) const;
   void setModelData(QWidget *editor, QAbstractItemModel *model,const QModelIndex &index) const;
   void updateEditorGeometry(QWidget *editor,const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

class timeTableDelegate : public QItemDelegate
{
 Q_OBJECT
 public:
   timeTableDelegate(QObject *parent = 0);
   QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,const QModelIndex &index) const;
   void setModelData(QWidget *editor, QAbstractItemModel *model,const QModelIndex &index) const;
   void updateEditorGeometry(QWidget *editor,const QStyleOptionViewItem &option, const QModelIndex &index) const;
};



#endif

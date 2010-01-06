/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006-2010,  RetroShare Team
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

#ifndef _DETAILSDIALOG_H
#define _DETAILSDIALOG_H

#include <QtGui>
#include <QObject>
#include <QModelIndex>
#include <QVariant>

#include "ui_DetailsDialog.h"


class DetailsDialog : public QDialog
{
  Q_OBJECT

public:
  /** Default constructor */
  DetailsDialog(QWidget *parent = 0, Qt::WFlags flags = 0);
  /** Default destructor */
  ~DetailsDialog();


public slots:
  /** Overloaded QWidget.show */
  void show();
  
  void setFileName(const QString & filename);
	void setHash(const QString & hash);
	void setLink(const QString & link);
	void setSize(const qulonglong & size);
	void setStatus(const QString & status);
	void setPriority(const QString & priority); 
	void setSources(const QString & sources);
	void setDatarate(const double & datarate);
	void setCompleted(const qulonglong & completed); 
  void setRemaining(const qulonglong & remaining) ;
  void setType(const QString & type); 
 
protected:
  void closeEvent (QCloseEvent * event);
  
private slots:
  void on_ok_dButton_clicked();
  void on_cancel_dButton_clicked();
  
private:

	class QStandardItemModel *CommentsModel;


  
  /** Qt Designer generated object */
  Ui::DetailsDialog ui;
};

#endif


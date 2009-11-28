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

#include "delegates.h"

//----------------------------------------------------Absence table delegate------------------------------------------------
absenceDelegate::absenceDelegate(QObject *parent) : QItemDelegate(parent)
{
}

QWidget *absenceDelegate::createEditor(QWidget *parent,const QStyleOptionViewItem &/* option*/,const QModelIndex &/*index*/) const
{
        QDateEdit *editor = new QDateEdit(QDate::currentDate(),parent);
        editor->setDisplayFormat("dd/M/yyyy");
        editor->setCalendarPopup(true);
        return editor;
}

void absenceDelegate::setEditorData(QWidget *editor,const QModelIndex &index) const
{
        QDateEdit *dateEditor = qobject_cast<QDateEdit *>(editor);
        if (dateEditor) 
            dateEditor->setDate(QDate::fromString(index.model()->data(index, Qt::EditRole).toString(), "d/M/yyyy"));
}


void absenceDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,const QModelIndex &index) const
{
    
       QDateEdit *dateEditor = qobject_cast<QDateEdit *>(editor);
        if (dateEditor) 
            model->setData(index, dateEditor->date().toString("dd/M/yyyy"));
    
}

void absenceDelegate::updateEditorGeometry(QWidget *editor,const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
    editor->setGeometry(option.rect);
}

//----------------------------------------------------To-do list delegate----------------------------------------------------

todoDelegate::todoDelegate(QObject* parent): QItemDelegate(parent)
{
}
    
void todoDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
 if(index.column()==4)
  {
   int progress = index.model()->data(index, Qt::EditRole).toInt();   
   //lets draw our cool progress bar here
   QStyleOptionProgressBar opt;
   opt.rect = option.rect;
   opt.minimum = 0;
   opt.maximum = 100;
   opt.progress = progress;
   opt.text = QString("%1%").arg(progress);
   opt.textVisible = true;
   QApplication::style()->drawControl(QStyle::CE_ProgressBar, &opt, painter, 0);
  }
   else
     QItemDelegate::paint(painter, option, index);
}

QWidget *todoDelegate::createEditor(QWidget *parent,const QStyleOptionViewItem &/* option*/,const QModelIndex &index) const
{
	if((index.column()==2)||(index.column()==0))
	{
         QDateEdit *editor = new QDateEdit(QDate::currentDate(),parent);
         editor->setDisplayFormat("dd/M/yyyy");
         editor->setCalendarPopup(true);
         return editor;
        }
         else 
          if(index.column()==3)
           {
            QComboBox *editor = new QComboBox(parent);
            editor -> setEditable(true);
            for(int i=1;i<=10;i++) editor->addItem(QString::number(i));
            return editor;
           }
           else 
            if(index.column()==1)
             {
               QLineEdit *editor = new QLineEdit(parent);
               return editor;
             }
             if(index.column()==4)
              {
               QLineEdit *editor = new QLineEdit(parent);
               return editor;
              }
}

void todoDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,const QModelIndex &index) const
{
    if((index.column()==2)||(index.column()==0))
     {
       QDateEdit *dateEditor = qobject_cast<QDateEdit *>(editor);
        if (dateEditor) {
            model->setData(index, dateEditor->date().toString("dd/M/yyyy"));}
     }
     else
      if(index.column()==3)
      {
    	QComboBox *priorityBox = static_cast<QComboBox*>(editor);
    	if (priorityBox) 
            model->setData(index, priorityBox->currentText());
      }
      else
       if(index.column()==1)
        {
         QLineEdit *taskEdit = static_cast<QLineEdit*>(editor);
    	 if (taskEdit) 
            model->setData(index, taskEdit->text());
        }
        if(index.column()==4)
         { 
          QLineEdit *taskEdit = static_cast<QLineEdit*>(editor);
    	  if (taskEdit) 
            model->setData(index, taskEdit->text());
         }
}

void todoDelegate::updateEditorGeometry(QWidget *editor,const QStyleOptionViewItem &option,const QModelIndex &/* index */) const
{
    editor->setGeometry(option.rect);
}

//----------------------------------------------------Schedule Delegate -----------------------------------------------------------

scheduleDelegate::scheduleDelegate(QObject *parent) : QItemDelegate(parent)
{
}

QWidget *scheduleDelegate::createEditor(QWidget *parent,const QStyleOptionViewItem &/* option*/,const QModelIndex &index) const
{
        
	if(index.column()!=0)
        {
         QTimeEdit *editor = new QTimeEdit(QTime::currentTime().addSecs(60),parent);
         editor->setDisplayFormat("hh:mm");
         return editor;
        }
        else 
         {
          QLineEdit *editor = new QLineEdit(parent);
          return editor;
         }
}

void scheduleDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,const QModelIndex &index) const
{
	if(index.column()!=0)
         {
          QTimeEdit *timeEditor = qobject_cast< QTimeEdit *>(editor);
          if (timeEditor) 
            model->setData(index, timeEditor->time().toString("hh:mm"));
         }
         else 
          {
           QLineEdit *eventEdit = static_cast<QLineEdit*>(editor);
    	   if (eventEdit) 
            model->setData(index, eventEdit->text());
          }
}

void scheduleDelegate::updateEditorGeometry(QWidget *editor,const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
    editor->setGeometry(option.rect);
}


//---------------------------------------------------Time Table Delegate ------------------------------------------------------

timeTableDelegate::timeTableDelegate(QObject *parent) : QItemDelegate(parent)
{
}

QWidget *timeTableDelegate::createEditor(QWidget *parent,const QStyleOptionViewItem &/* option*/,const QModelIndex &index) const
{
        
	if((index.column()==0)||(index.column()==1))
        {
         QTimeEdit *editor = new QTimeEdit(QTime::currentTime().addSecs(60),parent);
         editor->setDisplayFormat("hh:mm");
         return editor;
        }
        else 
         {
          QLineEdit *editor = new QLineEdit(parent);
          return editor;
         }
}

void timeTableDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,const QModelIndex &index) const
{
	if((index.column()==0)||(index.column()==1))
         {
          QTimeEdit *timeEditor = qobject_cast< QTimeEdit *>(editor);
          if (timeEditor) 
            model->setData(index, timeEditor->time().toString("hh:mm"));
         }
         else 
          {
           QLineEdit *eventEdit = static_cast<QLineEdit*>(editor);
    	   if (eventEdit) 
            model->setData(index, eventEdit->text());
          }
}

void timeTableDelegate::updateEditorGeometry(QWidget *editor,const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
    editor->setGeometry(option.rect);
}


//-------------------------------------------MARK TABLE DELEGATE

//---------------------------------------------------Time Table Delegate ------------------------------------------------------

markDelegate::markDelegate(QObject *parent) : QItemDelegate(parent)
{
}

QWidget *markDelegate::createEditor(QWidget *parent,const QStyleOptionViewItem &/* option*/,const QModelIndex &index) const
{
        
	if(index.column() % 2!=0)
        {
         QDateEdit *editor = new QDateEdit(QDate::currentDate(),parent);
         editor->setDisplayFormat("d.M.yy");
         editor->setCalendarPopup(true);
         return editor;
        }
        else 
         {
          QLineEdit *editor = new QLineEdit(parent);
          return editor;
         }
}

void markDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,const QModelIndex &index) const
{
	if(index.column() % 2!=0)
         {
          QDateEdit *dateEditor = qobject_cast<QDateEdit *>(editor);
          if (dateEditor) {
            model->setData(index, dateEditor->date().toString("d.M.yy"));}
         }
         else 
          {
           QLineEdit *markEdit = static_cast<QLineEdit*>(editor);
    	   if (markEdit) 
            model->setData(index, markEdit->text());
          }
}

void markDelegate::updateEditorGeometry(QWidget *editor,const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
    editor->setGeometry(option.rect);
}


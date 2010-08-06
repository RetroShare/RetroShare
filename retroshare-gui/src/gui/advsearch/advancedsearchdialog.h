/****************************************************************
*  RetroShare is distributed under the following license:
*
*  Copyright (C) 2006, 2007 The RetroShare Team
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
#ifndef _AdvancedSearch_h_
#define _AdvancedSearch_h_

#include <QFontMetrics>
#include <QDialog>
#include <QList>
#include <QScrollArea>
#include <QSizePolicy>
#include "ui_AdvancedSearchDialog.h"
#include "expressionwidget.h"
#include <retroshare/rsexpr.h>

class AdvancedSearchDialog : public QDialog, public Ui::AdvancedSearchDialog 
{
    Q_OBJECT
        
public:
    AdvancedSearchDialog(QWidget * parent = 0 );
    Expression * getRsExpr();
    QString getSearchAsString();
signals:
    void search(Expression*);
    
private slots:
    void deleteExpression(ExpressionWidget*);
    void addNewExpression();
    void reset();
    void prepareSearch();
    
private:
    QLayout * dialogLayout;
    QVBoxLayout * expressionsLayout;
    QList<ExpressionWidget*> * expressions;
    QFontMetrics * metrics;
};

#endif // _AdvancedSearch_h_

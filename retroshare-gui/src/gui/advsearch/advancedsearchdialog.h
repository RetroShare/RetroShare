/*******************************************************************************
 * gui/advsearch/advancedsearchdialog.h                                        *
 *                                                                             *
 * LibResAPI: API for local socket server                                      *
 *                                                                             *
 * Copyright (C) RetroShare Team <retroshare.project@gmail.com>                *
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
    RsRegularExpression::Expression * getRsExpr();
    QString getSearchAsString();
signals:
    void search(RsRegularExpression::Expression*);
    
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

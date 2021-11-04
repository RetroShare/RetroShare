/*******************************************************************************
 * gui/advsearch/expressionwidget.h                                            *
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
#ifndef _ExpressionWidget_h_
#define _ExpressionWidget_h_

#include "ui_expressionwidget.h"

#include "guiexprelement.h"

#include <retroshare/rsexpr.h>

#include <QWidget>

#include <iostream>

/**
    Represents an Advanced Search GUI Expression object which acts as a container
    for a series of GuiExprElement objects. The structure of the expression can
    change dynamically, dependant on user choices made while assembling an expression. 
    The default appearance is simply a search on name.
*/
class ExpressionWidget : public QWidget, public Ui::ExpressionWidget 
{
    Q_OBJECT

public:
    ExpressionWidget( QWidget * parent = 0, bool initial=false );
    
    /** delivers the expression represented by this widget
     the operator to join this expression with any previous 
     expressions is provided by the getOperator method */
    RsRegularExpression::Expression* getRsExpression();

    /** supplies the operator to be used when joining this expression
        to the whole query */
    RsRegularExpression::LogicalOperator getOperator();
    
    QString toString();

signals:
    /** associates an expression object with the delete event */
    void signalDelete(ExpressionWidget*);

private slots:
    /** emits the signalDelete signal with a pointer to this object
        for use by listeners */
    void deleteExpression();

    /** dynbamically changes the structure of the expression based on
        the terms combobox changes */
    void adjustExprForTermType(int);
    
    /** dynamically adjusts the expression dependant on the choices 
        made in the condition combobox e.g. inRange and equals
        have different parameter fields */
    void adjustExprForConditionType(int);


private:
    bool isStringSearchExpression();

    bool isFirst;
    bool inRangedConfig;
    ExprSearchType searchType;
};

#endif // _ExpressionWidget_h_

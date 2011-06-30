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

#include "advancedsearchdialog.h"

AdvancedSearchDialog::AdvancedSearchDialog(QWidget * parent) : QDialog (parent) 
{
    setupUi(this);
    dialogLayout = this->layout();
    metrics = new QFontMetrics(this->font());

    // the list of expressions
    expressions = new QList<ExpressionWidget*>();

    // a area for holding the objects
    expressionsLayout = new QVBoxLayout();
    expressionsLayout->setSpacing(0);
    expressionsLayout->setMargin(0);
    expressionsLayout->setObjectName(QString::fromUtf8("expressionsLayout"));
    expressionsFrame->setSizePolicy(QSizePolicy::MinimumExpanding, 
                                  QSizePolicy::MinimumExpanding);
    expressionsFrame->setLayout(expressionsLayout);
    
    // we now add the first expression widgets to the dialog via a vertical
    // layout 
    reset();//addNewExpression();

    connect (this->addExprButton, SIGNAL(clicked()),
             this, SLOT(addNewExpression()));
    connect (this->resetButton, SIGNAL(clicked()),
             this, SLOT(reset()));
    connect(this->executeButton, SIGNAL(clicked()),
            this, SLOT(prepareSearch()));
}


void AdvancedSearchDialog::addNewExpression()
{
    int sizeChange = metrics->height() + 26;

    ExpressionWidget *expr;
    if (expressions->size() == 0)
    {
        //create an initial expression
        expr = new ExpressionWidget(expressionsFrame, true);
    } else {
        expr = new ExpressionWidget(expressionsFrame);
    }
    
    expressions->append(expr);
    expressionsLayout->addWidget(expr, 1, Qt::AlignLeft);
    
    
    connect(expr, SIGNAL(signalDelete(ExpressionWidget*)),
            this, SLOT(deleteExpression(ExpressionWidget*)));
    
    //expressionsLayout->invalidate();
    //searchCriteriaBox->setMinimumSize(searchCriteriaBox->minimumWidth(), 
     //                                 searchCriteriaBox->minimumHeight() + sizeChange);
    //searchCriteriaBox->adjustSize();
    expressionsFrame->adjustSize();
    this->setMinimumSize(this->minimumWidth(), this->minimumHeight()+sizeChange);
    this->adjustSize();

}

void AdvancedSearchDialog::deleteExpression(ExpressionWidget* expr)
{
    int sizeChange = metrics->height() + 26;
    
    expressions->removeAll(expr);
    expr->hide();
    expressionsLayout->removeWidget(expr);
    delete expr;
    
    expressionsLayout->invalidate();
    //searchCriteriaBox->setMinimumSize(searchCriteriaBox->minimumWidth(), 
      //                                searchCriteriaBox->minimumHeight() - sizeChange);
    //searchCriteriaBox->adjustSize();
    expressionsFrame->adjustSize();
    this->setMinimumSize(this->minimumWidth(), this->minimumHeight()-sizeChange);
    this->adjustSize();
}

void AdvancedSearchDialog::reset()
{
    ExpressionWidget *expr;
    while (!expressions->isEmpty())
    {
        expr = expressions->takeLast();
        deleteExpression(expr);
    }
    
    // now add a new default expressions
    addNewExpression();
}

void AdvancedSearchDialog::prepareSearch()
{
    emit search(getRsExpr());
}


Expression * AdvancedSearchDialog::getRsExpr()
{
    Expression * wholeExpression;

    // process the special case: first expression
    wholeExpression = expressions->at(0)->getRsExpression();
    

    // iterate through the items in elements and
    for (int i = 1; i < expressions->size(); ++i) {
        // extract the expression information and compound it with the
        // first expression
        wholeExpression = new CompoundExpression(expressions->at(i)->getOperator(),
                                                 wholeExpression,
                                                 expressions->at(i)->getRsExpression());
    } 
    return wholeExpression;
}

QString AdvancedSearchDialog::getSearchAsString()
{
    QString str = expressions->at(0)->toString();
    

    // iterate through the items in elements and
    for (int i = 1; i < expressions->size(); ++i) {
        // extract the expression information and compound it with the
        // first expression
        str += QString(" ") + expressions->at(i)->toString();
    } 
    return str;
}


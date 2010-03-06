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

#include "expressionwidget.h"

ExpressionWidget::ExpressionWidget(QWidget * parent, bool initial) : QWidget(parent)
{
    setupUi(this);
    
    inRangedConfig = false;
    
    // the default search type
    searchType = NameSearch;
    
    exprLayout = this->layout();
    
    exprOpFrame->setLayout          (createLayout());
    exprTermFrame->setLayout        (createLayout());
    exprConditionFrame->setLayout   (createLayout());
    exprParamFrame->setLayout       (createLayout());
    exprParamFrame->setSizePolicy(QSizePolicy::MinimumExpanding, 
                                  QSizePolicy::Fixed);
        
    elements = new QList<GuiExprElement*>();
    
    exprOpElem = new ExprOpElement();
    exprOpFrame->layout()->addWidget(exprOpElem);
    elements->append(exprOpElem);
        
    exprTermElem = new ExprTermsElement();
    exprTermFrame->layout()->addWidget(exprTermElem);
    elements->append(exprTermElem);
    connect (exprTermElem, SIGNAL(currentIndexChanged(int)),
             this, SLOT (adjustExprForTermType(int)));
    
    exprCondElem = new ExprConditionElement(searchType);
    exprConditionFrame->layout()->addWidget(exprCondElem);
    elements->append(exprCondElem);
    connect (exprCondElem, SIGNAL (currentIndexChanged(int)),
             this, SLOT (adjustExprForConditionType(int)));
             
    exprParamElem= new ExprParamElement(searchType);
    exprParamFrame->layout()->addWidget(exprParamElem);
    elements->append(exprParamElem);
    
    // set up the default search: a search on name
    adjustExprForTermType(searchType);
    isFirst = initial;
    deleteExprButton    ->setVisible(!isFirst);
    exprOpElem         ->setVisible(!isFirst);
    exprTermFrame       ->show();
    exprConditionFrame  ->show();
    exprParamFrame      ->show();
    
    // connect the delete button signal        
    connect (deleteExprButton, SIGNAL (clicked()),
                this, SLOT(deleteExpression()));
    
    this->show();
}

QLayout * ExpressionWidget::createLayout(QWidget * parent)
{
    QHBoxLayout * hboxLayout;
    if (parent == 0) 
    {
        hboxLayout = new QHBoxLayout();
    } else {
        hboxLayout = new QHBoxLayout(parent);
    }
    hboxLayout->setSpacing(0);
    hboxLayout->setMargin(0);
    return hboxLayout;
}

bool ExpressionWidget::isStringSearchExpression()
{
    return (searchType == NameSearch || searchType == PathSearch 
		|| searchType == ExtSearch || searchType == HashSearch);
}

void ExpressionWidget::adjustExprForTermType(int index)
{
    ExprSearchType type = (*GuiExprElement::TermsIndexMap)[index];
    searchType = type;
    
    // now adjust the relevant elements
    // the condition combobox
    exprCondElem->adjustForSearchType(type);
    
    // the parameter expression: can be a date, 1-2 edit fields
    //    or a size with units etc
    exprParamElem->adjustForSearchType(type);
    exprParamFrame->adjustSize();
    
    exprLayout->invalidate();
    this->adjustSize();
}

void ExpressionWidget::adjustExprForConditionType(int newCondition)
{
    // we adjust the appearance for a ranged selection
    inRangedConfig = (newCondition == GuiExprElement::RANGE_INDEX);
    exprParamElem->setRangedSearch(inRangedConfig);
    exprParamFrame->layout()->invalidate();
    exprParamFrame->adjustSize();
}

void ExpressionWidget::deleteExpression() 
{
   this->hide();
   emit signalDelete(this); 
}

LogicalOperator ExpressionWidget::getOperator()
{
    return exprOpElem->getLogicalOperator();
}

Expression* ExpressionWidget::getRsExpression()
{
    Expression * expr = NULL;
    
    std::list<std::string> wordList;
    int lowVal = 0;
    int highVal = 0;
    
    if (isStringSearchExpression()) 
    {
        QString txt = exprParamElem->getStrSearchValue();
        QStringList words = txt.split(" ", QString::SkipEmptyParts);
        for (int i = 0; i < words.size(); ++i)
            wordList.push_back(words.at(i).toStdString());
    } else if (inRangedConfig){
        // correct for reversed ranges to be nice to the user
        lowVal = exprParamElem->getIntLowValue();
        highVal = exprParamElem->getIntHighValue();
        if (lowVal >highVal)
        {   
            lowVal  = lowVal^highVal;
            highVal = lowVal^highVal;
            lowVal  = lowVal^highVal;
        }
    }
            
    switch (searchType)
    {
        case NameSearch:
            expr = new NameExpression(exprCondElem->getStringOperator(), 
                                      wordList,
                                      exprParamElem->ignoreCase());
            break;
        case PathSearch:
            expr = new PathExpression(exprCondElem->getStringOperator(), 
                                      wordList,
                                      exprParamElem->ignoreCase());
            break;
        case ExtSearch:
            expr = new ExtExpression(exprCondElem->getStringOperator(), 
                                      wordList, 
                                      exprParamElem->ignoreCase());
            break;
        case HashSearch:
            expr = new HashExpression(exprCondElem->getStringOperator(), 
                                      wordList);
            break;
        case DateSearch:
            if (inRangedConfig) {    
                expr = new DateExpression(exprCondElem->getRelOperator(), 
                                          lowVal,
                                          highVal);
            } else {
                expr = new DateExpression(exprCondElem->getRelOperator(), 
                                          exprParamElem->getIntValue());
            }
            break;
        case PopSearch:
            if (inRangedConfig) {    
                expr = new DateExpression(exprCondElem->getRelOperator(), 
                                          lowVal,
                                          highVal);
            } else {
                expr = new DateExpression(exprCondElem->getRelOperator(), 
                                          exprParamElem->getIntValue());
            }
            break;
        case SizeSearch:
            if (inRangedConfig) {    
                expr = new SizeExpression(exprCondElem->getRelOperator(),  
                                          lowVal,
                                          highVal);
            } else {
                expr = new SizeExpression(exprCondElem->getRelOperator(), 
                                          exprParamElem->getIntValue());
            }
            break;
    };
    return expr;
}

QString ExpressionWidget::toString()
{
    QString str = "";
    if (!isFirst)
    {
        str += exprOpElem->toString() + " "; 
    }
    str += exprTermElem->toString() + " ";
    str += exprCondElem->toString() + " ";
    str += exprParamElem->toString();
    return str;
}


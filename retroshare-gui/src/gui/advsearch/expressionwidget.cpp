/*******************************************************************************
 * gui/advsearch/expressionwidget.cpp                                          *
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
    ExprSearchType type = GuiExprElement::TermsIndexMap[index];
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

RsRegularExpression::LogicalOperator ExpressionWidget::getOperator()
{
    return exprOpElem->getLogicalOperator();
}

static int checkedConversion(uint64_t s)
{
	if(s > 0x7fffffff)
	{
		std::cerr << "Error: bad convertion from uint64_t s=" << s << " into int" << std::endl;
		return 0 ;
	}
	return (int)s ;
}

RsRegularExpression::Expression* ExpressionWidget::getRsExpression()
{
    RsRegularExpression::Expression * expr = NULL;
    
    std::list<std::string> wordList;
    uint64_t lowVal = 0;
    uint64_t highVal = 0;
    
    if (isStringSearchExpression()) 
    {
        QString txt = exprParamElem->getStrSearchValue();
        QStringList words = txt.split(" ", QString::SkipEmptyParts);
        for (int i = 0; i < words.size(); ++i)
            wordList.push_back(words.at(i).toUtf8().constData());
    } else if (inRangedConfig){
        // correct for reversed ranges to be nice to the user
        lowVal = exprParamElem->getIntLowValue();
        highVal = exprParamElem->getIntHighValue();
        if (lowVal >highVal)
        {   
            lowVal  = lowVal^highVal;	// csoler: wow, that is some style!
            highVal = lowVal^highVal;
            lowVal  = lowVal^highVal;
        }
    }
            
    switch (searchType)
    {
        case NameSearch:
            expr = new RsRegularExpression::NameExpression(exprCondElem->getStringOperator(),
                                      wordList,
                                      exprParamElem->ignoreCase());
            break;
        case PathSearch:
            expr = new RsRegularExpression::PathExpression(exprCondElem->getStringOperator(),
                                      wordList,
                                      exprParamElem->ignoreCase());
            break;
        case ExtSearch:
            expr = new RsRegularExpression::ExtExpression(exprCondElem->getStringOperator(),
                                      wordList, 
                                      exprParamElem->ignoreCase());
            break;
        case HashSearch:
            expr = new RsRegularExpression::HashExpression(exprCondElem->getStringOperator(),
                                      wordList);
            break;
        case DateSearch:
            if (inRangedConfig) {    
                expr = new RsRegularExpression::DateExpression(exprCondElem->getRelOperator(), checkedConversion(lowVal), checkedConversion(highVal));
            } else {
                expr = new RsRegularExpression::DateExpression(exprCondElem->getRelOperator(), checkedConversion(exprParamElem->getIntValue()));
            }
            break;
        case PopSearch:
            if (inRangedConfig) {    
                expr = new RsRegularExpression::DateExpression(exprCondElem->getRelOperator(), checkedConversion(lowVal), checkedConversion(highVal));
            } else {
                expr = new RsRegularExpression::DateExpression(exprCondElem->getRelOperator(), checkedConversion(exprParamElem->getIntValue()));
            }
            break;
        case SizeSearch:
            if (inRangedConfig) 
				{    
						if(lowVal >= (uint64_t)(1024*1024*1024) || highVal >= (uint64_t)(1024*1024*1024)) 
                                 expr = new RsRegularExpression::SizeExpressionMB(exprCondElem->getRelOperator(), (int)(lowVal / (1024*1024)), (int)(highVal / (1024*1024)));
						else
                            expr = new RsRegularExpression::SizeExpression(exprCondElem->getRelOperator(),  lowVal, highVal);
            } 
				else 
				{
					uint64_t s = exprParamElem->getIntValue() ;

					if(s >= (uint64_t)(1024*1024*1024))
                        expr = new RsRegularExpression::SizeExpressionMB(exprCondElem->getRelOperator(), (int)(s/(1024*1024))) ;
					else
                        expr = new RsRegularExpression::SizeExpression(exprCondElem->getRelOperator(), (int)s) ;
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


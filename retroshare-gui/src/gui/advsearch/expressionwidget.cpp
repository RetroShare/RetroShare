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
#include "util/QtVersion.h"

ExpressionWidget::ExpressionWidget(QWidget * parent, bool initial)
    : QWidget(parent)
    , isFirst (initial), inRangedConfig(false)
    , searchType (NameSearch) // the default search type
{
	setupUi(this);

	connect (exprTermElem, SIGNAL(currentIndexChanged(int)),
	         this, SLOT (adjustExprForTermType(int)));

	connect (exprCondElem, SIGNAL (currentIndexChanged(int)),
	         this, SLOT (adjustExprForConditionType(int)));

	// set up the default search: a search on name
	adjustExprForTermType(searchType);
	exprOpElem         ->setVisible(!isFirst);
	deleteExprButton   ->setVisible(!isFirst);

	// connect the delete button signal
	connect (deleteExprButton, SIGNAL (clicked()),
	         this, SLOT(deleteExpression()) );

	this->show();
}

bool ExpressionWidget::isStringSearchExpression()
{
	return (   searchType == NameSearch || searchType == PathSearch
	        || searchType == ExtSearch  || searchType == HashSearch);
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

	this->setMinimumWidth( exprOpElem->width()+exprTermElem->width()
	                     + exprCondElem->width()+exprParamElem->width()
	                     + deleteExprButton->width() );
}

void ExpressionWidget::adjustExprForConditionType(int newCondition)
{
	// we adjust the appearance for a ranged selection
	inRangedConfig = (newCondition == GuiExprElement::RANGE_INDEX);
	exprParamElem->setRangedSearch(inRangedConfig);
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
        QStringList words = txt.split(" ", QtSkipEmptyParts);
        for (int i = 0; i < words.size(); ++i)
            wordList.push_back(words.at(i).toUtf8().constData());
    } else if (inRangedConfig){
        // correct for reversed ranges to be nice to the user
        lowVal = exprParamElem->getIntLowValue();
        highVal = exprParamElem->getIntHighValue();
        if (lowVal >highVal)
        {   
            auto tmp=lowVal;
            lowVal = highVal;
            highVal = tmp;
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
        switch(exprCondElem->getRelOperator())	// we need to convert expressions so that the delta is 1 day (i.e. 86400 secs)
        {
             // The conditions below account for 3 things:
             // - the swap between variables in rsexpr.h:214
             // - the swap of variables when calling getRelOperator() (See guiexprelement.cpp:166)
             // - the fact that some comparisions in the unit of days may add 86400 seconds.

            default:
            case RsRegularExpression::Equals:
                expr = new RsRegularExpression::DateExpression(RsRegularExpression::InRange, checkedConversion(exprParamElem->getIntValue()), checkedConversion(86400+exprParamElem->getIntValue()));
                break;
            case RsRegularExpression::InRange:
                expr = new RsRegularExpression::DateExpression(RsRegularExpression::InRange, checkedConversion(lowVal), 86400+checkedConversion(highVal));
                break;
            case RsRegularExpression::Greater:			// means we expect file.date() <   some day D. So file.date() < D, meaning Exp=Greater
                expr = new RsRegularExpression::DateExpression(RsRegularExpression::Greater,checkedConversion(exprParamElem->getIntValue()));
                break;
            case RsRegularExpression::SmallerEquals:	// means we expect file.date() >=  some day D. So file.date() >= D, meaning Exp=SmallerEquals
                expr = new RsRegularExpression::DateExpression(RsRegularExpression::SmallerEquals,checkedConversion(exprParamElem->getIntValue()));
                break;
            case RsRegularExpression::Smaller:			// means we expect file.date() >  some day D. So file.date() >= D+86400, meaning Exp=SmallerEquals
                expr = new RsRegularExpression::DateExpression(RsRegularExpression::SmallerEquals, checkedConversion(86400+exprParamElem->getIntValue()-1));
                break;
            case RsRegularExpression::GreaterEquals:	// means we expect file.date() <= some day D. So file.date() < D+86400, meaning Exp=Greater
                expr = new RsRegularExpression::DateExpression(RsRegularExpression::Greater, checkedConversion(86400+exprParamElem->getIntValue()));
                break;
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
                auto cond = exprCondElem->getRelOperator();
                bool MB = false;

                if(s >= (uint64_t)(1024*1024*1024))
                {
                    MB=true;
                    s >>= 20;
                }

                // Specific case for Equal operator, which we convert to a range, so as to avoid matching arbitrary digits

                if(cond == RsRegularExpression::Equals)
                {
                    // Now compute a proper interval. There is no optimal solution, so we aim for the simplest: add/remove 20% of the initial value.

                    if(MB)
                        expr = new RsRegularExpression::SizeExpressionMB(RsRegularExpression::InRange, (int)(s*0.8) , (int)(s*1.2));
                    else
                        expr = new RsRegularExpression::SizeExpression(RsRegularExpression::InRange, (int)(s*0.8) , (int)(s*1.2));
                }
                else
                {
                    if(MB)
                        expr = new RsRegularExpression::SizeExpressionMB(cond, (int)s);
                    else
                        expr = new RsRegularExpression::SizeExpression(cond, (int)s) ;
                }
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


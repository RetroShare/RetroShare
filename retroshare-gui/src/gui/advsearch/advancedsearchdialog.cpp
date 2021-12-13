/*******************************************************************************
 * gui/advsearch/advancedsearchdialog.cpp                                      *
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

#include "advancedsearchdialog.h"

#include "gui/common/FilesDefs.h"

#include <QGridLayout>

AdvancedSearchDialog::AdvancedSearchDialog(QWidget * parent) : QDialog (parent) 
{
	setupUi(this);

	QFontMetrics metrics = QFontMetrics(this->font());
	searchCriteriaBox_VL->setContentsMargins(2, metrics.height()/2, 2, 2);
	addExprButton->setIconSize(QSize(metrics.height(),metrics.height())*1.5);
	resetButton->setIconSize(QSize(metrics.height(),metrics.height())*1.5);

	// Save current default size as minimum to only add expresssions size to it.
	this->adjustSize();
	this->setMinimumSize(this->size());

	// the list of expressions
	expressions = new QList<ExpressionWidget*>();

	// we now add the first expression widgets to the dialog
	reset();//addNewExpression();

	connect (  addExprButton, SIGNAL(clicked())
	         , this,          SLOT(addNewExpression()));
	connect (  resetButton,   SIGNAL(clicked())
	         , this,          SLOT(reset()));
	connect (  searchButton,  SIGNAL(clicked())
	         , this,          SLOT(prepareSearch()));
}


void AdvancedSearchDialog::addNewExpression()
{
	ExpressionWidget *expr = new ExpressionWidget(searchCriteriaBox, (expressions->size() == 0));
	expressions->append(expr);

	searchCriteriaBox_VL->addWidget(expr);
	searchCriteriaBox_VL->setAlignment(Qt::AlignTop);
	expr->adjustSize();
	if (searchCriteriaBox->minimumWidth() < expr->minimumWidth())
		searchCriteriaBox->setMinimumWidth(expr->minimumWidth());

	QSize exprHeight = QSize(0,expr->height());

	connect(  expr, SIGNAL(signalDelete(ExpressionWidget*))
	        , this, SLOT(deleteExpression(ExpressionWidget*)) );

	this->setMinimumSize(this->minimumSize() + exprHeight);
	int marg = gradFrame_GL->contentsMargins().left()+gradFrame_GL->contentsMargins().right();
	marg += this->contentsMargins().left()+this->contentsMargins().right();
	if (this->minimumWidth() < (searchCriteriaBox->minimumWidth()+marg))
		this->setMinimumWidth(searchCriteriaBox->minimumWidth()+marg);
}

void AdvancedSearchDialog::deleteExpression(ExpressionWidget* expr)
{
	QSize exprHeight = QSize(0,expr->height());

	expressions->removeAll(expr);
	expr->hide();
	searchCriteriaBox_VL->removeWidget(expr);
	delete expr;

	this->setMinimumSize(this->minimumSize() - exprHeight);
	this->resize(this->size() - exprHeight);
}

void AdvancedSearchDialog::reset()
{
    while (!expressions->isEmpty())
        deleteExpression(expressions->takeLast());
    
    // now add a new default expressions
    addNewExpression();
}

void AdvancedSearchDialog::prepareSearch()
{
    emit search(getRsExpr());
}


RsRegularExpression::Expression * AdvancedSearchDialog::getRsExpr()
{
    RsRegularExpression::Expression * wholeExpression;

    // process the special case: first expression
    wholeExpression = expressions->at(0)->getRsExpression();

    // iterate through the items in elements and
#warning Phenom (2017-07-21): I don t know if it is a real memLeak for wholeExpression. If not remove this warning and add a comment how it is deleted.
    // cppcheck-suppress memleak
    for (int i = 1; i < expressions->size(); ++i) {
        // extract the expression information and compound it with the
        // first expression
        wholeExpression = new RsRegularExpression::CompoundExpression(expressions->at(i)->getOperator(),
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


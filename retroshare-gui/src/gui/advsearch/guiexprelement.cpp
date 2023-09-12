/*******************************************************************************
 * gui/advsearch/guiexprelement.cpp                                            *
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

#include "guiexprelement.h"
#define STR_FIELDS_MIN_WFACTOR  20.0
#define SIZE_FIELDS_MIN_WFACTOR  8.0
#define DATE_FIELDS_MIN_WFACTOR 10.0
#define FIELDS_MIN_HFACTOR       1.2

const int GuiExprElement::AND_INDEX         = 0;
const int GuiExprElement::OR_INDEX          = 1;
const int GuiExprElement::XOR_INDEX         = 2;

const int GuiExprElement::NAME_INDEX        = 0;
const int GuiExprElement::PATH_INDEX        = 1;
const int GuiExprElement::EXT_INDEX         = 2;
const int GuiExprElement::HASH_INDEX        = 3;
//const int GuiExprElement::KEYWORDS_INDEX    = ???;
//const int GuiExprElement::COMMENTS_INDEX    = ???;
//const int GuiExprElement::META_INDEX        = ???;
const int GuiExprElement::DATE_INDEX        = 4;
const int GuiExprElement::SIZE_INDEX        = 5;
const int GuiExprElement::POP_INDEX         = 6;

const int GuiExprElement::CONTAINS_INDEX    = 0;
const int GuiExprElement::CONTALL_INDEX     = 1;
const int GuiExprElement::IS_INDEX          = 2;

const int GuiExprElement::LT_INDEX          = 0;
const int GuiExprElement::LTE_INDEX         = 1;
const int GuiExprElement::EQUAL_INDEX       = 2;
const int GuiExprElement::GTE_INDEX         = 3;
const int GuiExprElement::GT_INDEX          = 4;
const int GuiExprElement::RANGE_INDEX       = 5;

QStringList GuiExprElement::exprOpsList;
QStringList GuiExprElement::searchTermsOptionsList;
QStringList GuiExprElement::stringOptionsList;
QStringList GuiExprElement::relOptionsList;

QMap<int, ExprSearchType> GuiExprElement::TermsIndexMap;

QMap<int, RsRegularExpression::LogicalOperator> GuiExprElement::logicalOpIndexMap;
QMap<int, RsRegularExpression::StringOperator> GuiExprElement::strConditionIndexMap;
QMap<int, RsRegularExpression::RelOperator> GuiExprElement::relConditionIndexMap;

QMap<int, QString> GuiExprElement::logicalOpStrMap;
QMap<int, QString> GuiExprElement::termsStrMap;
QMap<int, QString> GuiExprElement::strConditionStrMap;
QMap<int, QString> GuiExprElement::relConditionStrMap;

bool GuiExprElement::initialised = false;


GuiExprElement::GuiExprElement(QWidget * parent)
    : QFrame(parent), searchType(NameSearch)
{
	if (!GuiExprElement::initialised)
		initialiseOptionsLists();

	setObjectName("trans_InternalFrame");
	createLayout(this);
}


void GuiExprElement::initialiseOptionsLists()
{
    const QString AND     = tr("and");
    const QString OR      = tr("and / or");
    const QString XOR     = tr("or"); // exclusive or

    const QString NAME    = tr("Name");
    const QString PATH    = tr("Path");
    const QString EXT     = tr("Extension");
    const QString HASH    = tr("Hash");
    //const QString KEYWORDS= tr("Keywords");
    //const QString COMMENTS= tr("Comments");
    //const QString META    = tr("Meta");
    const QString DATE    = tr("Date");
    const QString SIZE    = tr("Size");
    const QString POP     = tr("Popularity");

    const QString CONTAINS= tr("contains");
    const QString CONTALL = tr("contains all");
    const QString IS      = tr("is");

    const QString LT      = tr("less than");
    const QString LTE     = tr("less than or equal");
    const QString EQUAL   = tr("equals");
    const QString GTE     = tr("greater than or equal");
    const QString GT      = tr("greater than");
    const QString RANGE   = tr("is in range");

    exprOpsList.append(AND);
    exprOpsList.append(OR);
    exprOpsList.append(XOR);

    GuiExprElement::searchTermsOptionsList.append(NAME);
    GuiExprElement::searchTermsOptionsList.append(PATH);
    GuiExprElement::searchTermsOptionsList.append(EXT);
    GuiExprElement::searchTermsOptionsList.append(HASH);
    //GuiExprElement::searchTermsOptionsList.append(KEYWORDS);
    //GuiExprElement::searchTermsOptionsList.append(COMMENTS);
    //GuiExprElement::searchTermsOptionsList.append(META);
    GuiExprElement::searchTermsOptionsList.append(DATE);
    GuiExprElement::searchTermsOptionsList.append(SIZE);
    //GuiExprElement::searchTermsOptionsList.append(POP);
    
    GuiExprElement::stringOptionsList.append(CONTAINS);
    GuiExprElement::stringOptionsList.append(CONTALL);
    GuiExprElement::stringOptionsList.append(IS);
    
    GuiExprElement::relOptionsList.append(LT);
    GuiExprElement::relOptionsList.append(LTE);
    GuiExprElement::relOptionsList.append(EQUAL);
    GuiExprElement::relOptionsList.append(GTE);
    GuiExprElement::relOptionsList.append(GT);
    GuiExprElement::relOptionsList.append(RANGE);

    // now the maps
    GuiExprElement::logicalOpIndexMap[GuiExprElement::AND_INDEX] = RsRegularExpression::AndOp;
    GuiExprElement::logicalOpIndexMap[GuiExprElement::OR_INDEX] = RsRegularExpression::OrOp;
    GuiExprElement::logicalOpIndexMap[GuiExprElement::XOR_INDEX] = RsRegularExpression::XorOp;
    
    GuiExprElement::TermsIndexMap[GuiExprElement::NAME_INDEX] = NameSearch;
    GuiExprElement::TermsIndexMap[GuiExprElement::PATH_INDEX] = PathSearch;
    GuiExprElement::TermsIndexMap[GuiExprElement::EXT_INDEX] = ExtSearch;
    GuiExprElement::TermsIndexMap[GuiExprElement::HASH_INDEX] = HashSearch;
    GuiExprElement::TermsIndexMap[GuiExprElement::DATE_INDEX] = DateSearch;
    GuiExprElement::TermsIndexMap[GuiExprElement::SIZE_INDEX] = SizeSearch;
    GuiExprElement::TermsIndexMap[GuiExprElement::POP_INDEX] = PopSearch;
    
    GuiExprElement::strConditionIndexMap[GuiExprElement::CONTAINS_INDEX] = RsRegularExpression::ContainsAnyStrings;
    GuiExprElement::strConditionIndexMap[GuiExprElement::CONTALL_INDEX] = RsRegularExpression::ContainsAllStrings;
    GuiExprElement::strConditionIndexMap[GuiExprElement::IS_INDEX] = RsRegularExpression::EqualsString;
    
/*  W A R N I N G !!!!
    the cb elements correspond to their inverse rel op counterparts in rsexpr.h due to the nature of 
    the implementation.
    For example consider "size greater than 100kb" selected in the GUI.
    The rsexpr.cc impl returns true if the CONDITION specified is greater than the file size passed as argument 
    as rsexpr iterates through the files. So, the user wants files that are greater than 100kb but the impl returns
    files where the condition is greater than the file size i.e. files whose size is less than to the condition
    Therefore we invert the mapping of rel conditions here to match the behaviour of the impl.
    Also the Equal nature of comparisons should be kept, since = is symmetric.
*/    
    GuiExprElement::relConditionIndexMap[GuiExprElement::LT_INDEX] = RsRegularExpression::Greater;
    GuiExprElement::relConditionIndexMap[GuiExprElement::LTE_INDEX] = RsRegularExpression::GreaterEquals;
    GuiExprElement::relConditionIndexMap[GuiExprElement::EQUAL_INDEX] = RsRegularExpression::Equals;
    GuiExprElement::relConditionIndexMap[GuiExprElement::GTE_INDEX] = RsRegularExpression::SmallerEquals;
    GuiExprElement::relConditionIndexMap[GuiExprElement::GT_INDEX] = RsRegularExpression::Smaller;
    GuiExprElement::relConditionIndexMap[GuiExprElement::RANGE_INDEX] = RsRegularExpression::InRange;

    // the string to index map
    GuiExprElement::termsStrMap[GuiExprElement::NAME_INDEX] = NAME;
    GuiExprElement::termsStrMap[GuiExprElement::PATH_INDEX] = PATH;
    GuiExprElement::termsStrMap[GuiExprElement::EXT_INDEX]  = EXT;
    GuiExprElement::termsStrMap[GuiExprElement::HASH_INDEX]  = HASH;
    GuiExprElement::termsStrMap[GuiExprElement::DATE_INDEX] = DATE;
    GuiExprElement::termsStrMap[GuiExprElement::SIZE_INDEX] = SIZE;
    GuiExprElement::termsStrMap[GuiExprElement::POP_INDEX]  = POP;

    GuiExprElement::logicalOpStrMap[GuiExprElement::AND_INDEX] = AND;
    GuiExprElement::logicalOpStrMap[GuiExprElement::XOR_INDEX] = XOR;
    GuiExprElement::logicalOpStrMap[GuiExprElement::OR_INDEX]  = OR;
    
    GuiExprElement::strConditionStrMap[GuiExprElement::CONTAINS_INDEX] = CONTAINS;
    GuiExprElement::strConditionStrMap[GuiExprElement::CONTALL_INDEX] = CONTALL;
    GuiExprElement::strConditionStrMap[GuiExprElement::IS_INDEX] = IS;
    
    GuiExprElement::relConditionStrMap[GuiExprElement::LT_INDEX] = LT;
    GuiExprElement::relConditionStrMap[GuiExprElement::LTE_INDEX] = LTE;
    GuiExprElement::relConditionStrMap[GuiExprElement::EQUAL_INDEX] = EQUAL;
    GuiExprElement::relConditionStrMap[GuiExprElement::GTE_INDEX] = GTE;
    GuiExprElement::relConditionStrMap[GuiExprElement::GT_INDEX] = GT;
    GuiExprElement::relConditionStrMap[GuiExprElement::RANGE_INDEX] = RANGE;


    GuiExprElement::initialised = true;
}

QStringList GuiExprElement::getConditionOptions(ExprSearchType t)
{
    switch (t) {
        case NameSearch:
        case PathSearch:
        case ExtSearch:
        case HashSearch:
            return GuiExprElement::stringOptionsList;
            break;
        case DateSearch:
        case PopSearch:
        case SizeSearch:
        default:
            return GuiExprElement::relOptionsList;
    }
    return QStringList();
}

QHBoxLayout * GuiExprElement::createLayout(QWidget * parent /*= nullptr*/)
{
    QHBoxLayout * hboxLayout = new QHBoxLayout(parent);
    hboxLayout->setMargin(0);
    hboxLayout->setSpacing(0);
    return hboxLayout;
}

QSize GuiExprElement::getMinSize(float widthFactor/*=1*/, float heightFactor/*=1*/)
{
	QFontMetrics fm = QFontMetrics(font());
#if QT_VERSION < QT_VERSION_CHECK(5,11,0)
	QSize size = QSize(fm.width("_")*widthFactor, fm.height()*heightFactor);
#else
	QSize size = QSize(fm.horizontalAdvance("_")*widthFactor, fm.height()*heightFactor);
#endif
	return size;
}

bool GuiExprElement::isStringSearchExpression()
{
	return (searchType == NameSearch || searchType == PathSearch
	        || searchType == ExtSearch || searchType == HashSearch);
}


/* ********************************************************************/
/* *********** L O G I C A L   O P    E L E M E N T  ******************/
/* ********************************************************************/
ExprOpElement::ExprOpElement(QWidget * parent)
    : GuiExprElement(parent)
{
	cb = new RSComboBox(this);
	cb->addItems(GuiExprElement::exprOpsList);
	layout()->addWidget(cb);
	setMinimumSize(cb->sizeHint());
}

QString ExprOpElement::toString()
{
    return GuiExprElement::logicalOpStrMap[cb->currentIndex()];
}


RsRegularExpression::LogicalOperator ExprOpElement::getLogicalOperator()
{
    return GuiExprElement::logicalOpIndexMap[cb->currentIndex()];
}


/* **********************************************************/
/* *********** T E R M S    E L E M E N T  ******************/
/* **********************************************************/
ExprTermsElement::ExprTermsElement(QWidget * parent)
    : GuiExprElement(parent)
{
	cb = new RSComboBox(this);
	connect (cb, SIGNAL(currentIndexChanged(int)),
	         this, SIGNAL(currentIndexChanged(int)));
	cb->addItems(GuiExprElement::searchTermsOptionsList);
	layout()->addWidget(cb);
	setMinimumSize(cb->sizeHint());
}
QString ExprTermsElement::toString()
{
    return GuiExprElement::termsStrMap[cb->currentIndex()];
}

/* ******************************************************************/
/* *********** C O N D I T I O N    E L E M E N T  ******************/
/* ******************************************************************/
ExprConditionElement::ExprConditionElement(QWidget * parent, ExprSearchType type)
    : GuiExprElement(parent)
{
	cb = new RSComboBox(this);
	connect (cb, SIGNAL(currentIndexChanged(int)),
	         this, SIGNAL(currentIndexChanged(int)));
	cb->addItems(getConditionOptions(type));
	layout()->addWidget(cb);
	setMinimumSize(cb->sizeHint());
}


QString ExprConditionElement::toString()
{
    if (isStringSearchExpression())
    {
        return GuiExprElement::strConditionStrMap[cb->currentIndex()];
    }

    return GuiExprElement::relConditionStrMap[cb->currentIndex()];
}

RsRegularExpression::RelOperator ExprConditionElement::getRelOperator()
{
    return GuiExprElement::relConditionIndexMap[cb->currentIndex()];
}

RsRegularExpression::StringOperator ExprConditionElement::getStringOperator()
{
    return GuiExprElement::strConditionIndexMap[cb->currentIndex()];
}

void ExprConditionElement::adjustForSearchType(ExprSearchType type)
{
    cb->clear();
    cb->addItems(getConditionOptions(type));
    searchType = type;
}

/* **********************************************************/
/* *********** P A R A M    E L E M E N T  ******************/
/* **********************************************************/
ExprParamElement::ExprParamElement(QWidget * parent, ExprSearchType type)
    : GuiExprElement(parent), inRangedConfig(false)
{
	adjustForSearchType(type);
}

QString ExprParamElement::toString()
{
	QString str = "";
	if (isStringSearchExpression())
	{
		str = QString("\"") + getStrSearchValue() + QString("\"");
		// we don't bother with case if hash search
		if (searchType != HashSearch) {
			str += (ignoreCase() ? QString(" (ignore case)")
			                     : QString(" (case sensitive)"));
		}
	} else
	{
		if (searchType ==  DateSearch) {
			QDateEdit * dateEdit =  findChild<QDateEdit *> ("param1");
			str = dateEdit->text();
			if (inRangedConfig)
			{
				str += QString(" ") + tr("to") + QString(" ");
				dateEdit = findChild<QDateEdit *> ("param2");
				str += dateEdit->text();
			}
		} else if (searchType == SizeSearch)
		{
			QLineEdit * lineEditSize =  findChild<QLineEdit*>("param1");
			str = ("" == lineEditSize->text()) ? "0"
			                                   : lineEditSize->text();
			RSComboBox * cb = findChild<RSComboBox*> ("unitsCb1");
			str += QString(" ") + cb->itemText(cb->currentIndex());
			if (inRangedConfig)
			{
				str += QString(" ") + tr("to") + QString(" ");
				lineEditSize =  findChild<QLineEdit*>("param2");
				str += ("" == lineEditSize->text()) ? "0"
				                                    : lineEditSize->text();
				cb = findChild<RSComboBox*> ("unitsCb2");
				str += QString(" ") + cb->itemText(cb->currentIndex());
			}
		}
	}
	return str;
}

void clearLayout(QLayout *layout) {
	if (layout == NULL)
		return;
	QLayoutItem *item;
	while((item = layout->takeAt(0))) {
		if (item->layout()) {
			clearLayout(item->layout());
			delete item->layout();
		}
		if (item->widget()) {
			delete item->widget();
		}
		delete item;
	}
}

void ExprParamElement::adjustForSearchType(ExprSearchType type)
{
	// record which search type is active
	searchType = type;
	QRegExp regExp("0|[1-9][0-9]*");
	numValidator = new QRegExpValidator(regExp, this);
	QRegExp hexRegExp("[A-Fa-f0-9]*");
	hexValidator = new QRegExpValidator(hexRegExp, this);

	QHBoxLayout* hbox = static_cast<QHBoxLayout*>(layout());
	clearLayout(hbox);

	setMinimumSize(getMinSize(STR_FIELDS_MIN_WFACTOR,FIELDS_MIN_HFACTOR) );
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

	if (isStringSearchExpression())
	{
		// set up for default of a simple input field
		QLineEdit* lineEdit = new QLineEdit(this);
		lineEdit->setMinimumSize(getMinSize(STR_FIELDS_MIN_WFACTOR,FIELDS_MIN_HFACTOR));
		lineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
		lineEdit->setObjectName("param1");
		hbox->addWidget(lineEdit);

		hbox->addSpacing(9);

		QCheckBox* icCb = new QCheckBox(tr("ignore case"), this);
		icCb->setMinimumSize(getMinSize(icCb->text().length()+2,FIELDS_MIN_HFACTOR));
		icCb->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
		icCb->setObjectName("ignoreCaseCB");
		icCb->setCheckState(Qt::Checked);
		hbox->addWidget(icCb);

		// hex search specifics: hidden case sensitivity and hex validator
		if (searchType == HashSearch) {
			icCb->hide();
			lineEdit->setValidator(hexValidator);
		}
		setMinimumSize(lineEdit->minimumSize()
		               + QSize((searchType != HashSearch ? icCb->minimumWidth() : 0),0) );

	} else if (searchType == DateSearch)
	{
		QDateEdit * dateEdit = new QDateEdit(QDate::currentDate(), this);
		dateEdit->setMinimumSize(getMinSize(DATE_FIELDS_MIN_WFACTOR, FIELDS_MIN_HFACTOR));
		dateEdit->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
		dateEdit->setDisplayFormat(tr("yyyy-MM-dd"));
		dateEdit->setObjectName("param1");
		dateEdit->setMinimumDate(QDate(1970, 1, 1));
		dateEdit->setMaximumDate(QDate(2099, 12,31));
		hbox->addWidget(dateEdit, Qt::AlignLeft);
		hbox->addStretch();
		setMinimumSize(dateEdit->minimumSize());

	} else if (searchType == SizeSearch)
	{
		QLineEdit * lineEdit = new QLineEdit(this);
		lineEdit->setMinimumSize(getMinSize(SIZE_FIELDS_MIN_WFACTOR, FIELDS_MIN_HFACTOR));
		lineEdit->setMaximumSize(getMinSize(SIZE_FIELDS_MIN_WFACTOR, FIELDS_MIN_HFACTOR));
		lineEdit->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
		lineEdit->setObjectName("param1");
		lineEdit->setValidator(numValidator);
		hbox->addWidget(lineEdit, Qt::AlignLeft);

		hbox->addSpacing(9);

		RSComboBox * cb = new RSComboBox(this);
		cb->setObjectName("unitsCb1");
		cb->addItem(tr("KB"), QVariant(1024));
		cb->addItem(tr("MB"), QVariant(1024*1024));
		cb->addItem(tr("GB"), QVariant(1024*1024*1024));
		hbox->addWidget(cb);
		hbox->addStretch();
		setMinimumSize(lineEdit->minimumSize() + QSize(9,0)
		               + QSize(cb->minimumWidth(),0) );

	}

	/* POP Search not implemented
	else if (searchType == PopSearch)
	{
		QLineEdit * lineEdit = new QLineEdit(this);
		lineEdit->setObjectName("param1");
		lineEdit->setValidator(numValidator);
		hbox->addWidget(lineEdit);
		hbox->addStretch();
		setMinimumSize(lineEdit->minimumSize());
	}*/
	hbox->invalidate();
	adjustSize();
	show();
}

void ExprParamElement::setRangedSearch(bool ranged)
{
    
    if (inRangedConfig == ranged) return; // nothing to do here
    inRangedConfig = ranged;
    QHBoxLayout* hbox = static_cast<QHBoxLayout*>(layout());

    // add additional or remove extra input fields depending on whether
    // ranged search or not
    if (inRangedConfig)
    {

        if(hbox->itemAt(hbox->count()-1)->spacerItem())
            delete hbox->takeAt(hbox->count()-1);

        QLabel * toLbl = new QLabel(tr("to"));
        toLbl->setMinimumSize(getMinSize(2, FIELDS_MIN_HFACTOR));
        toLbl->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

        if (searchType ==  DateSearch) {
            QDateEdit * dateEdit = new QDateEdit(QDate::currentDate(), this);
            dateEdit->setMinimumSize(getMinSize(DATE_FIELDS_MIN_WFACTOR, FIELDS_MIN_HFACTOR));
            dateEdit->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            dateEdit->setDisplayFormat(tr("yyyy-MM-dd"));
            dateEdit->setObjectName("param2");
            dateEdit->setMinimumDate(QDate(1970, 1, 1));
            dateEdit->setMaximumDate(QDate(2099, 12,31));
            
            hbox->addSpacing(9);
            hbox->addWidget(toLbl, Qt::AlignLeft);
            hbox->addSpacing(9);
            hbox->addWidget(dateEdit, Qt::AlignLeft);
            hbox->addStretch();
            setMinimumSize(minimumSize() + QSize(9,0)
                           + QSize(toLbl->minimumWidth(),0) + QSize(9,0)
                           + QSize(dateEdit->minimumWidth(),0) );

        } else if (searchType == SizeSearch) {
            QLineEdit * lineEdit = new QLineEdit(this);
            lineEdit->setMinimumSize(getMinSize(SIZE_FIELDS_MIN_WFACTOR, FIELDS_MIN_HFACTOR));
            lineEdit->setMaximumSize(getMinSize(SIZE_FIELDS_MIN_WFACTOR, FIELDS_MIN_HFACTOR));
            lineEdit->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            lineEdit->setObjectName("param2");
            lineEdit->setValidator(numValidator);
            
            RSComboBox * cb = new RSComboBox(this);
            cb->setObjectName("unitsCb2");
            cb->addItem(tr("KB"), QVariant(1024));
            cb->addItem(tr("MB"), QVariant(1048576));
            cb->addItem(tr("GB"), QVariant(1073741824));
            
            hbox->addSpacing(9);
            hbox->addWidget(toLbl, Qt::AlignLeft);
            hbox->addSpacing(9);
            hbox->addWidget(lineEdit, Qt::AlignLeft);
            hbox->addSpacing(9);
            hbox->addWidget(cb);
            hbox->addStretch();
            setMinimumSize(minimumSize() + QSize(9,0)
                           + QSize(toLbl->minimumWidth(),0) + QSize(9,0)
                           + QSize(lineEdit->minimumWidth(),0) + QSize(9,0)
                           + QSize(cb->minimumWidth(),0) );

        }
//        else if (searchType == PopSearch)
//        {
//            QLineEdit * lineEdit = new QLineEdit(this);
//            lineEdit->setObjectName("param2");
//            lineEdit->setValidator(numValidator);

//            hbox->addSpacing(9);
//            hbox->addWidget(toLbl, Qt::AlignLeft);
//            hbox->addSpacing(9);
//            hbox->addWidget(lineEdit, Qt::AlignLeft);
//            hbox->addStretch();
//            setMinimumSize(minimumSize() + QSize(9,0)
//                           + QSize(toLbl->minimumWidth(),0) + QSize(9,0)
//                           + QSize(lineEdit->minimumWidth(),0) );
//        }
        hbox->invalidate();
        adjustSize();
        show();
    } else {
        adjustForSearchType(searchType);
    }
}

bool ExprParamElement::ignoreCase()
{
    return (isStringSearchExpression()
            && (findChild<QCheckBox*>("ignoreCaseCB"))
                                                ->checkState()==Qt::Checked);
}

QString ExprParamElement::getStrSearchValue()
{
    if (!isStringSearchExpression()) return "";

    QLineEdit * lineEdit = findChild<QLineEdit*>("param1");
    return lineEdit->displayText();
}

uint64_t ExprParamElement::getIntValueFromField(QString fieldName, bool isToField,bool *ok)
{
    uint64_t val = 0;
    if(ok!=NULL)
        *ok=true ;
    QString suffix = (isToField) ? "2": "1" ;

    // NOTE qFindChild necessary for MSVC 6 compatibility!!
    switch (searchType)
    {
        case DateSearch:
        {
            QDateEdit * dateEdit = findChild<QDateEdit *> (fieldName + suffix);
#if QT_VERSION < QT_VERSION_CHECK(5,15,0)
            QDateTime time = QDateTime(dateEdit->date());
#else
            QDateTime time = dateEdit->date().startOfDay();
#endif
            val = (uint64_t)time.toTime_t();
            break;
        }
        case SizeSearch:
        {
            QLineEdit * lineEditSize = findChild<QLineEdit*>(fieldName + suffix);
            bool ok2 = false;
            val = (lineEditSize->displayText()).toULongLong(&ok2);
            if (ok2) 
            {
                RSComboBox * cb = findChild<RSComboBox*>((QString("unitsCb") + suffix));
                QVariant data = cb->itemData(cb->currentIndex());
                val *= data.toULongLong();
            }
            else
                if(ok!=NULL)
                    *ok=false ;
           
            break;
        }
        case PopSearch: // not implemented
/*        {
            QLineEdit * lineEditPop = qFindChild<QLineEdit*>(elem, (fieldName + suffix));
            bool ok = false;
            val = (lineEditPop->displayText()).toInt(&ok);
            if (!ok) {
                val = -1;
            }
            break;
        }*/
        case NameSearch:
        case PathSearch:
        case ExtSearch:
        case HashSearch:
        default:
            // shouldn't be here...val stays at -1
            if(ok!=NULL)
            *ok=false ;
    }
   
    return val;
}

uint64_t ExprParamElement::getIntValue()
{
    return getIntValueFromField("param");
}

uint64_t ExprParamElement::getIntLowValue()
{
    return getIntValue();
}

uint64_t ExprParamElement::getIntHighValue()
{
    if (!inRangedConfig) return getIntValue();
    return getIntValueFromField("param", true);
}


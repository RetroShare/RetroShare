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
#define STR_FIELDS_MIN_WIDTH  200
#define SIZE_FIELDS_MIN_WIDTH  80
#define DATE_FIELDS_MIN_WIDTH  100
#define FIELDS_MIN_HEIGHT    26

#define LOGICAL_OP_CB_WIDTH 70
#define STD_CB_WIDTH        90
#define CONDITION_CB_WIDTH 170

const int GuiExprElement::AND_INDEX         = 0;
const int GuiExprElement::OR_INDEX          = 1;
const int GuiExprElement::XOR_INDEX         = 2;

const int GuiExprElement::NAME_INDEX        = 0;
const int GuiExprElement::PATH_INDEX        = 1;
const int GuiExprElement::EXT_INDEX         = 2;
const int GuiExprElement::HASH_INDEX         = 3;
/*const int GuiExprElement::KEYWORDS_INDEX    = ???;
const int GuiExprElement::COMMENTS_INDEX    = ???;
const int GuiExprElement::META_INDEX        = ???;*/
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
                : QWidget(parent)
{
    if (!GuiExprElement::initialised) 
    {
        initialiseOptionsLists();
    }
    searchType = NameSearch;

}


void GuiExprElement::initialiseOptionsLists()
{
    const QString AND     = tr("and");
    const QString OR      = tr("and / or");
    const QString XOR     = tr("or"); // exclusive or

    const QString NAME    = tr("Name");
    const QString PATH    = tr("Path");
    const QString EXT     = tr("Extension");
    const QString HASH     = tr("Hash");
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
//     GuiExprElement::searchTermsOptionsList.append(POP);
    
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
    the implementation.there 
    For example consider "size greater than 100kb" selected in the GUI.
    The rsexpr.cc impl returns true if the CONDITION specified is greater than the file size passed as argument 
    as rsexpr iterates through the files. So, the user wants files that are greater than 100kb but the impl returns
    files where the condition is greater than the file size i.e. files whose size is less than or equal to the condition 
    Therefore we invert the mapping of rel conditions here to match the behaviour of the impl.
*/    
    GuiExprElement::relConditionIndexMap[GuiExprElement::LT_INDEX] = RsRegularExpression::GreaterEquals;
    GuiExprElement::relConditionIndexMap[GuiExprElement::LTE_INDEX] = RsRegularExpression::Greater;
    GuiExprElement::relConditionIndexMap[GuiExprElement::EQUAL_INDEX] = RsRegularExpression::Equals;
    GuiExprElement::relConditionIndexMap[GuiExprElement::GTE_INDEX] = RsRegularExpression::Smaller;
    GuiExprElement::relConditionIndexMap[GuiExprElement::GT_INDEX] = RsRegularExpression::SmallerEquals;
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

QHBoxLayout * GuiExprElement::createLayout(QWidget * parent)
{
    QHBoxLayout * hboxLayout;
    if (parent == 0) 
    {
        hboxLayout = new QHBoxLayout();
    } else {
        hboxLayout = new QHBoxLayout(parent);
    }
    hboxLayout->setMargin(0);
    hboxLayout->setSpacing(0);
    return hboxLayout;
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
    internalframe = new QFrame(this);
    internalframe->setLayout(createLayout());
    cb = new QComboBox(this);
    cb->setMinimumSize(LOGICAL_OP_CB_WIDTH, FIELDS_MIN_HEIGHT);
    cb->addItems(GuiExprElement::exprOpsList);
    internalframe->layout()->addWidget(cb);
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
    internalframe = new QFrame(this);
    internalframe->setLayout(createLayout());
    cb = new QComboBox(this);
    cb->setMinimumSize(STD_CB_WIDTH, FIELDS_MIN_HEIGHT);
    connect (cb, SIGNAL(currentIndexChanged(int)),
             this, SIGNAL(currentIndexChanged(int)));
    cb->addItems(GuiExprElement::searchTermsOptionsList);
    internalframe->layout()->addWidget(cb);
}
QString ExprTermsElement::toString()
{
    return GuiExprElement::termsStrMap[cb->currentIndex()];
}

/* ******************************************************************/
/* *********** C O N D I T I O N    E L E M E N T  ******************/
/* ******************************************************************/
ExprConditionElement::ExprConditionElement(ExprSearchType type, QWidget * parent)
                : GuiExprElement(parent)
{
    internalframe = new QFrame(this);
    internalframe->setLayout(createLayout());
    cb = new QComboBox(this);
    cb->setMinimumSize(CONDITION_CB_WIDTH, FIELDS_MIN_HEIGHT);
    connect (cb, SIGNAL(currentIndexChanged(int)),
             this, SIGNAL(currentIndexChanged(int)));
    cb->addItems(getConditionOptions(type));
    internalframe->layout()->addWidget(cb);
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
ExprParamElement::ExprParamElement(ExprSearchType type, QWidget * parent)
                : GuiExprElement(parent)
{
    internalframe = new QFrame(this);
    internalframe->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    internalframe->setLayout(createLayout());
    inRangedConfig = false;
    searchType = type;
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
            QDateEdit * dateEdit =  internalframe->findChild<QDateEdit *> ("param1");
            str = dateEdit->text();
            if (inRangedConfig)
            {
                str += QString(" ") + tr("to") + QString(" ");
                dateEdit = internalframe->findChild<QDateEdit *> ("param2");
                str += dateEdit->text();
            }
        } else if (searchType == SizeSearch) 
        {
            QLineEdit * lineEditSize =  internalframe->findChild<QLineEdit*>("param1");
            str = ("" == lineEditSize->text()) ? "0"
                                               : lineEditSize->text();
            QComboBox * cb = internalframe->findChild<QComboBox*> ("unitsCb1");
            str += QString(" ") + cb->itemText(cb->currentIndex());
            if (inRangedConfig)
            {
                str += QString(" ") + tr("to") + QString(" ");
                lineEditSize =  internalframe->findChild<QLineEdit*>("param2");
                str += ("" == lineEditSize->text()) ? "0"
                                                    : lineEditSize->text();
                cb = internalframe->findChild<QComboBox*> ("unitsCb2");
                str += QString(" ") + cb->itemText(cb->currentIndex());
            }
        }
    }
    return str;
}


void ExprParamElement::adjustForSearchType(ExprSearchType type)
{    
    // record which search type is active
    searchType = type;
    QRegExp regExp("0|[1-9][0-9]*");
    numValidator = new QRegExpValidator(regExp, this);
    QRegExp hexRegExp("[A-Fa-f0-9]*");
    hexValidator = new QRegExpValidator(hexRegExp, this);
    
    // remove all elements
    QList<QWidget*> children = internalframe->findChildren<QWidget*>();
    QWidget* child;
    QLayout * lay_out = internalframe->layout();
     while (!children.isEmpty())
    {
        child = children.takeLast();
        child->hide();
        lay_out->removeWidget(child);
        delete child;
    }
    delete lay_out;

    QHBoxLayout* hbox = createLayout();
    internalframe->setLayout(hbox);
    internalframe->setMinimumSize(320,26);

    if (isStringSearchExpression())
    {
        // set up for default of a simple input field
        QLineEdit* lineEdit = new QLineEdit(internalframe);
        lineEdit->setMinimumSize(STR_FIELDS_MIN_WIDTH, FIELDS_MIN_HEIGHT);
        lineEdit->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
        lineEdit->setObjectName("param1");
        hbox->addWidget(lineEdit);
        hbox->addSpacing(9);
        QCheckBox* icCb = new QCheckBox(tr("ignore case"), internalframe);
        icCb->setObjectName("ignoreCaseCB");
	icCb->setCheckState(Qt::Checked);
	// hex search specifics: hidden case sensitivity and hex validator
	if (searchType == HashSearch) {
		icCb->hide();
		lineEdit->setValidator(hexValidator);        
	}
	hbox->addWidget(icCb);
        hbox->addStretch();
	
    } else if (searchType == DateSearch) 
    {
        QDateEdit * dateEdit = new QDateEdit(QDate::currentDate(), internalframe);
        dateEdit->setMinimumSize(DATE_FIELDS_MIN_WIDTH, FIELDS_MIN_HEIGHT);
        dateEdit->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        dateEdit->setDisplayFormat(tr("dd.MM.yyyy"));
        dateEdit->setObjectName("param1");
        dateEdit->setMinimumDate(QDate(1970, 1, 1));
        dateEdit->setMaximumDate(QDate(2099, 12,31));
        hbox->addWidget(dateEdit, Qt::AlignLeft);
        hbox->addStretch();
    } else if (searchType == SizeSearch) 
    {
        QLineEdit * lineEdit = new QLineEdit(internalframe);
        lineEdit->setMinimumSize(SIZE_FIELDS_MIN_WIDTH, FIELDS_MIN_HEIGHT);
        lineEdit->setMaximumSize(SIZE_FIELDS_MIN_WIDTH, FIELDS_MIN_HEIGHT);
        lineEdit->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        lineEdit->setObjectName("param1");
        lineEdit->setValidator(numValidator);
        hbox->addWidget(lineEdit, Qt::AlignLeft);

        QComboBox * cb = new QComboBox(internalframe);
        cb->setObjectName("unitsCb1");
        cb-> addItem(tr("KB"), QVariant(1024));
        cb->addItem(tr("MB"), QVariant(1048576));
        cb->addItem(tr("GB"), QVariant(1073741824));
        hbox->addSpacing(9);
        internalframe->layout()->addWidget(cb);
        hbox->addStretch();
    } 

    /* POP Search not implemented
    else if (searchType == PopSearch)
    {
        QLineEdit * lineEdit = new QLineEdit(elem);
        lineEdit->setObjectName("param1");
        lineEdit->setValidator(numValidator);
        elem->layout()->addWidget(lineEdit);
    }*/
    hbox->invalidate();
    internalframe->adjustSize();
    internalframe->show();
    this->adjustSize();
}

void ExprParamElement::setRangedSearch(bool ranged)
{    
    
    if (inRangedConfig == ranged) return; // nothing to do here
    inRangedConfig = ranged;
    QHBoxLayout* hbox = (dynamic_cast<QHBoxLayout*>(internalframe->layout()));

    // add additional or remove extra input fields depending on whether
    // ranged search or not
    if (inRangedConfig)
    {
        QLabel * toLbl = new QLabel(tr("to"));
        toLbl->setMinimumSize(10, FIELDS_MIN_HEIGHT);
        toLbl->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

        if (searchType ==  DateSearch) {
           internalframe->setMinimumSize(250,26);            
            QDateEdit * dateEdit = new QDateEdit(QDate::currentDate(), internalframe);
            dateEdit->setMinimumSize(DATE_FIELDS_MIN_WIDTH, FIELDS_MIN_HEIGHT);
            dateEdit->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            dateEdit->setObjectName("param2");
            dateEdit->setDisplayFormat(tr("dd.MM.yyyy"));
            dateEdit->setMinimumDate(QDate(1970, 1, 1));
            dateEdit->setMaximumDate(QDate(2099, 12,31));
            
            hbox->addSpacing(9);
            hbox->addWidget(toLbl, Qt::AlignLeft);
            hbox->addSpacing(9);
            hbox->addWidget(dateEdit, Qt::AlignLeft);
            hbox->addStretch();
        } else if (searchType == SizeSearch) {
            internalframe->setMinimumSize(340,26);
            QLineEdit * lineEdit = new QLineEdit(internalframe);
            lineEdit->setMinimumSize(SIZE_FIELDS_MIN_WIDTH, FIELDS_MIN_HEIGHT);
            lineEdit->setMaximumSize(SIZE_FIELDS_MIN_WIDTH, FIELDS_MIN_HEIGHT);
            lineEdit->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            lineEdit->setObjectName("param2");
            lineEdit->setValidator(numValidator);
            
            QComboBox * cb = new QComboBox(internalframe);
            cb->setObjectName("unitsCb2");
            cb-> addItem(tr("KB"), QVariant(1024));
            cb->addItem(tr("MB"), QVariant(1048576));
            cb->addItem(tr("GB"), QVariant(1073741824));
            
            hbox->addSpacing(9);
            hbox->addWidget(toLbl, Qt::AlignLeft);
            hbox->addSpacing(9);
            hbox->addWidget(lineEdit, Qt::AlignLeft);
            hbox->addSpacing(9);
            hbox->addWidget(cb);
            hbox->addStretch();
        } 
//         else if (searchType == PopSearch)
//         {
//             elem->layout()->addWidget(new QLabel(tr("to")), Qt::AlignCenter);
//             QLineEdit * lineEdit = new QLineEdit(elem);
//             lineEdit->setObjectName("param2");
//             lineEdit->setValidator(numValidator);
//             elem->layout()->addWidget(slineEdit);
//         }
        hbox->invalidate();
        internalframe->adjustSize();
        internalframe->show();
        this->adjustSize();
    } else {
        adjustForSearchType(searchType);
    }
}

bool ExprParamElement::ignoreCase()
{    
    return (isStringSearchExpression()
            && (internalframe->findChild<QCheckBox*>("ignoreCaseCB"))
                                                ->checkState()==Qt::Checked);
}

QString ExprParamElement::getStrSearchValue()
{
    if (!isStringSearchExpression()) return "";

    QLineEdit * lineEdit = internalframe->findChild<QLineEdit*>("param1");
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
            QDateEdit * dateEdit = internalframe->findChild<QDateEdit *> (fieldName + suffix);
            QDateTime * time = new QDateTime(dateEdit->date());
            val = (uint64_t)time->toTime_t();
            break;
        }   
        case SizeSearch:
        {
            QLineEdit * lineEditSize = internalframe->findChild<QLineEdit*>(fieldName + suffix);
            bool ok2 = false;
            val = (lineEditSize->displayText()).toULongLong(&ok2);
            if (ok2) 
				{
				QComboBox * cb = internalframe->findChild<QComboBox*>((QString("unitsCb") + suffix));
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


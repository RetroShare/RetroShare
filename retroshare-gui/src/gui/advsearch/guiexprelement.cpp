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

#include "guiexprelement.h"
#define STR_FIELDS_MIN_WIDTH  200
#define SIZE_FIELDS_MIN_WIDTH  80
#define DATE_FIELDS_MIN_WIDTH  100
#define FIELDS_MIN_HEIGHT    30

#define LOGICAL_OP_CB_WIDTH 70
#define STD_CB_WIDTH        90
#define CONDITION_CB_WIDTH 170

const QString GuiExprElement::AND     = QObject::tr("and");
const QString GuiExprElement::OR      = QObject::tr("and / or");
const QString GuiExprElement::XOR     = QObject::tr("or"); // exclusive or

const QString GuiExprElement::NAME    = QObject::tr("Name");
const QString GuiExprElement::PATH    = QObject::tr("Path");
const QString GuiExprElement::EXT     = QObject::tr("Extension");
const QString GuiExprElement::HASH     = QObject::tr("Hash");
//const QString GuiExprElement::KEYWORDS= QObject::tr("Keywords");
//const QString GuiExprElement::COMMENTS= QObject::tr("Comments");
//const QString GuiExprElement::META    = QObject::tr("Meta");
const QString GuiExprElement::DATE    = QObject::tr("Date");
const QString GuiExprElement::SIZE    = QObject::tr("Size");
const QString GuiExprElement::POP     = QObject::tr("Popularity");

const QString GuiExprElement::CONTAINS= QObject::tr("contains");
const QString GuiExprElement::CONTALL = QObject::tr("contains all");
const QString GuiExprElement::IS      = QObject::tr("is");

const QString GuiExprElement::LT      = QObject::tr("less than");
const QString GuiExprElement::LTE     = QObject::tr("less than or equal");
const QString GuiExprElement::EQUAL   = QObject::tr("equals");
const QString GuiExprElement::GTE     = QObject::tr("greater than or equal");
const QString GuiExprElement::GT      = QObject::tr("greater than");
const QString GuiExprElement::RANGE   = QObject::tr("is in range");


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

QStringList * GuiExprElement::exprOpsList = new QStringList();
QStringList * GuiExprElement::searchTermsOptionsList = new QStringList();
QStringList * GuiExprElement::stringOptionsList = new QStringList();
QStringList * GuiExprElement::relOptionsList = new QStringList();

QMap<int, ExprSearchType> * GuiExprElement::TermsIndexMap = new QMap<int, ExprSearchType>();

QMap<int, LogicalOperator> * GuiExprElement::logicalOpIndexMap = new QMap<int, LogicalOperator>();
QMap<int, StringOperator> * GuiExprElement::strConditionIndexMap = new QMap<int, StringOperator>();
QMap<int, RelOperator> * GuiExprElement::relConditionIndexMap = new QMap<int, RelOperator>();

QMap<int, QString> * GuiExprElement::logicalOpStrMap = new QMap<int, QString>();
QMap<int, QString> * GuiExprElement::termsStrMap = new QMap<int, QString>();
QMap<int, QString> * GuiExprElement::strConditionStrMap = new QMap<int, QString>();
QMap<int, QString> * GuiExprElement::relConditionStrMap = new QMap<int, QString>();

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
    exprOpsList->append(AND);
    exprOpsList->append(OR);
    exprOpsList->append(XOR);

    GuiExprElement::searchTermsOptionsList->append(NAME); 
    GuiExprElement::searchTermsOptionsList->append(PATH); 
    GuiExprElement::searchTermsOptionsList->append(EXT); 
    GuiExprElement::searchTermsOptionsList->append(HASH); 
    //GuiExprElement::searchTermsOptionsList->append(KEYWORDS); 
    //GuiExprElement::searchTermsOptionsList->append(COMMENTS); 
    //GuiExprElement::searchTermsOptionsList->append(META); 
    GuiExprElement::searchTermsOptionsList->append(DATE); 
    GuiExprElement::searchTermsOptionsList->append(SIZE); 
//     GuiExprElement::searchTermsOptionsList->append(POP);
    
    GuiExprElement::stringOptionsList->append(CONTAINS);
    GuiExprElement::stringOptionsList->append(CONTALL);
    GuiExprElement::stringOptionsList->append(IS);
    
    GuiExprElement::relOptionsList->append(LT);
    GuiExprElement::relOptionsList->append(LTE);
    GuiExprElement::relOptionsList->append(EQUAL);
    GuiExprElement::relOptionsList->append(GTE);
    GuiExprElement::relOptionsList->append(GT);
    GuiExprElement::relOptionsList->append(RANGE);

    // now the maps
    (*GuiExprElement::logicalOpIndexMap)[GuiExprElement::AND_INDEX] = AndOp;
    (*GuiExprElement::logicalOpIndexMap)[GuiExprElement::OR_INDEX] = OrOp;
    (*GuiExprElement::logicalOpIndexMap)[GuiExprElement::XOR_INDEX] = XorOp;
    
    (*GuiExprElement::TermsIndexMap)[GuiExprElement::NAME_INDEX] = NameSearch;
    (*GuiExprElement::TermsIndexMap)[GuiExprElement::PATH_INDEX] = PathSearch;
    (*GuiExprElement::TermsIndexMap)[GuiExprElement::EXT_INDEX] = ExtSearch;
    (*GuiExprElement::TermsIndexMap)[GuiExprElement::HASH_INDEX] = HashSearch;
    (*GuiExprElement::TermsIndexMap)[GuiExprElement::DATE_INDEX] = DateSearch;
    (*GuiExprElement::TermsIndexMap)[GuiExprElement::SIZE_INDEX] = SizeSearch;
    (*GuiExprElement::TermsIndexMap)[GuiExprElement::POP_INDEX] = PopSearch;
    
    (*GuiExprElement::strConditionIndexMap)[GuiExprElement::CONTAINS_INDEX] = ContainsAnyStrings;
    (*GuiExprElement::strConditionIndexMap)[GuiExprElement::CONTALL_INDEX] = ContainsAllStrings;
    (*GuiExprElement::strConditionIndexMap)[GuiExprElement::IS_INDEX] = EqualsString;
    
/*  W A R N I N G !!!!
    the cb elements correspond to their inverse rel op counterparts in rsexpr.h due to the nature of 
    the implementation.there 
    For example consider "size greater than 100kb" selected in the GUI.
    The rsexpr.cc impl returns true if the CONDITION specified is greater than the file size passed as argument 
    as rsexpr iterates through the files. So, the user wants files that are greater than 100kb but the impl returns
    files where the condition is greater than the file size i.e. files whose size is less than or equal to the condition 
    Therefore we invert the mapping of rel conditions here to match the behaviour of the impl.
*/    
    (*GuiExprElement::relConditionIndexMap)[GuiExprElement::LT_INDEX] = GreaterEquals;
    (*GuiExprElement::relConditionIndexMap)[GuiExprElement::LTE_INDEX] = Greater;
    (*GuiExprElement::relConditionIndexMap)[GuiExprElement::EQUAL_INDEX] = Equals;
    (*GuiExprElement::relConditionIndexMap)[GuiExprElement::GTE_INDEX] = Smaller;
    (*GuiExprElement::relConditionIndexMap)[GuiExprElement::GT_INDEX] = SmallerEquals;
    (*GuiExprElement::relConditionIndexMap)[GuiExprElement::RANGE_INDEX] = InRange;

    // the string to index map
    (*GuiExprElement::termsStrMap)[GuiExprElement::NAME_INDEX] = GuiExprElement::NAME;
    (*GuiExprElement::termsStrMap)[GuiExprElement::PATH_INDEX] = GuiExprElement::PATH;
    (*GuiExprElement::termsStrMap)[GuiExprElement::EXT_INDEX]  = GuiExprElement::EXT;
    (*GuiExprElement::termsStrMap)[GuiExprElement::HASH_INDEX]  = GuiExprElement::HASH;
    (*GuiExprElement::termsStrMap)[GuiExprElement::DATE_INDEX] = GuiExprElement::DATE;
    (*GuiExprElement::termsStrMap)[GuiExprElement::SIZE_INDEX] = GuiExprElement::SIZE;
    (*GuiExprElement::termsStrMap)[GuiExprElement::POP_INDEX]  = GuiExprElement::POP;

    (*GuiExprElement::logicalOpStrMap)[GuiExprElement::AND_INDEX] = GuiExprElement::AND;
    (*GuiExprElement::logicalOpStrMap)[GuiExprElement::XOR_INDEX] = GuiExprElement::XOR;
    (*GuiExprElement::logicalOpStrMap)[GuiExprElement::OR_INDEX]  = GuiExprElement::OR;
    
    (*GuiExprElement::strConditionStrMap)[GuiExprElement::CONTAINS_INDEX] = GuiExprElement::CONTAINS;
    (*GuiExprElement::strConditionStrMap)[GuiExprElement::CONTALL_INDEX] = GuiExprElement::CONTALL;
    (*GuiExprElement::strConditionStrMap)[GuiExprElement::IS_INDEX] = GuiExprElement::IS;
    
    (*GuiExprElement::relConditionStrMap)[GuiExprElement::LT_INDEX] = GuiExprElement::LT;
    (*GuiExprElement::relConditionStrMap)[GuiExprElement::LTE_INDEX] = GuiExprElement::LTE;
    (*GuiExprElement::relConditionStrMap)[GuiExprElement::EQUAL_INDEX] = GuiExprElement::EQUAL;
    (*GuiExprElement::relConditionStrMap)[GuiExprElement::GTE_INDEX] = GuiExprElement::GTE;
    (*GuiExprElement::relConditionStrMap)[GuiExprElement::GT_INDEX] = GuiExprElement::GT;
    (*GuiExprElement::relConditionStrMap)[GuiExprElement::RANGE_INDEX] = GuiExprElement::RANGE;


    GuiExprElement::initialised = true;
}

QStringList* GuiExprElement::getConditionOptions(ExprSearchType t)
{
    QStringList * list = new QStringList();
    switch (t) {
        case NameSearch:
        case PathSearch:
        case ExtSearch:
        case HashSearch:
            list = GuiExprElement::stringOptionsList;
            break;
        case DateSearch:
        case PopSearch:
        case SizeSearch:
        default:
            list = GuiExprElement::relOptionsList;
    }
    return list;
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
    cb->addItems(*(GuiExprElement::exprOpsList));
    internalframe->layout()->addWidget(cb);
}

QString ExprOpElement::toString()
{
    return (*GuiExprElement::logicalOpStrMap)[cb->currentIndex()];
}


LogicalOperator ExprOpElement::getLogicalOperator()
{
    return (*GuiExprElement::logicalOpIndexMap)[cb->currentIndex()];
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
    cb->addItems(*(GuiExprElement::searchTermsOptionsList));
    internalframe->layout()->addWidget(cb);
}
QString ExprTermsElement::toString()
{
    return (*GuiExprElement::termsStrMap)[cb->currentIndex()];
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
    cb->addItems(*(getConditionOptions(type)));
    internalframe->layout()->addWidget(cb);
}


QString ExprConditionElement::toString()
{
    QString str = "";
    if (isStringSearchExpression())
    {
        str = (*GuiExprElement::strConditionStrMap)[cb->currentIndex()];
    } else {
        str = (*GuiExprElement::relConditionStrMap)[cb->currentIndex()];
    }    
    return str;
}

RelOperator ExprConditionElement::getRelOperator()
{
    return (*GuiExprElement::relConditionIndexMap)[cb->currentIndex()];
}

StringOperator ExprConditionElement::getStringOperator()
{
    return (*GuiExprElement::strConditionIndexMap)[cb->currentIndex()];
}

void ExprConditionElement::adjustForSearchType(ExprSearchType type)
{
    cb->clear();
    cb->addItems(*(getConditionOptions(type)));
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
            QDateEdit * dateEdit =  qFindChild<QDateEdit *> (internalframe, "param1");
            str = dateEdit->text();
            if (inRangedConfig)
            {
                str += QString(" ") + tr("to") + QString(" ");
                dateEdit = qFindChild<QDateEdit *> (internalframe, "param2");
                str += dateEdit->text();
            }
        } else if (searchType == SizeSearch) 
        {
            QLineEdit * lineEditSize =  qFindChild<QLineEdit*>(internalframe, "param1");
            str = ("" == lineEditSize->text()) ? "0"
                                               : lineEditSize->text();
            QComboBox * cb = qFindChild<QComboBox*> (internalframe, "unitsCb1");
            str += QString(" ") + cb->itemText(cb->currentIndex());
            if (inRangedConfig)
            {
                str += QString(" ") + tr("to") + QString(" ");
                lineEditSize =  qFindChild<QLineEdit*>(internalframe, "param2");
                str += ("" == lineEditSize->text()) ? "0"
                                                    : lineEditSize->text();
                cb = qFindChild<QComboBox*> (internalframe, "unitsCb2");
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
    QList<QWidget*> children = qFindChildren<QWidget*>(internalframe);
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
    internalframe->setMinimumSize(320,30);

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
           internalframe->setMinimumSize(250,30);            
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
            internalframe->setMinimumSize(340,30);
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
            && (qFindChild<QCheckBox*>(internalframe, "ignoreCaseCB"))
                                                ->checkState()==Qt::Checked);
}

QString ExprParamElement::getStrSearchValue()
{
    if (!isStringSearchExpression()) return "";

    QLineEdit * lineEdit = qFindChild<QLineEdit*>(internalframe, "param1");
    return lineEdit->displayText();
}

int ExprParamElement::getIntValueFromField(QString fieldName, bool isToField)
{
    int val = -1;
    QString suffix = (isToField) ? "2": "1" ;

    // NOTE qFindChild necessary for MSVC 6 compatibility!!
    switch (searchType)
    {
        case DateSearch:
        {
            QDateEdit * dateEdit =  qFindChild<QDateEdit *> (internalframe, (fieldName + suffix));
            QDateTime * time = new QDateTime(dateEdit->date());
            val = time->toTime_t();
            break;
        }   
        case SizeSearch:
        {
            QLineEdit * lineEditSize =  qFindChild<QLineEdit*>(internalframe, (fieldName + suffix));
            bool ok = false;
            val = (lineEditSize->displayText()).toInt(&ok);
            if (ok) {
                QComboBox * cb = qFindChild<QComboBox*> (internalframe, (QString("unitsCb") + suffix));
                QVariant data = cb->itemData(cb->currentIndex());
                val *= data.toInt();
            } else {
                val = -1;
            }
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
            val = -1;
    }
   
    return val;
}

int ExprParamElement::getIntValue()
{
    return getIntValueFromField("param");
}

int ExprParamElement::getIntLowValue()
{
    return getIntValue();
}

int ExprParamElement::getIntHighValue()
{
    if (!inRangedConfig) return getIntValue();
    return getIntValueFromField("param", true);
}


/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2007 Ricardo Villalba <rvm@escomposlinux.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/* This is based on qq14-actioneditor-code.zip from Qt */


#include "actionseditor.h"

#include <QTableWidget>
#include <QHeaderView>

#include <QLayout>
#include <QObject>
#include <QPushButton>
#include <QString>
#include <QSettings>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QFileInfo>
#include <QRegExp>
#include <QApplication>
#include <QAction>

#include "images.h"
#include "filedialog.h"
#include "helper.h"

#include "shortcutgetter.h"


/*
#include <QLineEdit>
#include <QItemDelegate>

class MyDelegate : public QItemDelegate 
{
public:
	MyDelegate(QObject *parent = 0);

	QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                           const QModelIndex &index) const;
		virtual void setModelData(QWidget * editor, QAbstractItemModel * model, 
                              const QModelIndex & index ) const;
};

MyDelegate::MyDelegate(QObject *parent) : QItemDelegate(parent)
{
}

static QString old_accel_text;

QWidget * MyDelegate::createEditor(QWidget *parent, 
								   const QStyleOptionViewItem & option,
	                               const QModelIndex & index) const
{
	qDebug("MyDelegate::createEditor");

	old_accel_text = index.model()->data(index, Qt::DisplayRole).toString();
	//qDebug( "text: %s", old_accel_text.toUtf8().data());
	
	return QItemDelegate::createEditor(parent, option, index);
}

void MyDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                              const QModelIndex &index) const
{
	QLineEdit *line_edit = static_cast<QLineEdit*>(editor);

	QString accelText = QKeySequence(line_edit->text()).toString();
	if (accelText.isEmpty() && !line_edit->text().isEmpty()) {
		model->setData(index, old_accel_text);
	}
	else {
		model->setData(index, accelText);
	}
}
*/


#if USE_MULTIPLE_SHORTCUTS
QString ActionsEditor::shortcutsToString(QList <QKeySequence> shortcuts_list) {
	QString accelText = "";

	for (int n=0; n < shortcuts_list.count(); n++) {
		accelText += shortcuts_list[n].toString(QKeySequence::PortableText);
		if (n < (shortcuts_list.count()-1)) accelText += ", ";
	}

	return accelText;
}

QList <QKeySequence> ActionsEditor::stringToShortcuts(QString shortcuts) {
	QList <QKeySequence> shortcuts_list;

	QStringList l = shortcuts.split(',');

	for (int n=0; n < l.count(); n++) {
		//qDebug("%s", l[n].toUtf8().data());
		QString s = QKeySequence( l[n].simplified() );
		shortcuts_list.append( s );
		//qDebug("ActionsEditor::stringToShortcuts: shortcut %d: '%s'", n, s.toUtf8().data());
	}

	return shortcuts_list;
}
#endif


#define COL_CONFLICTS 0
#define COL_SHORTCUT 1
#define COL_DESC 2
#define COL_NAME 3

ActionsEditor::ActionsEditor(QWidget * parent, Qt::WindowFlags f)
	: QWidget(parent, f)
{
	latest_dir = Helper::shortcutsPath();

    actionsTable = new QTableWidget(0, COL_NAME +1, this);
	actionsTable->setSelectionMode( QAbstractItemView::SingleSelection );
	actionsTable->verticalHeader()->hide();

	actionsTable->horizontalHeader()->setResizeMode(COL_DESC, QHeaderView::Stretch);
	actionsTable->horizontalHeader()->setResizeMode(COL_NAME, QHeaderView::Stretch);

	actionsTable->setAlternatingRowColors(true);
#if USE_SHORTCUTGETTER
	actionsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
	actionsTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
#endif
	//actionsTable->setItemDelegateForColumn( COL_SHORTCUT, new MyDelegate(actionsTable) );

#if !USE_SHORTCUTGETTER
	connect(actionsTable, SIGNAL(currentItemChanged(QTableWidgetItem *,QTableWidgetItem *)),
            this, SLOT(recordAction(QTableWidgetItem *)) );
	connect(actionsTable, SIGNAL(itemChanged(QTableWidgetItem *)),
            this, SLOT(validateAction(QTableWidgetItem *)) );
#else
	connect(actionsTable, SIGNAL(itemActivated(QTableWidgetItem *)),
            this, SLOT(editShortcut()) );
#endif

	saveButton = new QPushButton(this);
	loadButton = new QPushButton(this);

	connect(saveButton, SIGNAL(clicked()), this, SLOT(saveActionsTable()));
	connect(loadButton, SIGNAL(clicked()), this, SLOT(loadActionsTable()));

#if USE_SHORTCUTGETTER
	editButton = new QPushButton(this);
	connect( editButton, SIGNAL(clicked()), this, SLOT(editShortcut()) );
#endif

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->setSpacing(8);
#if USE_SHORTCUTGETTER
	buttonLayout->addWidget(editButton);
#endif
    buttonLayout->addStretch(1);
	buttonLayout->addWidget(loadButton);
	buttonLayout->addWidget(saveButton);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setMargin(8);
    mainLayout->setSpacing(8);
    mainLayout->addWidget(actionsTable);
    mainLayout->addLayout(buttonLayout);

	retranslateStrings();
}

ActionsEditor::~ActionsEditor() {
}

void ActionsEditor::retranslateStrings() {
	actionsTable->setHorizontalHeaderLabels( QStringList() << "" <<
		tr("Shortcut") << tr("Description") << tr("Name") );

	saveButton->setText(tr("&Save"));
	saveButton->setIcon(Images::icon("save"));

	loadButton->setText(tr("&Load"));
	loadButton->setIcon(Images::icon("open"));

#if USE_SHORTCUTGETTER
	editButton->setText(tr("&Change shortcut..."));
#endif

	//updateView(); // The actions are translated later, so it's useless
}

bool ActionsEditor::isEmpty() {
	return actionsList.isEmpty();
}

void ActionsEditor::clear() {
	actionsList.clear();
}

void ActionsEditor::addActions(QWidget *widget) {
	QAction *action;

	QList<QAction *> actions = widget->findChildren<QAction *>();
	for (int n=0; n < actions.count(); n++) {
		action = static_cast<QAction*> (actions[n]);
		if (!action->objectName().isEmpty())
	        actionsList.append(action);
    }

	updateView();
}

void ActionsEditor::updateView() {
	actionsTable->setRowCount( actionsList.count() );

    QAction *action;
	QString accelText;

#if !USE_SHORTCUTGETTER
	dont_validate = true;
#endif
	//actionsTable->setSortingEnabled(false);

	for (int n=0; n < actionsList.count(); n++) {
		action = static_cast<QAction*> (actionsList[n]);

#if USE_MULTIPLE_SHORTCUTS
		accelText = shortcutsToString( action->shortcuts() );
#else
		accelText = action->shortcut().toString();
#endif

		// Conflict column
		QTableWidgetItem * i_conf = new QTableWidgetItem();

		// Name column
		QTableWidgetItem * i_name = new QTableWidgetItem(action->objectName());

		// Desc column
		QTableWidgetItem * i_desc = new QTableWidgetItem(action->text().replace("&",""));
		i_desc->setIcon( action->icon() );

		// Shortcut column
		QTableWidgetItem * i_shortcut = new QTableWidgetItem(accelText);

		// Set flags
#if !USE_SHORTCUTGETTER
		i_conf->setFlags(Qt::ItemIsEnabled);
		i_name->setFlags(Qt::ItemIsEnabled);
		i_desc->setFlags(Qt::ItemIsEnabled);
#else
		i_conf->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
		i_name->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
		i_desc->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
		i_shortcut->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
#endif

		// Add items to table
		actionsTable->setItem(n, COL_CONFLICTS, i_conf );
		actionsTable->setItem(n, COL_NAME, i_name );
		actionsTable->setItem(n, COL_DESC, i_desc );
		actionsTable->setItem(n, COL_SHORTCUT, i_shortcut );

	}
	hasConflicts(); // Check for conflicts

	actionsTable->resizeColumnsToContents();
	actionsTable->setCurrentCell(0, COL_SHORTCUT);

#if !USE_SHORTCUTGETTER
	dont_validate = false;
#endif
	//actionsTable->setSortingEnabled(true);
}


void ActionsEditor::applyChanges() {
	qDebug("ActionsEditor::applyChanges");

	for (int row = 0; row < (int)actionsList.size(); ++row) {
		QAction *action = actionsList[row];
		QTableWidgetItem *i = actionsTable->item(row, COL_SHORTCUT);

#if USE_MULTIPLE_SHORTCUTS
		action->setShortcuts( stringToShortcuts(i->text()) );
#else
		action->setShortcut( QKeySequence(i->text()) );
#endif
	}
}

#if !USE_SHORTCUTGETTER
void ActionsEditor::recordAction(QTableWidgetItem * i) {
	//qDebug("ActionsEditor::recordAction");

	//QTableWidgetItem * i = actionsTable->currentItem();
	if (i->column() == COL_SHORTCUT) {
		//qDebug("ActionsEditor::recordAction: %d %d %s", i->row(), i->column(), i->text().toUtf8().data());
		oldAccelText = i->text();
	}
}

void ActionsEditor::validateAction(QTableWidgetItem * i) {
	//qDebug("ActionsEditor::validateAction");
	if (dont_validate) return;

	if (i->column() == COL_SHORTCUT) {
	    QString accelText = QKeySequence(i->text()).toString();

	    if (accelText.isEmpty() && !i->text().isEmpty()) {
			/*
			QAction * action = static_cast<QAction*> (actionsList[i->row()]);
			QString oldAccelText= action->accel().toString();
			*/
	        i->setText(oldAccelText);
		}
	    else {
	        i->setText(accelText);
		}

		if (hasConflicts()) qApp->beep();
	}
}

#else

void ActionsEditor::editShortcut() {
	QTableWidgetItem * i = actionsTable->item( actionsTable->currentRow(), COL_SHORTCUT );
	if (i) {
		ShortcutGetter d;
		QString result = d.exec( i->text() );

		if (!result.isNull()) {
		    QString accelText = QKeySequence(result).toString(QKeySequence::PortableText);
			i->setText(accelText);
			if (hasConflicts()) qApp->beep();
		}
	}
}
#endif

int ActionsEditor::findActionName(const QString & name) {
	for (int row=0; row < actionsTable->rowCount(); row++) {
		if (actionsTable->item(row, COL_NAME)->text() == name) return row;
	}
	return -1;
}

int ActionsEditor::findActionAccel(const QString & accel, int ignoreRow) {
	for (int row=0; row < actionsTable->rowCount(); row++) {
		QTableWidgetItem * i = actionsTable->item(row, COL_SHORTCUT);
		if ( (i) && (i->text() == accel) ) {
			if (ignoreRow == -1) return row;
			else
			if (ignoreRow != row) return row;
		}
	}
	return -1;
}

bool ActionsEditor::hasConflicts() {
	int found;
	bool conflict = false;

	QString accelText;
	QTableWidgetItem *i;

	for (int n=0; n < actionsTable->rowCount(); n++) {
		//actionsTable->setText( n, COL_CONFLICTS, " ");
		i = actionsTable->item( n, COL_CONFLICTS );
		if (i) i->setIcon( QPixmap() );

		i = actionsTable->item(n, COL_SHORTCUT );
		if (i) {
			accelText = i->text();
			if (!accelText.isEmpty()) {
				found = findActionAccel( accelText, n );
				if ( (found != -1) && (found != n) ) {
					conflict = true;
					//actionsTable->setText( n, COL_CONFLICTS, "!");
					actionsTable->item( n, COL_CONFLICTS )->setIcon( Images::icon("conflict") );
				}
			}
		}
	}
	//if (conflict) qApp->beep();
	return conflict;
}


void ActionsEditor::saveActionsTable() {
	QString s = MyFileDialog::getSaveFileName(
                    this, tr("Choose a filename"), 
                    latest_dir,
                    tr("Key files") +" (*.keys)" );

	if (!s.isEmpty()) {
		// If filename has no extension, add it
		if (QFileInfo(s).suffix().isEmpty()) {
			s = s + ".keys";
		}
		if (QFileInfo(s).exists()) {
			int res = QMessageBox::question( this,
					tr("Confirm overwrite?"),
                    tr("The file %1 already exists.\n"
                       "Do you want to overwrite?").arg(s),
                    QMessageBox::Yes,
                    QMessageBox::No,
                    Qt::NoButton);
			if (res == QMessageBox::No ) {
            	return;
			}
		}
		latest_dir = QFileInfo(s).absolutePath();
		bool r = saveActionsTable(s);
		if (!r) {
			QMessageBox::warning(this, tr("Error"), 
               tr("The file couldn't be saved"), 
               QMessageBox::Ok, Qt::NoButton);
		}
	} 
}

bool ActionsEditor::saveActionsTable(const QString & filename) {
	qDebug("ActionsEditor::saveActions: '%s'", filename.toUtf8().data());

	QFile f( filename );
	if ( f.open( QIODevice::WriteOnly ) ) {
		QTextStream stream( &f );
		stream.setCodec("UTF-8");

		for (int row=0; row < actionsTable->rowCount(); row++) {
			stream << actionsTable->item(row, COL_NAME)->text() << "\t" 
                   << actionsTable->item(row, COL_SHORTCUT)->text() << "\n";
		}
		f.close();
		return true;
	}
	return false;
}

void ActionsEditor::loadActionsTable() {
	QString s = MyFileDialog::getOpenFileName(
                    this, tr("Choose a file"),
                    latest_dir, tr("Key files") +" (*.keys)" );

	if (!s.isEmpty()) {
		latest_dir = QFileInfo(s).absolutePath();
		bool r = loadActionsTable(s);
		if (!r) {
			QMessageBox::warning(this, tr("Error"), 
               tr("The file couldn't be loaded"), 
               QMessageBox::Ok, Qt::NoButton);
		}
	}
}

bool ActionsEditor::loadActionsTable(const QString & filename) {
	qDebug("ActionsEditor::loadActions: '%s'", filename.toUtf8().data());

	QRegExp rx("^(.*)\\t(.*)");
	int row;

    QFile f( filename );
    if ( f.open( QIODevice::ReadOnly ) ) {

#if !USE_SHORTCUTGETTER
		dont_validate = true;
#endif

        QTextStream stream( &f );
		stream.setCodec("UTF-8");

        QString line;
        while ( !stream.atEnd() ) {
            line = stream.readLine();
			qDebug("line: '%s'", line.toUtf8().data());
			if (rx.indexIn(line) > -1) {
				QString name = rx.cap(1);
				QString accelText = rx.cap(2);
				qDebug(" name: '%s' accel: '%s'", name.toUtf8().data(), accelText.toUtf8().data());
				row = findActionName(name);
				if (row > -1) {
					qDebug("Action found!");
					actionsTable->item(row, COL_SHORTCUT)->setText(accelText);
				}				
			} else {
				qDebug(" wrong line");
			}
		}
		f.close();
		hasConflicts(); // Check for conflicts

#if !USE_SHORTCUTGETTER
		dont_validate = false;
#endif

		return true;
	} else {
		return false;
	}
}


// Static functions

void ActionsEditor::saveToConfig(QObject *o, QSettings *set) {
	qDebug("ActionsEditor::saveToConfig");

	set->beginGroup("actions");

	QAction *action;
	QList<QAction *> actions = o->findChildren<QAction *>();
	for (int n=0; n < actions.count(); n++) {
		action = static_cast<QAction*> (actions[n]);
		if (!action->objectName().isEmpty()) {
#if USE_MULTIPLE_SHORTCUTS
			QString accelText = shortcutsToString(action->shortcuts());
#else
			QString accelText = action->shortcut().toString();
#endif
			set->setValue(action->objectName(), accelText);
		}
    }

	set->endGroup();
}


void ActionsEditor::loadFromConfig(QObject *o, QSettings *set) {
	qDebug("ActionsEditor::loadFromConfig");

	set->beginGroup("actions");

	QAction *action;
	QString accelText;

	QList<QAction *> actions = o->findChildren<QAction *>();
	for (int n=0; n < actions.count(); n++) {
		action = static_cast<QAction*> (actions[n]);
		if (!action->objectName().isEmpty()) {
#if USE_MULTIPLE_SHORTCUTS
			QString current = shortcutsToString(action->shortcuts());
			accelText = set->value(action->objectName(), current).toString();
			action->setShortcuts( stringToShortcuts( accelText ) );
#else
			accelText = set->value(action->objectName(), action->shortcut().toString()).toString();
			action->setShortcut(QKeySequence(accelText));
#endif
		}
    }

	set->endGroup();
}

QAction * ActionsEditor::findAction(QObject *o, const QString & name) {
	QAction *action;

	QList<QAction *> actions = o->findChildren<QAction *>();
	for (int n=0; n < actions.count(); n++) {
		action = static_cast<QAction*> (actions[n]);
		if (name == action->objectName()) return action;
    }

	return 0;
}

QStringList ActionsEditor::actionsNames(QObject *o) {
	QStringList l;

	QAction *action;

	QList<QAction *> actions = o->findChildren<QAction *>();
	for (int n=0; n < actions.count(); n++) {
		action = static_cast<QAction*> (actions[n]);
		//qDebug("action name: '%s'", action->objectName().toUtf8().data());
		//qDebug("action name: '%s'", action->text().toUtf8().data());
		if (!action->objectName().isEmpty())
			l.append( action->objectName() );
    }

	return l;
}


// Language change stuff
void ActionsEditor::changeEvent(QEvent *e) {
	if (e->type() == QEvent::LanguageChange) {
		retranslateStrings();
	} else {
		QWidget::changeEvent(e);
	}
}

#include "moc_actionseditor.cpp"

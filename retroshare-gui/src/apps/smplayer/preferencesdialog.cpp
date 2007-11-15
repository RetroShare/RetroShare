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


#include "preferencesdialog.h"

#include "prefwidget.h"
#include "prefgeneral.h"
#include "prefdrives.h"
#include "prefinterface.h"
#include "prefperformance.h"
#include "prefinput.h"
#include "prefsubtitles.h"
#include "prefadvanced.h"

#if USE_ASSOCIATIONS
#include "prefassociations.h"
#endif

#include "preferences.h"

#include <QVBoxLayout>
#include <QTextBrowser>

#include "images.h"

PreferencesDialog::PreferencesDialog(QWidget * parent, Qt::WindowFlags f)
	: QDialog(parent, f )
{
	setupUi(this);

	// Setup buttons
	okButton = buttonBox->button(QDialogButtonBox::Ok);
	cancelButton = buttonBox->button(QDialogButtonBox::Cancel);
	applyButton = buttonBox->button(QDialogButtonBox::Apply);
	helpButton = buttonBox->button(QDialogButtonBox::Help);
	connect( applyButton, SIGNAL(clicked()), this, SLOT(apply()) );
	connect( helpButton, SIGNAL(clicked()), this, SLOT(showHelp()) );
	

	setWindowIcon( Images::icon("logo") );

	help_window = new QTextBrowser(this);
	help_window->setWindowFlags(Qt::Window);
	help_window->resize(300, 450);
	//help_window->adjustSize();
	help_window->setWindowTitle( tr("SMPlayer - Help") );
	help_window->setWindowIcon( Images::icon("logo") );

	page_general = new PrefGeneral;
	addSection( page_general );

	page_drives = new PrefDrives;
	addSection( page_drives );

	page_performance = new PrefPerformance;
	addSection( page_performance );

	page_subtitles = new PrefSubtitles;
	addSection( page_subtitles );

	page_interface = new PrefInterface;
	addSection( page_interface );

	page_input = new PrefInput;
	addSection( page_input );

#if USE_ASSOCIATIONS
	page_associations = new PrefAssociations;
	addSection(page_associations);
#endif

	page_advanced = new PrefAdvanced;
	addSection( page_advanced );

	sections->setCurrentRow(General);

	//adjustSize();
	retranslateStrings();
}

PreferencesDialog::~PreferencesDialog()
{
}

void PreferencesDialog::showSection(Section s) {
	qDebug("PreferencesDialog::showSection: %d", s);

	sections->setCurrentRow(s);
}

void PreferencesDialog::retranslateStrings() {
	retranslateUi(this);

	for (int n=0; n < pages->count(); n++) {
		PrefWidget * w = (PrefWidget*) pages->widget(n);
		sections->item(n)->setText( w->sectionName() );
		sections->item(n)->setIcon( w->sectionIcon() );
	}

	if (help_window->isVisible()) {
		// Makes the help to retranslate
		showHelp();
	}

	help_window->setWindowTitle( tr("SMPlayer - Help") );

	// Qt 4.2 doesn't update the buttons' text
#if QT_VERSION < 0x040300
	okButton->setText( tr("OK") );
	cancelButton->setText( tr("Cancel") );
	applyButton->setText( tr("Apply") );
	helpButton->setText( tr("Help") );
#endif
}

void PreferencesDialog::accept() {
	hide();
	help_window->hide();
	setResult( QDialog::Accepted );
	emit applied();
}

void PreferencesDialog::apply() {
	setResult( QDialog::Accepted );
	emit applied();
}

void PreferencesDialog::reject() {
	hide();
	help_window->hide();
	setResult( QDialog::Rejected );

	setResult( QDialog::Accepted );
}

void PreferencesDialog::addSection(PrefWidget *w) {
	QListWidgetItem *i = new QListWidgetItem( w->sectionIcon(), w->sectionName() );
	sections->addItem( i );
	pages->addWidget(w);
}

void PreferencesDialog::setData(Preferences * pref) {
	page_general->setData(pref);
	page_drives->setData(pref);
	page_interface->setData(pref);
	page_performance->setData(pref);
	page_input->setData(pref);
	page_subtitles->setData(pref);
	page_advanced->setData(pref);

#if USE_ASSOCIATIONS
	page_associations->setData(pref);
#endif
}

void PreferencesDialog::getData(Preferences * pref) {
	page_general->getData(pref);
	page_drives->getData(pref);
	page_interface->getData(pref);
	page_performance->getData(pref);
	page_input->getData(pref);
	page_subtitles->getData(pref);
	page_advanced->getData(pref);

#if USE_ASSOCIATIONS
	page_associations->getData(pref);
#endif
}

bool PreferencesDialog::requiresRestart() {
	bool need_restart = page_general->requiresRestart();
	if (!need_restart) need_restart = page_drives->requiresRestart();
	if (!need_restart) need_restart = page_interface->requiresRestart();
	if (!need_restart) need_restart = page_performance->requiresRestart();
	if (!need_restart) need_restart = page_input->requiresRestart();
	if (!need_restart) need_restart = page_subtitles->requiresRestart();
	if (!need_restart) need_restart = page_advanced->requiresRestart();

	return need_restart;
}

void PreferencesDialog::showHelp() {
	PrefWidget * w = (PrefWidget*) pages->currentWidget();
	help_window->setHtml( w->help() );
	help_window->show();
	help_window->raise();
}

// Language change stuff
void PreferencesDialog::changeEvent(QEvent *e) {
	if (e->type() == QEvent::LanguageChange) {
		retranslateStrings();
	} else {
		QWidget::changeEvent(e);
	}
}

#include "moc_preferencesdialog.cpp"

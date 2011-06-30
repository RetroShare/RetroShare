/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2006-2008 Ricardo Villalba <rvm@escomposlinux.org>

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


#include "subchooserdialog.h"

static Qt::CheckState CurItemCheckState = Qt::Unchecked; 

SubChooserDialog::SubChooserDialog(QWidget * parent, Qt::WindowFlags f) 
	: QDialog(parent, f )
{
	setupUi(this);

	listWidget->setSelectionMode(QAbstractItemView::NoSelection);

	connect(selectAll, SIGNAL(clicked(bool)), this, SLOT(selectAllClicked(bool)));
	connect(selectNone, SIGNAL(clicked(bool)), this, SLOT(selectNoneClicked(bool)));
	connect(listWidget, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(listItemClicked(QListWidgetItem*))); 
	connect(listWidget, SIGNAL(itemPressed(QListWidgetItem*)), this, SLOT(listItemPressed(QListWidgetItem*))); 
}

SubChooserDialog::~SubChooserDialog()
{

}

void SubChooserDialog::selectAllClicked(bool) {
	for (int k = 0; k < listWidget->count(); k++)
		listWidget->item(k)->setCheckState(Qt::Checked);

	listWidget->setFocus(); 
}

void SubChooserDialog::selectNoneClicked(bool) {
	for (int k = 0; k < listWidget->count(); k++)
		listWidget->item(k)->setCheckState(Qt::Unchecked);

	listWidget->setFocus(); 
}

void SubChooserDialog::listItemClicked(QListWidgetItem* item) {
	if (item->checkState() == CurItemCheckState)
	{
		//Clicked on the list item (not checkbox)
		if (item->checkState() == Qt::Checked)
		{
			item->setCheckState(Qt::Unchecked);
		}
		else
			item->setCheckState(Qt::Checked); 
	}

	//else - clicked on the checkbox itself, do nothing
}

void SubChooserDialog::listItemPressed(QListWidgetItem* item) {
	CurItemCheckState = item->checkState(); 
}

void SubChooserDialog::addFile(QString filename) {
	QListWidgetItem* item = new QListWidgetItem(listWidget); 
	item->setText(filename);
	item->setCheckState(Qt::Unchecked);
}

QStringList SubChooserDialog::selectedFiles() {
	QStringList files;

	for (int n = 0; n < listWidget->count(); n++) {
		QListWidgetItem * i = listWidget->item(n); 
		if (i && i->checkState() == Qt::Checked) {
			files.append(i->text()); 
		}
	}

	return files;
}

void SubChooserDialog::retranslateStrings() {
	retranslateUi(this);
}

// Language change stuff
void SubChooserDialog::changeEvent(QEvent *e) {
	if (e->type() == QEvent::LanguageChange) {
		retranslateStrings();
	} else {
		QDialog::changeEvent(e);
	}
}

#include "moc_subchooserdialog.cpp"


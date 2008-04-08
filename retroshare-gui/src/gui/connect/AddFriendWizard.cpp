/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006, crypton
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

#include "AddFriendWizard.h"
//#include "rshare.h"
//#include "config/gconfig.h"

#include "rsiface/rsiface.h"
#include "rsiface/rspeers.h"

#include <sstream>

#include <QContextMenuEvent>
#include <QCursor>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPixmap>
#include <QPoint>

#include "gui/NetworkDialog.h"

/** Constructor */
AddFriendWizard::AddFriendWizard(NetworkDialog *cd, QWidget *parent, Qt::WFlags flags)
: QDialog(parent, flags), cDialog(cd)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui.setupUi(this);
  
	//GConfig config;
	//config.loadWidgetInformation(this);

    connect(ui.loadfileButton, SIGNAL(clicked()), this, SLOT(loadfile()));


    setFixedSize(QSize(508, 312));

  /* Hide platform specific features */
#ifdef Q_WS_WIN

#endif
}

/*!
* The destructor for AddFriendWizard
*/
AddFriendWizard::~AddFriendWizard()
{
}


void AddFriendWizard::reset(QSettings *settingsPointer)
{
    // set the labelstackedWidget and textstackedWidget to the first position
    //labelstackedWidget->setCurrentIndex(0);
    ui.textstackedWidget->setCurrentIndex(0);
    // disable the backButton: We don't need it if we are on the first position
    ui.backButton->setEnabled(false);                                                   
    // and we aren't at the last step, were the next button becomes the finish button
    lastStep = false;
    
    settings = settingsPointer;
    
}


void AddFriendWizard::on_nextButton_clicked()
{
    // The current index position
    int index = ui.textstackedWidget->currentIndex();
    // Test the different widgets
    switch ( ui.textstackedWidget->currentIndex()) {
        // 
        case 0 : 
        {
            if ( ui.keyradioButton->isChecked() )
            {
                ui.textstackedWidget->setCurrentWidget(ui.keypage); 

            }
            if ( ui.pqipemradioButton->isChecked() )
            {
                ui.textstackedWidget->setCurrentWidget(ui.pqipempage);
  
            }
            if ( ui.inviteradioButton->isChecked() )
            {
                ui.textstackedWidget->setCurrentWidget(ui.invitepage);
  
            }
  
        }
        return;
       
    }

    /*
    * move to the next widget, with the signal/slot also labelstackedWidget,
    * it's also moved
    */
    index++;
    // activate the backButton, because we are at the second widget or higher
    ui.backButton->setEnabled(true);

    
}

/*!
* The on_backButton_clicked function is called when the back button is clicked.
* First it set the variable index to the currentIndex of textstackedWidget minus 1.
* So we have the previous position. Then it tests if the index goes lower then 0.
* Because of there is no value prior 0, it sets the index back to 0.
* If we was at the last widget, the button was named "Finish". If we go back, it's
* named "Next" again and the lastStep is set to false.
*/
void AddFriendWizard::on_backButton_clicked()
{
    int index = ui.textstackedWidget->currentIndex() - 1;                                   
    if ( index <= 0 )
    {
        index = 0;
        ui.backButton->setEnabled(false);
    }
    //ui.nextButton->setText(tr("Next"));
    lastStep = false;
    ui.textstackedWidget->setCurrentIndex(0);
}

/*!
* This function closes the dialog without saving the values
*/
void AddFriendWizard::on_cancelButton_clicked()
{
    // if cancel is pressed, use the standard settings
    //writeSettings();
    // leave but show that cancel was pressed
    reject();
}

void AddFriendWizard::loadfile()
{

	/* show file dialog, 
	 * load file into screen, 
	 * push done button!
	 */
        std::string id;
	if (cDialog)
	{
		id = cDialog->loadneighbour();
	}
					  
	/* call make Friend */
	if (id != "")
	{
		close();
		cDialog->showpeerdetails(id);
	}
	else
	{
		/* error message */
		int ret = QMessageBox::warning(this, tr("RetroShare"),
                   tr("Certificate Load Failed"),
                   QMessageBox::Ok, QMessageBox::Ok);
	}
}

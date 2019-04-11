/*******************************************************************************
 * gui/QuikStartWizard.h                                                       *
 *                                                                             *
 * Copyright (c) 2006 Retroshare Team  <retroshare.project@gmail.com>          *
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

#ifndef _QUICKSTARTWIZARD_H
#define _QUICKSTARTWIZARD_H

#include <QDialog>
#include <QWizard>

#include "ui_QuickStartWizard.h"


class QuickStartWizard : public QDialog 
{
    Q_OBJECT
    Q_DISABLE_COPY(QuickStartWizard)
    
public:
    explicit QuickStartWizard(QWidget *parent = 0);
    virtual ~QuickStartWizard();
    
    void loadNetwork();
    void loadShare();    
    void loadGeneral();

protected:
    virtual void changeEvent(QEvent *e);
   // virtual void showEvent(QShowEvent * event);

private:
    Ui::QuickStartWizard ui;
    
    bool messageBoxOk(QString);

private Q_SLOTS:
        void on_shareIncomingDirectory_clicked();
        void on_pushButtonSharesRemove_clicked();
        void on_pushButtonSharesAdd_clicked();
        void on_pushButtonSystemExit_clicked();
        void on_pushButtonSystemFinish_clicked();
        void on_pushButtonSystemBack_clicked();
        void on_pushButtonSharesExit_clicked();
        void on_pushButtonSharesNext_clicked();
        void on_pushButtonSharesBack_clicked();
        void on_pushButtonStyleExit_clicked();
        void on_pushButtonStyleNext_clicked();
        void on_pushButtonStyleBack_clicked();
        void on_pushButtonWelcomeExit_clicked();
        void on_pushButtonWelcomeNext_clicked();
        void on_pushButtonConnectionExit_clicked();
        void on_pushButtonConnectionNext_clicked();
        void on_pushButtonConnectionBack_clicked();
	
	void updateFlags(bool);
	void saveChanges();
        //void toggleUPnP();
        //void toggleTunnelConnection(bool) ;



};

#endif // _QUICKSTARTWIZARD_H

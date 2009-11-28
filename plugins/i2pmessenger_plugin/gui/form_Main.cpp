/***************************************************************************
 *   Copyright (C) 2008 by normal   *
 *   normal@Desktop2   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
 
#include <QSystemTrayIcon>

#include "form_Main.h"



form_MainWindow::form_MainWindow(QWidget* parent)
    : QMainWindow(parent){
	setupUi(this); // this sets up GUI
	
        //this->setAttribute(Qt::WA_DeleteOnClose,true);
	
	initStyle();
	
	//initTryIconMenu();
	//initTryIcon();
    	init();
}
void form_MainWindow::init()
{
	using namespace SESSION_Types;

	Core= new cCore();
	fillComboBox();
	initToolBars();
	applicationIsClosing=false;
	
	Mute=false;
	

	QListWidget* listWidget=this->listWidget;

	connect (Core,SIGNAL(eventUserChanged()),this,
		SLOT(eventUserChanged()));

	connect(Core,SIGNAL(eventOnlineStatusChanged()),this,
		SLOT(OnlineStateChanged()));

	connect(listWidget,SIGNAL(itemDoubleClicked( QListWidgetItem* )),this,
		SLOT(openChatDialog ()));

	connect(listWidget, SIGNAL( customContextMenuRequested(QPoint)), this,
		SLOT( connecttreeWidgetCostumPopupMenu(QPoint)));

	connect(comboBox,SIGNAL(currentIndexChanged( int)),this,
		SLOT(onlineComboBoxChanged()));

	this->eventUserChanged();
}

form_MainWindow::~form_MainWindow(){
	
	delete Core;
	//delete trayIcon;
	//applicationIsClosing=true;
	//this->close();
}


void form_MainWindow::fillComboBox()
{
	QComboBox* comboBox = this->comboBox;
	comboBox->addItem(QIcon(ICON_USER_TRYTOCONNECT)	,"TryToConnect");
	comboBox->addItem(QIcon(ICON_USER_OFFLINE)	,"Offline");
}

void form_MainWindow::onlineComboBoxChanged()
{
	QComboBox* comboBox= this->comboBox;
	QString text=comboBox->currentText();

	if(text.contains("Online",Qt::CaseInsensitive)==true){
		if(Core->getOnlineStatus()!=User::USERONLINE) 
			Core->setOnlineStatus(User::USERONLINE);
	}
	else if(text.contains("WantToChat",Qt::CaseInsensitive)==true){
		if(Core->getOnlineStatus()!=User::USERWANTTOCHAT) 
			Core->setOnlineStatus(User::USERWANTTOCHAT);
	}
	else if(text.contains("Away",Qt::CaseInsensitive)==true){
		if(Core->getOnlineStatus()!=User::USERAWAY) 
			Core->setOnlineStatus(User::USERAWAY);
	}
	else if(text.contains("don't disturb",Qt::CaseInsensitive)==true){
		if(Core->getOnlineStatus()!=User::USERDONT_DISTURB) 
			Core->setOnlineStatus(User::USERDONT_DISTURB);
	}
	else if(text.contains("Invisible",Qt::CaseInsensitive)==true){
		if(Core->getOnlineStatus()!=User::USERINVISIBLE) 
			Core->setOnlineStatus(User::USERINVISIBLE);
	}
	else if(text.contains("Offline",Qt::CaseInsensitive)==true){
		if(Core->checkIfAFileTransferOrReciveisActive()==false){
			if(Core->getOnlineStatus()!=User::USEROFFLINE) 
				Core->setOnlineStatus(User::USEROFFLINE);
		}
		else{
			QMessageBox* msgBox= new QMessageBox(NULL);
			msgBox->setIcon(QMessageBox::Information);
			msgBox->setText("");
			msgBox->setInformativeText("Sorry a Filetransfer or Filerecive ist active,\nClosing aborted");
			msgBox->setStandardButtons(QMessageBox::Ok);
			msgBox->setDefaultButton(QMessageBox::Ok);
			msgBox->setWindowModality(Qt::NonModal);
			msgBox->show();
			OnlineStateChanged();

		}
	}
	else if(text.contains("TryToConnect",Qt::CaseInsensitive)==true){
		if(Core->getOnlineStatus()!=User::USERTRYTOCONNECT) 
			Core->setOnlineStatus(User::USERTRYTOCONNECT);
	}

}

void form_MainWindow::initToolBars()
{
	//toolBar->setIconSize(QSize(24, 24));
	QToolBar* toolBar=this->toolBar;
	
	
	toolBar->setMovable(false);
	toolBar->addAction(QIcon(ICON_NEWUSER)		,"add User"	,this,SLOT(openAdduserWindow() ) );
	toolBar->addAction(QIcon(ICON_SETTINGS)		,"Settings"	,this,SLOT(openConfigWindow() ) );
	toolBar->addAction(QIcon(ICON_DEBUGMESSAGES)	,"DebugMessages",this,SLOT(openDebugMessagesWindow() ) );
	toolBar->addAction(QIcon(ICON_MYDESTINATION)	,"ME"		,this,SLOT(namingMe()));
	//toolBar->addAction(QIcon(ICON_CLOSE)		,"Close"	,this,SLOT(closeApplication()));
	toolBar->addAction(QIcon(ICON_ABOUT)		,"About"	,this,SLOT(openAboutDialog()));
}


void form_MainWindow::openConfigWindow(){
	
	form_settingsgui* dialog= new form_settingsgui();
	connect(this,SIGNAL(closeAllWindows()),dialog,
	SLOT(close()));	

	dialog->show();
	
}
void form_MainWindow::openAdduserWindow(){
	form_newUserWindow* dialog= new form_newUserWindow(Core);

	connect(this,SIGNAL(closeAllWindows()),dialog,
	SLOT(close()));	
	
	dialog->show();
}

void form_MainWindow::openDebugMessagesWindow(){
	form_DebugMessages* dialog= new form_DebugMessages(this->Core);

	connect(this,SIGNAL(closeAllWindows()),dialog,
		SLOT(close()));	

	dialog->show();
}

void form_MainWindow::namingMe(){
	QClipboard *clipboard = QApplication::clipboard();
	QString Destination=Core->getMyDestination();
	if(Destination!=""){
		clipboard->setText(Destination);
		QMessageBox::information(this, "",
                        "Your Destination is in the clipboard",QMessageBox::Close);
	}
	else
		QMessageBox::information(this, "",
                	"Your Client must be Online for that",QMessageBox::Close);
}
void form_MainWindow::closeApplication(){

	if(Core->checkIfAFileTransferOrReciveisActive()==false){

		emit closeAllWindows();
	
		delete Core;
		delete trayIcon;
		applicationIsClosing=true;
		this->close();
	}
	else{

		QMessageBox* msgBox= new QMessageBox(NULL);
		msgBox->setIcon(QMessageBox::Information);
		msgBox->setText("");
		msgBox->setInformativeText("Sorry a Filetransfer or Filerecive ist active,\nClosing aborted");
		msgBox->setStandardButtons(QMessageBox::Ok);
		msgBox->setDefaultButton(QMessageBox::Ok);
		msgBox->setWindowModality(Qt::NonModal);
		msgBox->show();

	}
	


}
void form_MainWindow::eventUserChanged(){

	bool showUnreadMessageAtTray=false;
	QList<cUser*> users=Core->get_userList();
	listWidget->clear();

	
	for(int i=0;i<users.count();i++){	
		QListWidgetItem* newItem= new QListWidgetItem(listWidget);
		QListWidgetItem* ChildWidthI2PDestinationAsText= new QListWidgetItem(listWidget);
		
		if(users.at(i)->getHaveNewUnreadMessages()==true){
			newItem->setIcon(QIcon(ICON_NEWUNREADMESSAGE));
			showUnreadMessageAtTray=true;
			
		}
		else
		switch(users.at(i)->get_OnlineState())
		{
			
			case USERTRYTOCONNECT:
					{
						newItem->setIcon(QIcon(ICON_USER_OFFLINE));
						break;
					}
			case USERINVISIBLE:
			case USEROFFLINE:
					{
						newItem->setIcon(QIcon(ICON_USER_OFFLINE));
						break;
					}
			case USERONLINE:	
					{
						newItem->setIcon(QIcon(ICON_USER_ONLINE));	
						break;
					}
			case USERWANTTOCHAT:
					{
						newItem->setIcon(QIcon(ICON_USER_WANTTOCHAT));	
						break;
					}
			case USERAWAY:
					{
						newItem->setIcon(QIcon(ICON_USER_AWAY));	
						break;
					}
			case USERDONT_DISTURB:
					{
						newItem->setIcon(QIcon(ICON_USER_DONT_DUSTURB));	
						break;
					}


		}
		newItem->setText(users.at(i)->get_Name());
		newItem->setTextAlignment(Qt::AlignLeft);
     		newItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		ChildWidthI2PDestinationAsText->setText(users.at(i)->get_I2PDestination());
		ChildWidthI2PDestinationAsText->setHidden(true);//DEBUG
	}

		/*if(showUnreadMessageAtTray==false)
			trayIcon->setIcon(QIcon(ICON_QTCHAT));
		else
			trayIcon->setIcon(QIcon(ICON_NEWUNREADMESSAGE));*/
}

//void form_MainWindow::UserDoubleClicked(QListWidgetItem * item)
void form_MainWindow::openChatDialog()
{	
	QListWidgetItem *t=listWidget->item(listWidget->currentRow()+1);
	QString Destination =t->text();
		
	cUser* User;
	User=Core->getUserByI2P_Destination(Destination);
	if(User==NULL)return;

	if(User->getHaveAllreadyOneChatWindow()==false){
		form_ChatWidget* tmp= new form_ChatWidget(User);
	
		connect(this,SIGNAL(closeAllWindows()),tmp,
			SLOT(close()));	
	
		eventUserChanged();
		
		tmp->show();
	}
	else{
		QMessageBox::information(this, "",
                          "Only one Chatwindows for each User",QMessageBox::Close);
	}
}
void form_MainWindow::connecttreeWidgetCostumPopupMenu(QPoint point){
	QListWidget* listWidget=this->listWidget;

	if(listWidget->count()==0)return;

	QMenu contextMnu( this );
      	QMouseEvent *mevent = new QMouseEvent( QEvent::MouseButtonPress, point, Qt::RightButton, 
				Qt::RightButton, Qt::NoModifier );

	
	QAction* UserChat = new QAction("Chat",this);
		connect( UserChat , SIGNAL(triggered()),this, SLOT( openChatDialog()));

	
	QAction* UserDelete = new QAction("Delete",this);
		connect( UserDelete , SIGNAL(triggered()),this, SLOT( deleteUserClicked()));
	
	QAction* UserRename = new QAction("Rename",this);
		connect(UserRename,SIGNAL(triggered()),this, SLOT(userRenameCLicked()));
	
	QAction* CopyDestination = new QAction("Copy Destination",this);
		connect(CopyDestination,SIGNAL(triggered()),this, SLOT(copyDestination()));
	


	contextMnu.clear();
	contextMnu.addAction(UserChat);

	QListWidgetItem *t=listWidget->item(listWidget->currentRow()+1);
	QString Destination =t->text();
		
	cUser* User;
	User=Core->getUserByI2P_Destination(Destination);

	if(User->get_ConnectionStatus()==ONLINE)
	{
		QAction* UserSendFile = new QAction("SendFile",this);
		connect(UserSendFile,SIGNAL(triggered()),this, SLOT(SendFile()));
		contextMnu.addAction(UserSendFile);
	}

	contextMnu.addSeparator();
	contextMnu.addAction(UserRename);
	contextMnu.addAction(UserDelete);
	contextMnu.addAction(CopyDestination);

	contextMnu.exec( mevent->globalPos());
}

void form_MainWindow::deleteUserClicked(){
	QListWidgetItem *t=listWidget->item(listWidget->currentRow()+1);
	QString Destination =t->text();

	Core->deleteUserByI2PDestination(Destination);
}

void form_MainWindow::userRenameCLicked(){
	QListWidgetItem *t=listWidget->item(listWidget->currentRow());
	QString OldNickname=t->text();
	
	QListWidgetItem *t2= listWidget->item(listWidget->currentRow()+1);
	QString Destination =t2->text();

	form_RenameWindow* Dialog= new form_RenameWindow(Core,OldNickname,Destination);
	Dialog->show();
}

/*void form_MainWindow::closeEvent(QCloseEvent *e)
{
    static bool firstTime = true;
	if(applicationIsClosing==true) return;

    if (trayIcon->isVisible()) {
        if (firstTime)
        {

            firstTime = false;
        }
        hide();
        e->ignore();
    }

}

void form_MainWindow::updateMenu()
{
    toggleVisibilityAction->setText(isVisible() ? tr("Hide") : tr("Show"));
}

void form_MainWindow::toggleVisibility(QSystemTrayIcon::ActivationReason e)
{
    if(e == QSystemTrayIcon::Trigger || e == QSystemTrayIcon::DoubleClick){
        if(isHidden()){
            show();
 	    
	    //eventUserChanged();
	
            if(isMinimized()){
                if(isMaximized()){
                    showMaximized();
                }else{
                    showNormal();
                }
            }
            raise();
            activateWindow();
        }else{
            hide();
        }
    }
}

void form_MainWindow::toggleVisibilitycontextmenu()
{
    if (isVisible())
        hide();
    else
        show();
}*/

void form_MainWindow::OnlineStateChanged()
{
	QComboBox* comboBox = this->comboBox;
	if(Core->getOnlineStatus()==User::USERONLINE)
	{
		comboBox->clear();
		comboBox->addItem(QIcon(ICON_USER_ONLINE)	, "Online");		//index 0
		comboBox->addItem(QIcon(ICON_USER_WANTTOCHAT)	, "WantToChat");	//1
		comboBox->addItem(QIcon(ICON_USER_AWAY)		, "Away");		//2
		comboBox->addItem(QIcon(ICON_USER_DONT_DUSTURB)	, "don't disturb");	//3
		comboBox->addItem(QIcon(ICON_USER_INVISIBLE)	, "Invisible");		//4
		comboBox->addItem(QIcon(ICON_USER_OFFLINE)	, "Offline");		//5

		
	}
	else if(Core->getOnlineStatus()==User::USEROFFLINE){	
		comboBox->clear();
		comboBox->addItem(QIcon(ICON_USER_OFFLINE)	, "Offline");
		comboBox->addItem(QIcon(ICON_USER_TRYTOCONNECT)	, "TryToConnect");
		comboBox->setCurrentIndex(0);
	}	
	else if(Core->getOnlineStatus()==User::USERWANTTOCHAT){
		comboBox->setCurrentIndex(1);
	}
	else if(Core->getOnlineStatus()==User::USERAWAY){
		comboBox->setCurrentIndex(2);
	}
	else if(Core->getOnlineStatus()==User::USERDONT_DISTURB){
		comboBox->setCurrentIndex(3);
	}
	else if(Core->getOnlineStatus()==User::USERINVISIBLE){
		comboBox->setCurrentIndex(4);
	}
}

void form_MainWindow::openAboutDialog()
{
	form_HelpDialog* dialog = new form_HelpDialog(Core->get_ClientVersion(),Core->get_ProtocolVersion());
	
	connect(this,SIGNAL(closeAllWindows()),dialog,
	SLOT(close()));	

	dialog->show();
}

void form_MainWindow::initStyle()
{
        QSettings * settings=new QSettings(QApplication::applicationDirPath()+"/application.ini",QSettings::IniFormat);
	settings->beginGroup("General");
	//Load Style
		QString Style=(settings->value("current_Style","")).toString();
		if(Style.isEmpty()==true)
		{
			//find default Style for this System
			QRegExp regExp("Q(.*)Style");
			Style = QApplication::style()->metaObject()->className();
			
			if (Style == QLatin1String("QMacStyle"))
				Style = QLatin1String("Macintosh (Aqua)");
			else if (regExp.exactMatch(Style))
				Style = regExp.cap(1);
			
			//styleCombo->addItems(QStyleFactory::keys());
		}
		
		qApp->setStyle(Style);
	//Load Style end

	//Load Stylesheet	
		QFile file(QApplication::applicationDirPath() + "/qss/" + 
			settings->value("current_Style_sheet","Default").toString() + ".qss");
		
		file.open(QFile::ReadOnly);
		QString styleSheet = QLatin1String(file.readAll());
		qApp->setStyleSheet(styleSheet);
	//load Stylesheet end
	settings->endGroup();

    delete settings;
}

/*void form_MainWindow::initTryIconMenu()
{
	// Tray icon Menu
	menu = new QMenu(this);
	QObject::connect(menu, SIGNAL(aboutToShow()), this, SLOT(updateMenu()));
	toggleVisibilityAction = 
		menu->addAction(QIcon(ICON_QTCHAT), tr("Show/Hide"), this, SLOT(toggleVisibilitycontextmenu()));	

	toggleMuteAction=
		menu->addAction(QIcon(ICON_SOUND_ON), tr("Sound on"),this,SLOT(muteSound()));
	menu->addSeparator();
	//menu->addAction(QIcon(ICON_MINIMIZE), tr("Minimize"), this, SLOT(showMinimized()));
	//menu->addAction(QIcon(ICON_MAXIMIZE), tr("Maximize"), this, SLOT(showMaximized()));
	menu->addSeparator();
	menu->addAction(QIcon(ICON_CLOSE), tr("&Quit"), this, SLOT(closeApplication()));

	

}

void form_MainWindow::initTryIcon()
{
	// Create the tray icon
	trayIcon = new QSystemTrayIcon(this);
	trayIcon->setToolTip(tr("I2PChat"));
	trayIcon->setContextMenu(menu);
	trayIcon->setIcon(QIcon(ICON_QTCHAT));
    
   	 connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, 
        	SLOT(toggleVisibility(QSystemTrayIcon::ActivationReason)));
    	trayIcon->show();
}*/

void form_MainWindow::SendFile()
{
	QString FilePath=QFileDialog::getOpenFileName(this,"Open File", ".", "all Files (*.*)");
	
	QListWidgetItem *t=listWidget->item(listWidget->currentRow()+1);
	QString Destination =t->text();
	
	if(!FilePath.isEmpty())
		Core->addNewFileTransfer(FilePath,Destination);
	
}

void form_MainWindow::copyDestination()
{
	QListWidgetItem *t=listWidget->item(listWidget->currentRow()+1);
	QString Destination =t->text();
	
	QClipboard *clipboard = QApplication::clipboard();
	
		clipboard->setText(Destination);
		QMessageBox::information(this, "",
                        "The Destination is in the clipboard",QMessageBox::Close);
	
}

void form_MainWindow::muteSound()
{
	if(this->Mute==false)
	{
		toggleMuteAction->setIcon(QIcon(ICON_SOUND_OFF));
		toggleMuteAction->setText("Sound off");
		Mute=true;
	}
	else
	{
		toggleMuteAction->setIcon(QIcon(ICON_SOUND_ON));
		toggleMuteAction->setText("Sound on");
		Mute=false;

	}
	Core->MuteSound(Mute);
}

#include "form_chatwidget.h"
#include "src/User.h"

bool ChatEventEater::eventFilter(QObject *obj, QEvent *event)
{
	if ( event->type() == QEvent::KeyPress )
	{
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if ( obj->objectName() == "message")
        {
        	if ( keyEvent->key() == Qt::Key_Return && keyEvent->modifiers() == Qt::NoModifier)
        	{
        		emit sendMessage();
        		return true;
        	}
        	else if (keyEvent->key() == Qt::Key_Return 
        			&& keyEvent->modifiers() == Qt::ControlModifier)
        	{
        		emit sendMessage();
        		return true;
        	}
	}
	return QObject::eventFilter(obj, event);
	}

	return false;
}


form_ChatWidget::form_ChatWidget(cUser* user,QWidget* parent /* = 0 */)
: QWidget(parent)
{

	setupUi(this);
	this->setAttribute(Qt::WA_DeleteOnClose,true);

	this->user=user;

	user->set_HaveAllreadyOneChatWindow(true);

	QTextEdit *message=this->message;

	m_event_eater = new ChatEventEater(this);
    	connect(m_event_eater, SIGNAL(sendMessage()),
    		send, SLOT(click()));

	message->installEventFilter(m_event_eater);

	connect(user,SIGNAL(newMessageRecived()),this,
		SLOT(newMessageRecived()));

	connect(user,SIGNAL(OnlineStateChanged()),this,
		SLOT(changeWindowsTitle()));

	connect(this,SIGNAL(sendChatMessage(QString)),user,
		SLOT(sendChatMessage(QString)));

	connect(message,SIGNAL(cursorPositionChanged()),this,
		SLOT(WorkAround()));	

		
  	mCurrentFont = user->get_textFont();
	textColor = user->get_textColor();


	QPixmap pxm(24,24);
	pxm.fill(textColor);
	txtColor->setIcon(pxm);
	
	
	connect(send, SIGNAL(clicked()), SLOT(sendMessageSignal()));
	connect(txtColor, SIGNAL(clicked()), SLOT(setTextColor()));
	connect(txtBold, SIGNAL(clicked(bool)),SLOT(setBold(bool)));
	connect(txtFont, SIGNAL(clicked()), SLOT(setFont()));
	
	message->setTextColor(textColor);
	message->setCurrentFont(mCurrentFont);


	resize(450,400);
	changeWindowsTitle();
	newMessageRecived();
	
}

void form_ChatWidget::newMessageRecived(){
	QTextEdit *chat=this->chat;
	chat->clear();

	QStringList Messages=user->get_ChatMessages();
	int i=0;	
	while(i<Messages.count()){
		QString test =Messages.at(i);
		this->addMessage(test);
		i++;
	}
}


void form_ChatWidget::addMessage(QString text){
	QTextEdit *chat=this->chat;

	chat->insertHtml(text);

	QTextCursor cursor = chat->textCursor();
	cursor.movePosition(QTextCursor::End);
	chat->setTextCursor(cursor);
}	

void form_ChatWidget::setTextColor(){
	QTextEdit *message=this->message;
	textColor = QColorDialog::getColor(Qt::black, this);
	
	//textColor = QColorDialog::getColor(message->textColor(), this);
	QPixmap pxm(24,24);
	pxm.fill(textColor);
	txtColor->setIcon(pxm);
	user->set_textColor(textColor);
	
	message->setTextColor(textColor);
	

}

void form_ChatWidget::setFont()
{
	QTextEdit *message=this->message;
    	bool ok;
    	mCurrentFont = QFontDialog::getFont(&ok, mCurrentFont, this);
    	
	user->set_textFont(mCurrentFont);
	message->setCurrentFont(mCurrentFont);
	message->setFocus();
}


void form_ChatWidget::setBold(bool t){

	QTextEdit *message=this->message;
	QFont font = message->currentFont();
	font.setBold(t);
	user->set_textFont(mCurrentFont);
	message->setCurrentFont(font);

}

void form_ChatWidget::closeEvent(QCloseEvent *e){
	user->set_HaveAllreadyOneChatWindow(false);
	disconnect(user,SIGNAL(newMessageRecived()),this,
		SLOT(newMessageRecived()));
}

void form_ChatWidget::sendMessageSignal(){
	QTextEdit *message=this->message;
	if(message->toPlainText().length()==0)return;

	emit sendChatMessage(message->toHtml());
	message->clear();

	//message->document()->clear();

}

void form_ChatWidget::WorkAround()
{
	QTextEdit *message=this->message;
	if(message->textCursor().position()>1)return;
	else if(message->textCursor().position()==1 && message->toPlainText().length()==1)
	{
		message->selectAll();
		
		
		//message->textCursor().setPosition(QTextCursor::Start);
		//message->textCursor().setPosition(QTextCursor::Left);
		//message->textCursor().select(QTextCursor::Document);
		//message->textCursor().movePosition(QTextCursor::End,QTextCursor::KeepAnchor);

		message->setTextColor(textColor);
		message->setCurrentFont(mCurrentFont);

		message->moveCursor(QTextCursor::End,QTextCursor::MoveAnchor);
		message->textCursor().clearSelection();
	}

}


void form_ChatWidget::changeWindowsTitle()
{
	QString OnlineStatus;
	QString OnlineStatusIcon;
	switch(user->get_OnlineState())
		{
			
			case USERTRYTOCONNECT:
			case USERINVISIBLE:
			case USEROFFLINE:{
						OnlineStatus="offline";
						this->setWindowIcon(QIcon(ICON_USER_OFFLINE));
						break;
					}
			case USERONLINE:	
					{
						OnlineStatus="online";
						this->setWindowIcon(QIcon(ICON_USER_ONLINE));
						break;
					}
			case USERWANTTOCHAT:
					{
						OnlineStatus="want to chat";
						this->setWindowIcon(QIcon(ICON_USER_WANTTOCHAT));
						break;
					}
			case USERAWAY:
					{
						OnlineStatus="away";
						this->setWindowIcon(QIcon(ICON_USER_AWAY));	
						break;
					}
			case USERDONT_DISTURB:
					{
						OnlineStatus="don't disturb";
						this->setWindowIcon(QIcon(ICON_USER_DONT_DUSTURB));
						break;
					}


		}



	this->setWindowTitle("Chat with: "+user->get_Name()+"       ("+ OnlineStatus +")");
}


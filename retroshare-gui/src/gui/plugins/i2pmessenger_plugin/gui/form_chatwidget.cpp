#include "form_chatwidget.h"
#include "src/User.h"



form_ChatWidget::form_ChatWidget(cUser* user,QWidget* parent /* = 0 */)
: QWidget(parent)
{

	setupUi(this);
	this->setAttribute(Qt::WA_DeleteOnClose,true);

	this->user=user;
	user->set_HaveAllreadyOneChatWindow(true);
	
	connect(user,SIGNAL(newMessageRecived()),this,
		SLOT(newMessageRecived()));

	connect(this,SIGNAL(sendChatMessage(QString)),user,
		SLOT(sendChatMessage(QString)));

	
		
  	mCurrentFont = QFont("Comic Sans MS", 10);	
	
	textColor = Qt::black;
	QPixmap pxm(24,24);
	pxm.fill(textColor);
	txtColor->setIcon(pxm);
	
	connect(send, SIGNAL(clicked()), SLOT(sendMessageSignal()));
	connect(txtColor, SIGNAL(clicked()), SLOT(setTextColor()));
	connect(txtBold, SIGNAL(clicked(bool)),SLOT(setBold(bool)));
	connect(txtFont, SIGNAL(clicked()), SLOT(setFont()));
	
/*
	message->setStyleSheet(
	"QTextEdit {"
	" selection-color: white;"
	" selection-background-color: black;"
	" color: red" // text color
	"}"
 	);
*/

	resize(450,400);
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
	
	message->setTextColor(textColor);
	

}

void form_ChatWidget::setFont()
{
	QTextEdit *message=this->message;
    	bool ok;
    	mCurrentFont = QFontDialog::getFont(&ok, mCurrentFont, this);
    	
	
	message->setCurrentFont(mCurrentFont);
	message->setFocus();
}


void form_ChatWidget::setBold(bool t){

	QTextEdit *message=this->message;
	QFont font = message->currentFont();
	font.setBold(t);
	message->setCurrentFont(font);

}

void form_ChatWidget::closeEvent(QCloseEvent *e){
	user->set_HaveAllreadyOneChatWindow(false);
	disconnect(user,SIGNAL(newMessageRecived()),this,
		SLOT(newMessageRecived()));
}

void form_ChatWidget::sendMessageSignal(){
	QTextEdit *message=this->message;

	QFont font = message->currentFont();	
	QColor color= message->textColor();	
	
	
	emit sendChatMessage(message->toHtml());
	//message->setHtml("");

	message->setText(" ");
	message->selectAll();
	message->setTextColor(color);
	message->setCurrentFont(font);


}

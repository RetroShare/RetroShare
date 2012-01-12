#include "ChatTabWidget.h"
#include "ui_ChatTabWidget.h"
#include "PopupChatDialog.h"
#include "gui/common/StatusDefs.h"

#define IMAGE_WINDOW         ":/images/rstray3.png"
#define IMAGE_TYPING         ":/images/typing.png"
#define IMAGE_CHAT           ":/images/chat.png"

ChatTabWidget::ChatTabWidget(QWidget *parent) :
	QTabWidget(parent),
	ui(new Ui::ChatTabWidget)
{
	ui->setupUi(this);

    connect(this, SIGNAL(tabCloseRequested(int)), this, SLOT(tabClose(int)));
    connect(this, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));
}

ChatTabWidget::~ChatTabWidget()
{
	delete ui;
}

void ChatTabWidget::addDialog(PopupChatDialog *dialog)
{
	addTab(dialog, dialog->getTitle());

	QObject::connect(dialog, SIGNAL(infoChanged(PopupChatDialog*)), this, SLOT(tabInfoChanged(PopupChatDialog*)));

	tabInfoChanged(dialog);
}

void ChatTabWidget::removeDialog(PopupChatDialog *dialog)
{
	QObject::disconnect(dialog, SIGNAL(infoChanged(PopupChatDialog*)), this, SLOT(tabInfoChanged(PopupChatDialog*)));

	int tab = indexOf(dialog);
	if (tab >= 0) {
		removeTab(tab);
	}
}

void ChatTabWidget::tabClose(int tab)
{
	PopupChatDialog *dialog = dynamic_cast<PopupChatDialog*>(widget(tab));

	if (dialog) {
		if (dialog->canClose()) {
			dialog->deleteLater();
		}
	}
}

void ChatTabWidget::tabChanged(int tab)
{
	PopupChatDialog *dialog = dynamic_cast<PopupChatDialog*>(widget(tab));

	if (dialog) {
		dialog->activate();
		emit tabChanged(dialog);
	}
}

void ChatTabWidget::tabInfoChanged(PopupChatDialog *dialog)
{
	int tab = indexOf(dialog);
	if (tab >= 0) {
		if (dialog->isTyping()) {
			setTabIcon(tab, QIcon(IMAGE_TYPING));
		} else if (dialog->hasNewMessages()) {
			setTabIcon(tab, QIcon(IMAGE_CHAT));
		} else if (dialog->hasPeerStatus()) {
			setTabIcon(tab, QIcon(StatusDefs::imageIM(dialog->getPeerStatus())));
		} else {
			setTabIcon(tab, QIcon());
		}
	}

	emit infoChanged();
}

void ChatTabWidget::getInfo(bool &isTyping, bool &hasNewMessage, QIcon *icon)
{
	isTyping = false;
	hasNewMessage = false;

	PopupChatDialog *pcd;
	int tabCount = count();
	for (int i = 0; i < tabCount; i++) {
		pcd = dynamic_cast<PopupChatDialog*>(widget(i));
		if (pcd) {
			if (pcd->isTyping()) {
				isTyping = true;
			}
			if (pcd->hasNewMessages()) {
				hasNewMessage = true;
			}
		}
	}

	if (icon) {
		if (isTyping) {
			*icon = QIcon(IMAGE_TYPING);
		} else if (hasNewMessage) {
			*icon = QIcon(IMAGE_CHAT);
		} else {
			pcd = dynamic_cast<PopupChatDialog*>(currentWidget());
			if (pcd && pcd->hasPeerStatus()) {
				*icon = QIcon(StatusDefs::imageIM(pcd->getPeerStatus()));
			} else {
				*icon = QIcon();
			}
		}
	}
}

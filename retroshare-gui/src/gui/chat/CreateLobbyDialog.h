#ifndef CREATELOBBYDIALOG_H
#define CREATELOBBYDIALOG_H

#include <QDialog>

#include "ui_CreateLobbyDialog.h"

class CreateLobbyDialog : public QDialog {
	Q_OBJECT
public:
	/*
	 *@param chanId The channel id to send request for
	 */
	CreateLobbyDialog(const std::list<std::string>& friends_list,QWidget *parent = 0, Qt::WFlags flags = 0);
	~CreateLobbyDialog();

protected:
	void changeEvent(QEvent *e);

private:
	Ui::CreateLobbyDialog *ui;

private slots:
	void createLobby();
	void checkTextFields();
	void cancel();
};

#endif // CREATELOBBYDIALOG_H

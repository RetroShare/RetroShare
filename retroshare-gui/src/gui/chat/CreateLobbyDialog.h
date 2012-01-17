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
	CreateLobbyDialog(const std::list<std::string>& friends_list,QWidget *parent = 0, Qt::WFlags flags = 0, std::string grpId = "", int grpType = 0);
	~CreateLobbyDialog();

protected:
	void changeEvent(QEvent *e);

private:
	void setShareList(const std::list<std::string>&);

	Ui::CreateLobbyDialog *ui;

	std::string mGrpId;
	std::list<std::string> mShareList;
	int mGrpType;

private slots:
	void createLobby();
	void checkTextFields();
	void cancel();
	void togglePersonItem(QTreeWidgetItem* item, int col);
};

#endif // CREATELOBBYDIALOG_H

/****************************************************************************
***************************Author: Agnit Sarkar******************************
**************************CopyRight: April 2009******************************
********************* Email: agnitsarkar@yahoo.co.uk*************************
****************************************************************************/
#ifndef STEGOSAURUS_H
#define STEGOSAURUS_H

#include <QtGui>
#include "ui_stegosaurus.h"

class StegoSaurus : public QDialog
{
	Q_OBJECT

public:
	StegoSaurus(QWidget *parent = 0, Qt::WFlags flags = 0);
	~StegoSaurus();

protected:
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);

private:
	void resizeEvent ( QResizeEvent *);
	Ui::StegoSaurusClass ui;
	QImage *dryImage, *wetImage;
	unsigned int bitsAvailable, inputFileSize, sizeofData;

	void setDryImage(const QString &);
	void setWetImage(const QString &);

private slots:
	
	void on_btnWetImg_clicked();
	void on_btnFiletoHide_clicked();
	void on_btnDryImg_clicked();

	//The hide method
	void on_btnHide_clicked();
	//The recover method
	void on_btnRecover_clicked();
};

#endif // STEGOSAURUS_H

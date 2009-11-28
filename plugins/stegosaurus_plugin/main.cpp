/****************************************************************************
***************************Author: Agnit Sarkar******************************
**************************CopyRight: April 2009******************************
********************* Email: agnitsarkar@yahoo.co.uk*************************
****************************************************************************/
#include <QtGui/QApplication>
#include "stegosaurus.h"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	StegoSaurus w;
	w.show();
	return a.exec();
}

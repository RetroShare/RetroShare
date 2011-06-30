#include <QFile>
#include <QRegExp>
#include <stdio.h>

int main( int argc, char ** argv )
{
	QFile file("list.txt");
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return -1;

	QRegExp rx("^([a-zA-Z]+) ([a-zA-Z\\-]+)");

	QString line;
	while (!file.atEnd()) {
		line = QString(file.readLine()).simplified();
		if (!line.isEmpty()) {
			//qDebug("%s", line.toLatin1().constData());
			if (rx.indexIn(line) > -1) {
				QString s1 = rx.cap(1);
				QString s2 = rx.cap(2);
				//qDebug("code: %s, language: %s", s1.toLatin1().constData(), s2.toLatin1().constData());
				printf("\tl[\"%s\"] = tr(\"%s\");\n", s1.toLatin1().constData(), s2.toLatin1().constData());
			}
		}
	}
	file.close();

	return 0;
}


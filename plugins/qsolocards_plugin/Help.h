/*
    QSoloCards is a collection of Solitaire card games written using Qt
    Copyright (C) 2009  Steve Moore

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef HELP_H
#define HELP_H

#include <QtGui/QDialog>
#include <QtCore/QString>
#include <QtGui/QTextBrowser>

class Help: public QDialog
{
    Q_OBJECT
public:
    Help(QWidget * pParent,const QString & helpFile);
    ~Help();

    void setHelpFile(const QString &);

private:

    QTextBrowser *  m_pBrowser;
};

#endif // HELP_H

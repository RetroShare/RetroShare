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

#include "Help.h"

#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtGui/QHBoxLayout>

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
Help::Help(QWidget * pParent,const QString & helpFile)
        :QDialog(pParent),
        m_pBrowser(NULL)
{
    this->setWindowTitle(tr("QSoloCards:  Game Help").trimmed());

    this->setAttribute(Qt::WA_DeleteOnClose,true);

    m_pBrowser=new QTextBrowser;

    QHBoxLayout * pLayout=new QHBoxLayout;

    pLayout->addWidget(m_pBrowser,20);

    this->setLayout(pLayout);

    this->setHelpFile(helpFile);
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
Help::~Help()
{
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
void Help::setHelpFile(const QString & helpFile)
{
    QFile file(helpFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return;
    }

    QTextStream in(&file);
    QString line = in.readLine();

    QString text;

    while (!line.isNull())
    {
        text.append(line);
        text.append(" ");
        line = in.readLine();
    }

    m_pBrowser->setHtml(text);
}


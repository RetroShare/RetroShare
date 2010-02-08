/* ColorCode, a free MasterMind clone with built in solver
 * Copyright (C) 2009  Dirk Laebisch
 * http://www.laebisch.com/
 *
 * ColorCode is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ColorCode is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ColorCode. If not, see <http://www.gnu.org/licenses/>.
*/

#include "about.h"

About::About(QWidget* parent, Qt::WindowFlags f) : QDialog(parent, f)
{
    setupUi(this);
    setWindowIcon(QIcon(QPixmap(":/img/help-about.png")));
    mAuthorIcon->setPixmap(QPixmap(":/img/cc32.png"));
    mLicenseIcon->setPixmap(QPixmap(":/img/GNU-icon32.png"));

    mAuthorText->setText("<b>" + tr("ColorCode") + "</b><br>" +
                         tr("A needful game to train your brain ;-)") +
                         tr("<br><br>Free MasterMind clone including a built in,<br>rather intelligent solver.") +
                         "<br><br>" + tr("Version") + ": 0.5.5<br>" + tr("Author") + ": Dirk Laebisch");

    QString license_file = ":/docs/GPL.html";
    if (QFile::exists(license_file))
    {
        QFont fixed_font;
        fixed_font.setStyleHint(QFont::TypeWriter);
        fixed_font.setFamily("Courier");
        mLicenseText->setFont(fixed_font);

        QFile f(license_file);
        if (f.open(QIODevice::ReadOnly))
        {
            mLicenseText->setText(QString::fromUtf8(f.readAll().constData()));
        }
        f.close();
    }
    else
    {
        mLicenseText->setText(
        "<i>" +
        tr("This program is free software; you can redistribute it and/or modify "
        "it under the terms of the GNU General Public License as published by "
        "the Free Software Foundation; either version 2 of the License, or "
        "(at your option) any later version.") + "</i>"
        );
    }
}

About::~About()
{
}

QSize About::sizeHint () const
{
    return QSize(360, 270);
}

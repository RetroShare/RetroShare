/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006 - 2009,  RetroShre Team
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/
#include "NewTag.h"

#include <QColorDialog>

/** Default constructor */
NewTag::NewTag(std::map<int, TagItem> &Items, int nId /*= 0*/, QWidget *parent, Qt::WFlags flags)
  : QDialog(parent, flags), m_Items(Items)
{
    /* Invoke Qt Designer generated QObject setup routine */
    ui.setupUi(this);

    m_nId = nId;

    connect(ui.okButton, SIGNAL(clicked()), this, SLOT(OnOK()));
    connect(ui.cancelButton, SIGNAL(clicked()), this, SLOT(OnCancel()));
    connect(ui.colorButton, SIGNAL(clicked()), this, SLOT(setTagColor()));

    connect(ui.lineEdit, SIGNAL(textChanged(const QString &)), this, SLOT(textChanged(const QString &)));

    ui.okButton->setEnabled(false);

    if (m_nId) {
        TagItem &Item = m_Items [m_nId];
        ui.lineEdit->setText(Item.text);
        m_Color = Item.color;

        if (m_nId < 0) {
            // standard tag
            ui.lineEdit->setEnabled(false);
        }
    } else {
        m_Color = 0;
    }

    showColor (m_Color);
}

void NewTag::closeEvent (QCloseEvent * event)
{
    QDialog::closeEvent(event);
}

void NewTag::OnOK()
{
    TagItem Item;
    Item.text = ui.lineEdit->text();
    Item.color = m_Color;

    if (m_nId == 0) {
        // calculate new id
        m_nId = 1;
        std::map<int, TagItem>::iterator Item;
        for (Item = m_Items.begin(); Item != m_Items.end(); Item++) {
            if (Item->first + 1 > m_nId) {
                m_nId = Item->first + 1;
            }
        }
    }

    m_Items [m_nId] = Item;

    setResult(QDialog::Accepted);
    hide();
}

void NewTag::OnCancel()
{
    setResult(QDialog::Rejected);
    hide();
}

void NewTag::textChanged(const QString &text)
{
    bool bEnabled = true;

    if (text.isEmpty()) {
        bEnabled = false;
    } else {
        // check for existing text
        std::map<int, TagItem>::iterator Item;
        for (Item = m_Items.begin(); Item != m_Items.end(); Item++) {
            if (m_nId && Item->first == m_nId) {
                continue;
            }

            if (Item->second._delete) {
                continue;
            }

            if (Item->second.text == text) {
                bEnabled = false;
                break;
            }
        }
    }

    ui.okButton->setEnabled(bEnabled);
}

void NewTag::setTagColor()
{
    bool ok;
    QRgb color = QColorDialog::getRgba(m_Color, &ok, this);
    if (ok) {
        m_Color = color;
        showColor (m_Color);
    }
}

void NewTag::showColor(QRgb color)
{
    QPixmap pxm(16,16);
    pxm.fill(QColor(m_Color));
    ui.colorButton->setIcon(pxm);
}

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

#include "retroshare/rsmsgs.h"

#include <QColorDialog>

/** Default constructor */
NewTag::NewTag(MsgTagType &Tags, uint32_t nId /* = 0*/, QWidget *parent, Qt::WindowFlags flags)
  : QDialog(parent, flags), m_Tags(Tags)
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
        std::map<uint32_t, std::pair<std::string, uint32_t> >::iterator Tag;
        Tag = m_Tags.types.find(m_nId);
        if (Tag != m_Tags.types.end()) {
            ui.lineEdit->setText(QString::fromStdString(Tag->second.first));
            m_Color = QRgb(Tag->second.second);

            if (m_nId < RS_MSGTAGTYPE_USER) {
                // standard tag
                ui.lineEdit->setEnabled(false);
            }
        } else {
            // tag id not found
            m_Color = 0;
            m_nId = 0;
        }
    } else {
        m_Color = 0;
    }

    showColor (m_Color);
}

void NewTag::OnOK()
{
    if (m_nId == 0) {
        // calculate new id
        m_nId = RS_MSGTAGTYPE_USER;
        std::map<uint32_t, std::pair<std::string, uint32_t> >::iterator Tag;
        for (Tag = m_Tags.types.begin(); Tag != m_Tags.types.end(); Tag++) {
            if (Tag->first + 1 > m_nId) {
                m_nId = Tag->first + 1;
            }
        }
    }

    std::pair<std::string, uint32_t> newTag(ui.lineEdit->text().toStdString(), m_Color);
    m_Tags.types [m_nId] = newTag;

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
    std::string stdText = text.toStdString();

    if (text.isEmpty()) {
        bEnabled = false;
    } else {
        // check for existing text
        std::map<uint32_t, std::pair<std::string, uint32_t> >::iterator Tag;
        for (Tag = m_Tags.types.begin(); Tag != m_Tags.types.end(); Tag++) {
            if (m_nId && Tag->first == m_nId) {
                // its me
                continue;
            }

            if (Tag->second.first.empty()) {
                // deleted
                continue;
            }

            if (Tag->second.first == stdText) {
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
    pxm.fill(QColor(color));
    ui.colorButton->setIcon(pxm);
}

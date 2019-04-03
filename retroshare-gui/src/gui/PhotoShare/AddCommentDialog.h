/*******************************************************************************
 * retroshare-gui/src/gui/PhotoShare/AddCommentDialog.h                        *
 *                                                                             *
 * Copyright (C) 2018 by Retroshare Team     <retroshare.project@gmail.com>    *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#ifndef ADDCOMMENTDIALOG_H
#define ADDCOMMENTDIALOG_H

#include <QDialog>

namespace Ui {
    class AddCommentDialog;
}

class AddCommentDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddCommentDialog(QWidget *parent = 0);
    ~AddCommentDialog();
    QString getComment() const;

private:
    Ui::AddCommentDialog *ui;
};

#endif // ADDCOMMENTDIALOG_H

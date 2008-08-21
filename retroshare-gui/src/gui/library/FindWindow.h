/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2008, defnax
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

 #ifndef FINDWINDOW_H
 #define FINDWINDOW_H

 #include <QDialog>

 class QComboBox;
 class QDir;
 class QLabel;
 class QPushButton;
 class QTableWidget;

 class FindWindow : public QDialog
 {
     Q_OBJECT

 public:
     FindWindow(QWidget *parent = 0);

 private slots:
     void browse();
     void find();

 private:
     QStringList findFiles( QDir &directory,  QStringList &files,
                            QString &text);
     void showFiles(const QDir &directory, const QStringList &files);
     void showFolders(const QDir &directory, const QStringList &folders);
     QPushButton *createButton(const QString &text, const char *member);
     QComboBox *createComboBox(const QString &text = QString());
     void createFilesTable();

     QComboBox *fileComboBox;
     QComboBox *textComboBox;
     QComboBox *directoryComboBox;
     QLabel *fileLabel;
     QLabel *textLabel;
     QLabel *directoryLabel;
     QLabel *filesFoundLabel;
     QPushButton *browseButton;
     QPushButton *findButton;
     QTableWidget *filesTable;
 };
#endif

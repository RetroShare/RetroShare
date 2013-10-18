/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2008 Robert Fernie
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

#ifndef _CREATEBLOGMSG_H
#define _CREATEBLOGMSG_H

#include <QSettings>

#include "ui_CreateBlogMsg.h"
#include <stdint.h>

class SubFileItem;
class FileInfo;

QT_FORWARD_DECLARE_CLASS(QAction)
QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QFontComboBox)
QT_FORWARD_DECLARE_CLASS(QTextEdit)
QT_FORWARD_DECLARE_CLASS(QTextCharFormat)
QT_FORWARD_DECLARE_CLASS(QMenu)

class CreateBlogMsg : public QMainWindow
{
  Q_OBJECT

public:
  /** Default Constructor */
  CreateBlogMsg(std::string cId, QWidget *parent = 0, Qt::WindowFlags flags = 0);
  /** Default Destructor */

	void addAttachment(std::string path);
	void addAttachment(std::string hash, std::string fname, uint64_t size, 
	bool local, std::string srcId);

	void newBlogMsg();
	
	QPixmap picture;
  QSettings setter;

  void  Create_New_Image_Tag( const QString urlremoteorlocal );

private slots:

	void cancelMsg();
	void sendMsg();
	void addImage();
	
	void fontSizeIncrease();
  void fontSizeDecrease();
  void blockQuote();
  void toggleCode();
  void addPostSplitter();
  
  void setStartupText();
  void updateTextEdit();
  
  void fileNew();
  void fileOpen();
  bool fileSave();
  bool fileSaveAs();
  void filePrint();
  void filePrintPreview();
  void filePrintPdf();
  void printPreview(QPrinter *);
  
  void textBold();
  void textUnderline();
  void textItalic();
  void textFamily(const QString &f);
  void textSize(const QString &p);
  void changeFormatType(int styleIndex );

  
  void textColor();
  void textAlign(QAction *a);
  
  void addOrderedList();
  void addUnorderedList();
  
  void currentCharFormatChanged(const QTextCharFormat &format);
  void cursorPositionChanged();

  void clipboardDataChanged();


private:
	void setupFileActions();
	void setupEditActions();
	void setupViewActions();
	void setupInsertActions();
	void setupParagraphActions();
	void setupTextActions();

  void setCurrentFileName(const QString &fileName);
  bool load(const QString &f);
  bool maybeSave();

  void mergeFormatOnWordOrSelection(const QTextCharFormat &format);
  
  void fontChanged(const QFont &f);
  void colorChanged(const QColor &c);
  void alignmentChanged(Qt::Alignment a);


  void sendMessage(std::wstring subject, std::wstring msg);
  
  std::string mBlogId;
	
	  QAction *actionSave,
	      *actionTextBold,
        *actionTextUnderline,
        *actionTextItalic,		    
        *actionTextColor,
        *actionAlignLeft,
        *actionAlignCenter,
        *actionAlignRight,
        *actionAlignJustify,
        *actionUndo,
        *actionRedo,
        *actionCut,
        *actionCopy,
        *actionPaste;
        
    QComboBox *comboStyle;
    QFontComboBox *comboFont;
    QComboBox *comboSize;
    
  QString fileName;
	
	QColor codeBackground;
    QTextCharFormat defaultCharFormat;
    QTextBlockFormat defaultBlockFormat;
    QTextCharFormat lastCharFormat;
    QTextBlockFormat lastBlockFormat;
    
  /** Qt Designer generated object */
  Ui::CreateBlogMsg ui;
};



#endif


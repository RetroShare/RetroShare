/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2007, RetroShare Team
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

#ifndef _MESSAGECOMPOSER_H
#define _MESSAGECOMPOSER_H

#include <QMainWindow>
#include <QMap>
#include <QPointer>
#include <gui/settings/rsharesettings.h>
#include "gui/feeds/AttachFileItem.h"

#include "ui_MessageComposer.h"
#include "rsiface/rsfiles.h"

class QAction;
class QComboBox;
class QFontComboBox;
class QTextEdit;
class QTextCharFormat;

class MessageComposer : public QMainWindow 
{
  Q_OBJECT

public:
  /** Default Constructor */

  MessageComposer(QWidget *parent = 0, Qt::WFlags flags = 0);

  static void msgFriend(std::string id);

  void  newMsg(std::string msgId = "");

	/* worker fns */
  void  insertSendList();
  void  insertFileList(const std::list<DirDetails>&);
  void  insertFileList(const std::list<FileInfo>&);
  void  insertTitleText(std::string title);
  void  insertPastedText(std::string msg) ;
  void  insertForwardPastedText(std::string msg);
  void  insertHtmlText(std::string msg);
  void  insertMsgText(std::string msg);
  void  addRecipient(std::string id) ;
  void  Create_New_Image_Tag( const QString urlremoteorlocal );
 QSettings setter;

public slots:

	/* actions to take.... */
  void  sendMessage();
  void  cancelMessage();
  void  addImage();
    
  void changeFormatType(int styleIndex );


protected:
  void closeEvent (QCloseEvent * event);

private slots:
  
  /* toggle Contacts DockWidget */
  void toggleContacts();

	/* for toggling flags */
  void togglePersonItem( QTreeWidgetItem *item, int col );
  void toggleRecommendItem( QTreeWidgetItem *item, int col );
  
  void fileNew();
  void fileOpen();
  bool fileSave();
  bool fileSaveAs();
  void filePrint();
  void saveasDraft(); 

  //void filePrintPreview();
  void filePrintPdf();
  
  void textBold();
  void textUnderline();
  void textItalic();
  void textFamily(const QString &f);
  void textSize(const QString &p);
  void textStyle(int styleIndex);
  void textColor();
  void textAlign(QAction *a);

  void currentCharFormatChanged(const QTextCharFormat &format);
  void cursorPositionChanged();
  
  void clipboardDataChanged();
  
  void fileHashingFinished(AttachFileItem* file);

	void attachFile();
	void addAttachment(std::string);
	void checkAttachmentReady();

  void fontSizeIncrease();
  void fontSizeDecrease();
  void blockQuote();
  void toggleCode();
  void addPostSplitter();

  void filterRegExpChanged();
  void clearFilter();
  void titleChanged();

private:
  void setTextColor(const QColor& col) ;
	void setupFileActions();
	void setupEditActions();
	void setupViewActions();
	void setupInsertActions();
	void setupFormatActions();
	
	bool load(const QString &f);
	bool maybeSave();
	//bool image_extension( QString nametomake );
	void setCurrentFileName(const QString &fileName);

    void mergeFormatOnWordOrSelection(const QTextCharFormat &format);
    void fontChanged(const QFont &f);
    void colorChanged(const QColor &c);
    void alignmentChanged(Qt::Alignment a);

    void  sendMessage_internal(bool bDraftbox);
    
  void FilterItems();
  bool FilterItem(QTreeWidgetItem *pItem, QString &sPattern);

  void calculateTitle();

   /** Define the popup menus for the Context menu */
  QMenu* contextMnu;
  
   /** Defines the actions for the context menu */
  QAction* deletechannelAct;
  QAction* createchannelmsgAct;
  
  QAction *actionSave,
		*actionAlignLeft,
        *actionAlignCenter,
        *actionAlignRight,
        *actionAlignJustify,
        *actionUndo,
        *actionRedo,
        *actionCut,
        *actionCopy,
        *actionPaste;

  QTreeView *channelstreeView;
  
  QString fileName;
  QString nametomake;
  
  QColor codeBackground;
  QTextCharFormat defaultCharFormat;
  
  QHash<QString, QString> autoLinkDictionary;
  QHash<QString, QString> autoLinkTitleDictionary;
  QHash<QString, int> autoLinkTargetDictionary;

  std::string m_sMsgId; // existing message id
  int m_nBox;           // box from the existing message

  /* maps of files  */
  std::list<AttachFileItem *> mAttachments;

  bool mCheckAttachment;

  /** Qt Designer generated object */
  Ui::MessageComposer ui;

  std::list<FileInfo> _recList ;
};

#endif


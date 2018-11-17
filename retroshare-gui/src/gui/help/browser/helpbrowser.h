/*******************************************************************************
 * retroshare-gui/src/gui/help/browser/helpbrowser.h                           *
 *                                                                             *
 * Copyright (c) 2008, defnax          <retroshare.project@gmail.com>          *
 * Copyright (c) 2008, Matt Edman, Justin Hipple                               *
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

/*
** \file helpbrowser.h
** \version $Id: helpbrowser.h 2362 2008-02-29 04:30:11Z edmanm $ 
** \brief Displays a list of help topics and content
*/

#ifndef _HELPBROWSER_H
#define _HELPBROWSER_H

#include <QMainWindow>
#include <QCloseEvent>
#include <QDomDocument>
#include <QDomElement>
#include <QDomNodeList>
#include <QTreeWidgetItem>
#include <QTextBrowser>
#include <QTextCursor>
#include <gui/common/rwindow.h>

#include "ui_helpbrowser.h"

class HelpBrowser : public RWindow
{
  Q_OBJECT

protected:
  /** Default constructor **/
  HelpBrowser(QWidget *parent = 0);
  virtual ~HelpBrowser();

public:
  /** Overrides the default QWidget::show() */
  static void showWindow(const QString &topic);

private slots:
  /** Called when the user clicks "Find Next" */
  void findNext();
  /** Called when the user clicks "Find Previous" */
  void findPrev();
  /** Called when the user starts a search */
  void search();
  /** Called when the user selects a different item in the contents tree */
  void contentsItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *prev);
  /** Called when the user selects a different item in the search tree */
  void searchItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *prev);
   
private:
  /** Returns the language in which help topics should appear, or English
   * ("en") if no translated help files exist for the current GUI language. */
  QString language();
  /** Load the contents of the help topics tree from the specified XML file. */
  void loadContentsFromXml(QString xmlFile);
  /** Load the contents of the help topics tree from the given DOM document. */
  bool loadContents(const QDomDocument *document, QString &errorString);
  /** Parse a Topic element and handle all its children. */
  void parseHelpTopic(const QDomElement &element, QTreeWidgetItem *parent);
  /** Returns true if the given Topic element has the necessary attributes. */
  bool isValidTopicElement(const QDomElement &topicElement);
  /** Builds a resource path to an html file associated with a help topic. */
  QString getResourcePath(const QDomElement &topicElement);
  /** Searches the current page for the phrase in the Find box */
  void find(bool forward);
  /** Creates a new item to be placed in the topic tree. */
  QTreeWidgetItem* createTopicTreeItem(const QDomElement &topicElement,
                                       QTreeWidgetItem *parent);
  /** Called when the user selects a different item in the tree. */
  void currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *prev);
  /** Finds a topic in the topic tree. */
  QTreeWidgetItem* findTopicItem(QTreeWidgetItem *startItem, QString topic);
  /** Shows the help browser and finds a specific a topic in the browser. */
  void showTopic(QString topic);

  /** List of DOM elements representing topics. */
  QList<QDomElement> _elementList;
  /** Last phrase used for 'Find' */
  QString _lastFind;
  /** Last phrase searched on */
  QString _lastSearch;
  /** Indicates if phrase was previously found on current page */
  bool _foundBefore;

  /** Qt Designer generated QObject */
  Ui::HelpBrowser ui;
};

#endif
  

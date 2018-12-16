/*******************************************************************************
 * gui/common/vmessagebox.h                                                    *
 *                                                                             *
 * Copyright (c) 2008, defnax                                                  *
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
** \file vmessagebox.h
** \version $Id: vmessagebox.h 2362 2008-02-29 04:30:11Z edmanm $
** \brief Provides a custom Vidalia mesage box
*/

#ifndef _VMESSAGEBOX_H
#define _VMESSAGEBOX_H

#include <QMessageBox>
#include <QString>


class VMessageBox : public QMessageBox
{
  Q_OBJECT

public:
  enum Button {
    NoButton = 0,
    Ok,
    Cancel,
    Yes,
    No,
    Help,
    Retry,
    ShowLog,
    ShowSettings,
    Continue,
    Quit,
    Browse
  };
  
  /** Default constructor. */
  VMessageBox(QWidget *parent = 0);

  /** Displays an critical message box with the given caption, message text,
   * and visible buttons. To specify a button as a default button or an escape
   * button, OR the Button enum value with QMessageBox::Default or
   * QMessageBox::Escape, respectively. */
  static int critical(QWidget *parent, QString caption, QString text,
                        int button0, int button1 = NoButton, 
                        int button2 = NoButton);
  
  /** Displays an information message box with the given caption, message text,
   * and visible buttons. To specify a button as a default button or an escape
   * button, OR the Button enum value with QMessageBox::Default or
   * QMessageBox::Escape, respectively. */
  static int information(QWidget *parent, QString caption, QString text,
                            int button0, int button1 = NoButton, 
                            int button2 = NoButton);

  /** Displays a warning message box with the given caption, message text, and
   * visible buttons. To specify as a default button or an escape
   * button, OR the Button enum value with QMessageBox::Default or
   * QMessageBox::Escape, respectively. */
  static int warning(QWidget *parent, QString caption, QString text,
                        int button0, int button1 = NoButton, 
                        int button2 = NoButton);

  /** Displays a warning message box with the given caption, message text, and
   * visible buttons. To specify as a default button or an escape
   * button, OR the Button enum value with QMessageBox::Default or
   * QMessageBox::Escape, respectively. */
  static int question(QWidget *parent, QString caption, QString text,
                         int button0, int button1 = NoButton, 
                         int button2 = NoButton);
  
  /** Converts a Button enum value to a translated string. */
  static QString buttonText(int button);
  
private:
  /** Returns the button (0, 1, or 2) that is OR-ed with QMessageBox::Default,
   * or 0 if none are. */
  static int defaultButton(int button0, int button1, int button2);
  /** Returns the button (0, 1, or 2) that is OR-ed with QMessageBox::Escape,
   * or -1 if none are. */
  static int escapeButton(int button0, int button1, int button2);
  /** Returns the Button enum value from the given return value. */
  static int selected(int ret, int button0, int button1, int button2);
};

#endif


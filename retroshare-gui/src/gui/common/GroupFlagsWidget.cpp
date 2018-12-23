/*******************************************************************************
 * gui/common/GroupFlagsWidget.cpp                                             *
 *                                                                             *
 * Copyright (C) 2010, Retroshare Team <retroshare.project@gmail.com>          *
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

#include <QHBoxLayout>
#include <QSizePolicy>
#include "GroupFlagsWidget.h"
#include <retroshare/rsfiles.h>

#define FLAGS_ANONYMOUS_SEARCH_ON        ":icons/search_red_128.png"
#define FLAGS_ANONYMOUS_SEARCH_OFF       ":icons/blank_red_128.png"
#define FLAGS_BROWSABLE_ON               ":icons/browsable_green_128.png"
#define FLAGS_BROWSABLE_OFF              ":icons/blank_green_128.png"
#define FLAGS_ANONYMOUS_DL_ON            ":icons/anonymous_blue_128.png"
#define FLAGS_ANONYMOUS_DL_OFF           ":icons/blank_blue_128.png"

#define INDEX_ANON_SEARCH   0
#define INDEX_ANON_DL       1
#define INDEX_BROWSABLE     2

/*QString GroupFlagsWidget::_tooltips_on[4] = {
    QObject::tr("Directory is visible to friends"),
    QObject::tr("Directory can be search anonymously"),
    QObject::tr("Directory is accessible by anonymous tunnels")
};
QString GroupFlagsWidget::_tooltips_off[4] = {
    QObject::tr("Directory is not visible to friends"),
    QObject::tr("Directory cannot be searched anonymously"),
    QObject::tr("Directory is NOT accessible by anonymous tunnels")
};
*/
GroupFlagsWidget::GroupFlagsWidget(QWidget *parent,FileStorageFlags flags)
	: QWidget(parent)
{
	_layout = new QHBoxLayout(this) ;

    setMinimumSize(128 * QFontMetricsF(font()).height()/14.0,32 * QFontMetricsF(font()).height()/14.0) ;
    setMaximumSize(128 * QFontMetricsF(font()).height()/14.0,32 * QFontMetricsF(font()).height()/14.0) ;
	setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);

    _icons[2*INDEX_BROWSABLE+0]   = new QIcon(FLAGS_BROWSABLE_OFF) ;
    _icons[2*INDEX_BROWSABLE+1]    = new QIcon(FLAGS_BROWSABLE_ON) ;
    _icons[2*INDEX_ANON_SEARCH+0] = new QIcon(FLAGS_ANONYMOUS_SEARCH_OFF) ;
    _icons[2*INDEX_ANON_SEARCH+1]  = new QIcon(FLAGS_ANONYMOUS_SEARCH_ON) ;
    _icons[2*INDEX_ANON_DL+0]     = new QIcon(FLAGS_ANONYMOUS_DL_OFF) ;
    _icons[2*INDEX_ANON_DL+1]      = new QIcon(FLAGS_ANONYMOUS_DL_ON) ;

	setLayout(_layout) ;

    _flags[INDEX_BROWSABLE  ] = DIR_FLAGS_BROWSABLE ;
    _flags[INDEX_ANON_SEARCH] = DIR_FLAGS_ANONYMOUS_SEARCH ;
    _flags[INDEX_ANON_DL    ] = DIR_FLAGS_ANONYMOUS_DOWNLOAD ;

    for(int i=0;i<3;++i)
	{
		_buttons[i] = new QPushButton(this) ;
		_buttons[i]->setCheckable(true) ;
		_buttons[i]->setChecked(flags & _flags[i]) ;
        _buttons[i]->setIconSize(QSize(32 * QFontMetricsF(font()).height()/14.0,32 * QFontMetricsF(font()).height()/14.0));

		update_button_state(_buttons[i]->isChecked(),i) ;
		_layout->addWidget(_buttons[i]) ;
	}
		
    connect(_buttons[INDEX_ANON_DL    ],SIGNAL(toggled(bool)),this,SLOT(update_DL_button(bool))) ;
    connect(_buttons[INDEX_ANON_SEARCH],SIGNAL(toggled(bool)),this,SLOT(update_SR_button(bool))) ;
    connect(_buttons[INDEX_BROWSABLE  ],SIGNAL(toggled(bool)),this,SLOT(update_BR_button(bool))) ;

	_layout->setSpacing(0);
	_layout->setContentsMargins(0, 0, 0, 0);

	_layout->update() ;
}

void GroupFlagsWidget::updated() 
{
	emit flagsChanged(flags()) ;
}

FileStorageFlags GroupFlagsWidget::flags() const 
{
	FileStorageFlags flags ;

    for(int i=0;i<3;++i)
		if(_buttons[i]->isChecked()) flags |= _flags[i] ;

	return flags ;
}

void GroupFlagsWidget::setFlags(FileStorageFlags flags)
{
    for(int i=0;i<3;++i)
	{
		_buttons[i]->setChecked(flags & _flags[i]) ;
		update_button_state(_buttons[i]->isChecked(),i) ;
	}
}

void GroupFlagsWidget::update_button_state(bool b,int button_id)
{
  QString tip_on, tip_off;
  switch (button_id) {
    case INDEX_BROWSABLE:
      tip_on = tr("Directory content is visible to friend nodes (see list at right)");
      tip_off = tr("Directory content is NOT visible to friend nodes");
      break;
    case INDEX_ANON_SEARCH:
      tip_on = tr("Directory can be searched anonymously");
      tip_off = tr("Directory cannot be searched anonymously");
      break;
    case INDEX_ANON_DL:
      if(_buttons[INDEX_ANON_SEARCH]->isChecked())
          tip_on = tr("Files can be accessed using anonymous tunnels");
      else
          tip_on = tr("Files can be accessed using anonymous & end-to-end encrypted tunnels");

      tip_off = tr("Files cannot be downloaded anonymously");
      break;
    default:
      tip_on = "";
      tip_off = "";
  }
  _buttons[button_id]->setIcon(*_icons[2*button_id+(int)b]) ;
  _buttons[button_id]->setToolTip(b?tip_on:tip_off) ;
}

QString GroupFlagsWidget::groupInfoString(FileStorageFlags flags, const QList<QString>& groupNames)
{
	// makes a string that explains how files are shared / visible.
	
	QString res ;
	QString groups_string ;

	for(QList<QString>::const_iterator it(groupNames.begin());it!=groupNames.end();++it)
	{
		if(it != groupNames.begin())
			groups_string += ", " ;
		groups_string += *it ;
	}
	
    if(flags & DIR_FLAGS_BROWSABLE)
    {
        if(groupNames.empty())
            res += tr("All friend nodes can see this directory") + "\n" ;
        else
            res += tr("Only visible to friend nodes in groups: %1").arg(groups_string) + "\n" ;
    }
    else
        res += tr("Not visible to friend nodes") + "\n" ;

    if((flags & DIR_FLAGS_ANONYMOUS_DOWNLOAD) && !(flags & DIR_FLAGS_ANONYMOUS_SEARCH))
        res += tr("Files can be downloaded (but not searched) anonymously") ;
    else if((flags & DIR_FLAGS_ANONYMOUS_DOWNLOAD) && (flags & DIR_FLAGS_ANONYMOUS_SEARCH))
        res += tr("Files can be downloaded and searched anonymously") ;
    else if(!(flags & DIR_FLAGS_ANONYMOUS_DOWNLOAD) && (flags & DIR_FLAGS_ANONYMOUS_SEARCH))
        res += tr("Files can be searched (but not downloaded) anonymously") ;
    else
        res += tr("No one can anonymously access/search these files.") ;

    return res ;
}

void GroupFlagsWidget::update_DL_button(bool b) { update_button_state(b,INDEX_ANON_DL    ) ; updated() ; }
void GroupFlagsWidget::update_SR_button(bool b) { update_button_state(b,INDEX_ANON_SEARCH) ; updated() ; }
void GroupFlagsWidget::update_BR_button(bool b) { update_button_state(b,INDEX_BROWSABLE  ) ; updated() ; }

GroupFlagsWidget::~GroupFlagsWidget()
{
    for(int i=0;i<3;++i)
	{
		delete _buttons[i] ;
        delete _icons[2*i+0] ;
        delete _icons[2*i+1] ;
    }
}


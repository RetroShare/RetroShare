#include <QHBoxLayout>
#include <QSizePolicy>
#include "GroupFlagsWidget.h"
#include <retroshare/rsfiles.h>

#define FLAGS_GROUP_NETWORK_WIDE_ICON    ":images/anonymous_128_green.png"
#define FLAGS_GROUP_BROWSABLE_ICON       ":images/browsable_128_green.png"
#define FLAGS_GROUP_UNCHECKED            ":images/blank_128_green.png"
#define FLAGS_OTHER_NETWORK_WIDE_ICON    ":images/anonymous_128_blue.png"
#define FLAGS_OTHER_BROWSABLE_ICON       ":images/browsable_128_blue.png"
#define FLAGS_OTHER_UNCHECKED            ":images/blank_128_blue.png"

#define INDEX_GROUP_BROWSABLE     0
#define INDEX_GROUP_NETWORK_W     1
#define INDEX_OTHER_BROWSABLE     2
#define INDEX_OTHER_NETWORK_W     3
#define INDEX_GROUP_UNCHECKED     4
#define INDEX_OTHER_UNCHECKED     5

QString GroupFlagsWidget::_tooltips_on[4] = {
	QObject::tr("Directory is browsable for friends from parent groups"),
	QObject::tr("Directory is accessible by anonymous tunnels from friends from parent groups"),
	QObject::tr("Directory is browsable for any friend"),
	QObject::tr("Directory is accessible by anonymous tunnels from any friend") 
};
QString GroupFlagsWidget::_tooltips_off[4] = {
	QObject::tr(""),
	QObject::tr(""),
	QObject::tr(""),
	QObject::tr("") 
};

GroupFlagsWidget::GroupFlagsWidget(QWidget *parent,uint32_t flags)
	: QWidget(parent)
{
	_layout = new QHBoxLayout(this) ;

	setMinimumSize(128,32) ;
	setMaximumSize(128,32) ;
	setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);

	_icons[INDEX_GROUP_BROWSABLE] = new QIcon(FLAGS_GROUP_BROWSABLE_ICON) ;
	_icons[INDEX_GROUP_NETWORK_W] = new QIcon(FLAGS_GROUP_NETWORK_WIDE_ICON) ;
	_icons[INDEX_OTHER_BROWSABLE] = new QIcon(FLAGS_OTHER_BROWSABLE_ICON) ;
	_icons[INDEX_OTHER_NETWORK_W] = new QIcon(FLAGS_OTHER_NETWORK_WIDE_ICON) ;
	_icons[INDEX_GROUP_UNCHECKED] = new QIcon(FLAGS_GROUP_UNCHECKED) ;
	_icons[INDEX_OTHER_UNCHECKED] = new QIcon(FLAGS_OTHER_UNCHECKED) ;

	setLayout(_layout) ;

	_flags[0] = DIR_FLAGS_BROWSABLE_GROUPS ;
	_flags[1] = DIR_FLAGS_NETWORK_WIDE_GROUPS ;
	_flags[2] = DIR_FLAGS_BROWSABLE_OTHERS ;
	_flags[3] = DIR_FLAGS_NETWORK_WIDE_OTHERS ;

	for(int i=0;i<4;++i)
	{
		_buttons[i] = new QPushButton(this) ;
		_buttons[i]->setCheckable(true) ;
		_buttons[i]->setChecked(flags & _flags[i]) ;
		_buttons[i]->setIconSize(QSize(32,32));
		update_button_state(_buttons[i]->isChecked(),i) ;
		_layout->addWidget(_buttons[i]) ;
	}
		
	connect(_buttons[INDEX_GROUP_NETWORK_W],SIGNAL(toggled(bool)),this,SLOT(update_GN_button(bool))) ;
	connect(_buttons[INDEX_OTHER_NETWORK_W],SIGNAL(toggled(bool)),this,SLOT(update_ON_button(bool))) ;
	connect(_buttons[INDEX_GROUP_BROWSABLE],SIGNAL(toggled(bool)),this,SLOT(update_GB_button(bool))) ;
	connect(_buttons[INDEX_OTHER_BROWSABLE],SIGNAL(toggled(bool)),this,SLOT(update_OB_button(bool))) ;

	_layout->setSpacing(0);
	_layout->setContentsMargins(10, 0, 0, 0);

	_layout->update() ;
}

void GroupFlagsWidget::updated() 
{
	emit flagsChanged(flags()) ;
}

uint32_t GroupFlagsWidget::flags() const 
{
	uint32_t flags = 0x0 ;

	for(int i=0;i<4;++i)
		if(_buttons[i]->isChecked()) flags |= _flags[i] ;

	return flags ;
}

void GroupFlagsWidget::update_button_state(bool b,int button_id)
{
	if(b)
	{
		_buttons[button_id]->setIcon(*_icons[button_id]) ;
		_buttons[button_id]->setToolTip(_tooltips_on[button_id]) ;
	}
	else if(button_id == INDEX_GROUP_NETWORK_W || button_id == INDEX_GROUP_BROWSABLE)
	{
		_buttons[button_id]->setIcon(*_icons[INDEX_GROUP_UNCHECKED]) ;
		_buttons[button_id]->setToolTip(_tooltips_off[button_id]) ;
	}
	else
	{
		_buttons[button_id]->setIcon(*_icons[INDEX_OTHER_UNCHECKED]) ;
		_buttons[button_id]->setToolTip(_tooltips_off[button_id]) ;
	}
}

void GroupFlagsWidget::update_GN_button(bool b) { update_button_state(b,INDEX_GROUP_NETWORK_W) ; updated() ; }
void GroupFlagsWidget::update_GB_button(bool b) { update_button_state(b,INDEX_GROUP_BROWSABLE) ; updated() ; }
void GroupFlagsWidget::update_ON_button(bool b) { update_button_state(b,INDEX_OTHER_NETWORK_W) ; updated() ; }
void GroupFlagsWidget::update_OB_button(bool b) { update_button_state(b,INDEX_OTHER_BROWSABLE) ; updated() ; }

GroupFlagsWidget::~GroupFlagsWidget()
{
	for(int i=0;i<4;++i)
	{
		delete _buttons[i] ;
		delete _icons[i] ;
	}
	delete _icons[INDEX_GROUP_UNCHECKED] ;
	delete _icons[INDEX_OTHER_UNCHECKED] ;
}


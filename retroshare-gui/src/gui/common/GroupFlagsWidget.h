#pragma once

#include <stdint.h>
#include <QPushButton>
#include <QFrame>
#include <retroshare/rsflags.h>

class GroupFlagsWidget: public QWidget
{
	Q_OBJECT

	public:
		GroupFlagsWidget(QWidget *parent,FileStorageFlags flags = FileStorageFlags(0u)) ;
		virtual ~GroupFlagsWidget() ;

		FileStorageFlags flags() const ;
		void setFlags(FileStorageFlags flags) ;

		static QString groupInfoString(FileStorageFlags flags,const std::list<std::string>& groups) ;
	public slots:
		void updated() ;

	protected slots:
		void update_GN_button(bool) ;
		void update_GB_button(bool) ;
		void update_ON_button(bool) ;
		void update_OB_button(bool) ;

	signals:
		void flagsChanged(FileStorageFlags) const ;

	private:
		void update_button_state(bool b,int id) ;

		QPushButton *_buttons[4] ;

		QLayout *_layout ;
		QIcon *_icons[6] ;
		FileStorageFlags _flags[4] ;

		static QString _tooltips_on[4] ;
		static QString _tooltips_off[4] ;
};

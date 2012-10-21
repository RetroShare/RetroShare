#pragma once

#include <stdint.h>
#include <QPushButton>
#include <QWidget>

class GroupFlagsWidget: public QWidget
{
	Q_OBJECT

	public:
		GroupFlagsWidget(QWidget *parent,uint32_t flags) ;
		virtual ~GroupFlagsWidget() ;

		uint32_t flags() const ;

	public slots:
		void updated() ;

	protected slots:
		void update_GN_button(bool) ;
		void update_GB_button(bool) ;
		void update_ON_button(bool) ;
		void update_OB_button(bool) ;

	signals:
		void flagsChanged(uint32_t) const ;

	private:
		void update_button_state(bool b,int id) ;

		QPushButton *_buttons[4] ;

		QLayout *_layout ;
		QIcon *_icons[6] ;
		uint32_t _flags[4] ;

		static QString _tooltips_on[4] ;
		static QString _tooltips_off[4] ;
};

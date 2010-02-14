#pragma once

#include <QPoint>
#include "ui_TurtleRouterDialog.h"
#include "RsAutoUpdatePage.h"

class TurtleRouterDialog: public RsAutoUpdatePage, public Ui::TurtleRouterDialogForm
{
	Q_OBJECT

	public:
		TurtleRouterDialog(QWidget *parent = NULL) ;
		static void showUp() ;

                /** Default Constructor */
	protected slots:
		void showCtxMenu(const QPoint&) ;
		void removeFileHash() ;

	private:
		void fillTable(QTableWidget *table,const std::vector<std::vector<std::string> >&) ;
		QTimer *_timer ;

		virtual void updateDisplay() ;

		static TurtleRouterDialog *_instance ;
} ;

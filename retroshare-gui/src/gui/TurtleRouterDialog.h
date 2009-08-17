#pragma once

#include <QPoint>
#include "ui_TurtleRouterDialog.h"

class TurtleRouterDialog: public QWidget, public Ui::TurtleRouterDialogForm
{
	Q_OBJECT

	public:
		static void showUp() ;

                /** Default Constructor */
	protected slots:
		void update() ;
		void showCtxMenu(const QPoint&) ;
		void removeFileHash() ;

	private:
		void fillTable(QTableWidget *table,const std::vector<std::vector<std::string> >&) ;
		TurtleRouterDialog(QWidget *parent = NULL) ;
		QTimer *_timer ;

		static TurtleRouterDialog *_instance ;
} ;

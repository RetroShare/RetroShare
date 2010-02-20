#pragma once

#include <QPoint>
#include "ui_TurtleRouterDialog.h"
#include "RsAutoUpdatePage.h"

class TurtleRouterDialog: public RsAutoUpdatePage, public Ui::TurtleRouterDialogForm
{
	public:
		TurtleRouterDialog(QWidget *parent = NULL) ;

                /** Default Constructor */
	private:
		virtual void updateDisplay() ;
		QTreeWidgetItem *findParentHashItem(const std::string& hash) ;

		std::map<std::string,QTreeWidgetItem*> top_level_hashes ;
		QTreeWidgetItem *top_level_unknown_hashes ;
		QTreeWidgetItem *top_level_s_requests ;
		QTreeWidgetItem *top_level_t_requests ;
} ;

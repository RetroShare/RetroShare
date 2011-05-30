#pragma once

#include <QPoint>
#include <retroshare/rsturtle.h>
#include "ui_TurtleRouterDialog.h"
#include "RsAutoUpdatePage.h"

class TurtleRouterStatisticsWidget ;

class TurtleRouterDialog: public RsAutoUpdatePage, public Ui::TurtleRouterDialogForm
{
	public:
		TurtleRouterDialog(QWidget *parent = NULL) ;
		~TurtleRouterDialog();
		
		// Cache for peer names.
		static QString getPeerName(const std::string& peer_id) ;

	private:
		void updateTunnelRequests(	const std::vector<std::vector<std::basic_string<char> > >&, 
											const std::vector<std::vector<std::basic_string<char> > >&, 
											const std::vector<TurtleRequestDisplayInfo >&, 
											const std::vector<TurtleRequestDisplayInfo >&) ;
											
		void processSettings(bool bLoad);
		bool m_bProcessSettings;

		virtual void updateDisplay() ;
		QTreeWidgetItem *findParentHashItem(const std::string& hash) ;

		std::map<std::string,QTreeWidgetItem*> top_level_hashes ;
		QTreeWidgetItem *top_level_unknown_hashes ;
		QTreeWidgetItem *top_level_s_requests ;
		QTreeWidgetItem *top_level_t_requests ;

		TurtleRouterStatisticsWidget *_tst_CW ;
} ;

class TurtleRouterStatisticsWidget:  public QWidget
{
	public:
		TurtleRouterStatisticsWidget(QWidget *parent = NULL) ;

		virtual void paintEvent(QPaintEvent *event) ;
		virtual void resizeEvent(QResizeEvent *event);

		void updateTunnelStatistics(	const std::vector<std::vector<std::basic_string<char> > >&, 
												const std::vector<std::vector<std::basic_string<char> > >&, 
												const std::vector<TurtleRequestDisplayInfo >&, 
												const std::vector<TurtleRequestDisplayInfo >&) ;

	private:
		static QString speedString(float f) ;

		QPixmap pixmap ;
		int maxWidth,maxHeight ;
};


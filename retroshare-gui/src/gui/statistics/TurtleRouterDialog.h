#pragma once

#include <retroshare/rsturtle.h>
#include <retroshare/rstypes.h>
#include "ui_TurtleRouterDialog.h"
#include "ui_TurtleRouterStatistics.h"
#include "RsAutoUpdatePage.h"


class TurtleRouterDialog: public RsAutoUpdatePage, public Ui::TurtleRouterDialogForm
{
	Q_OBJECT

	public:
		TurtleRouterDialog(QWidget *parent = NULL) ;
		~TurtleRouterDialog();
		
		// Cache for peer names.
        static QString getPeerName(const RsPeerId &peer_id) ;

	private:
		void updateTunnelRequests(	const std::vector<std::vector<std::basic_string<char> > >&, 
											const std::vector<std::vector<std::basic_string<char> > >&, 
											const std::vector<TurtleSearchRequestDisplayInfo >&,
											const std::vector<TurtleTunnelRequestDisplayInfo >&) ;
											
		void processSettings(bool bLoad);
		bool m_bProcessSettings;

		virtual void updateDisplay() ;
		QTreeWidgetItem *findParentHashItem(const std::string& hash) ;

		std::map<std::string,QTreeWidgetItem*> top_level_hashes ;
		QTreeWidgetItem *top_level_unknown_hashes ;
		QTreeWidgetItem *top_level_s_requests ;
		QTreeWidgetItem *top_level_t_requests ;

} ;

class GxsTunnelsDialog: public RsAutoUpdatePage
{
    Q_OBJECT

public:
    GxsTunnelsDialog(QWidget *parent = NULL) ;
    ~GxsTunnelsDialog();

    // Cache for peer names.
    static QString getPeerName(const RsPeerId &peer_id) ;

protected:
    virtual void paintEvent(QPaintEvent *);
    virtual void resizeEvent(QResizeEvent *event);
private:
    void processSettings(bool bLoad);
    bool m_bProcessSettings;
    static QString speedString(float f);

    virtual void updateDisplay() ;
    
    int maxWidth ;
    int maxHeight ;
    
    QPixmap pixmap;
} ;

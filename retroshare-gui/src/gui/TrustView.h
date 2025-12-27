#pragma once

#include "ui_TrustView.h"
#include <retroshare-gui/RsAutoUpdatePage.h>

class QWheelEvent ;
class QShowEvent ;

class TrustView: public RsAutoUpdatePage, public Ui::TrustView
{
	Q_OBJECT

	public:
		TrustView() ;

	protected:
		virtual void wheelEvent(QWheelEvent *) ;
		virtual void showEvent(QShowEvent *) ;

		virtual void updateDisplay() ;
	public slots:
		void update() ;
		void updateZoom(int) ;
		void selectCell(int,int) ;
		void hideShowPeers(int) ;

	private:
		void getCellSize(int z,int& cell_width,int& cell_height) const ;
		int getRowColId(const std::string& name) ;

		static const int _base_cell_width = 60 ;
		static const int _base_cell_height = 30 ;
};


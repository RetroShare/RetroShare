/*******************************************************************************
 * gui/statistics/TurtleRouterDialog.h                                         *
 *                                                                             *
 * Copyright (c) 2011 Retroshare Team <retroshare.project@gmail.com>           *
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

#pragma once

#include <retroshare/rsturtle.h>
#include <retroshare/rstypes.h>

#include <retroshare-gui/RsAutoUpdatePage.h>

#include <QTreeWidgetItem>
#include "ui_TurtleRouterDialog.h"
#include "ui_TurtleRouterStatistics.h"

class QModelIndex;
class QPainter;
class FTTunnelsListDelegate ;

class TurtleTreeWidgetItem : public QTreeWidgetItem
{
public:
	using QTreeWidgetItem::QTreeWidgetItem;

	bool operator<(const QTreeWidgetItem &other) const override
	{
		int column = treeWidget()->sortColumn();

		if (treeWidget()->indexOfTopLevelItem(const_cast<TurtleTreeWidgetItem*>(this)) != -1)
		{
			// Top level items are always sorted by text (or just stay where they are if they are headers)
			return QTreeWidgetItem::operator<(other);
		}

		// Check for numeric sorting in UserRole
		QVariant v1 = data(column, Qt::UserRole);
		QVariant v2 = other.data(column, Qt::UserRole);

		if (v1.isValid() && v2.isValid())
		{
			return v1.toDouble() < v2.toDouble();
		}

		// Specialized sorting for the "String" column in Requests tab
		// We want complex expressions to be last.
		if (column == 3 && treeWidget()->objectName() == "_f2f_TW")
		{
			QString s1 = text(column);
			QString s2 = other.text(column);

			auto isComplex = [](const QString& s) {
				return s.startsWith("(") || s.startsWith("NAME") || s.startsWith("SIZE") || 
				       s.startsWith("DATE") || s.startsWith("EXTENSION") || s.startsWith("PATH") || 
				       s.startsWith("HASH") || s.startsWith("POPULARITY") || s.startsWith("Generic search");
			};

			bool complex1 = isComplex(s1);
			bool complex2 = isComplex(s2);

			if (complex1 && !complex2) return false;
			if (!complex1 && complex2) return true;
		}

		return QTreeWidgetItem::operator<(other);
	}
};

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

protected:
	void hideEvent(QHideEvent *event) override;
	FTTunnelsListDelegate *FTTDelegate;

} ;

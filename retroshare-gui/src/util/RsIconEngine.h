/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (c) 2016, RetroShare Team
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

#ifndef RSICONENGINE_H
#define RSICONENGINE_H

#ifndef Q_DECL_OVERRIDE
# define Q_DECL_OVERRIDE
#endif

#include <QIcon>
#include <QIconEngine>
#include <QObject>

/************************************************************************************/
/* Copied fromQIcon private source file:                                            */
/* https://code.woboq.org/qt5/qtbase/src/gui/image/qicon_p.h.html#QPixmapIconEngine */
/************************************************************************************/
struct RsIconEngineEntry
{
	RsIconEngineEntry():mode(QIcon::Normal), state(QIcon::Off){}
	RsIconEngineEntry(const QPixmap &pm, QIcon::Mode m = QIcon::Normal, QIcon::State s = QIcon::Off)
	  :pixmap(pm), size(pm.size()), mode(m), state(s){}
	RsIconEngineEntry(const QString &file, const QSize &sz = QSize(), QIcon::Mode m = QIcon::Normal, QIcon::State s = QIcon::Off)
	  :fileName(file), size(sz), mode(m), state(s){}
	RsIconEngineEntry(const QString &file, const QImage &image, QIcon::Mode m = QIcon::Normal, QIcon::State s = QIcon::Off);
	QPixmap pixmap;
	QString fileName;
	QSize size;
	QIcon::Mode mode;
	QIcon::State state;
	bool isNull() const {return (fileName.isEmpty() && pixmap.isNull()); }
};

Q_DECLARE_TYPEINFO(RsIconEngineEntry, Q_MOVABLE_TYPE);
inline RsIconEngineEntry::RsIconEngineEntry(const QString &file, const QImage &image, QIcon::Mode m, QIcon::State s)
  : fileName(file), size(image.size()), mode(m), state(s)
{
	pixmap.convertFromImage(image);
#if QT_VERSION >= 0x050000
	// Reset the devicePixelRatio. The pixmap may be loaded from a @2x file,
	// but be used as a 1x pixmap by QIcon.
	pixmap.setDevicePixelRatio(1.0);
#endif
}

/************************************************************************************/
/* RetroShare Code                                                                  */
/************************************************************************************/
class RsIconEngine : public QIconEngine
{
public:
	RsIconEngine();
	RsIconEngine(const RsIconEngine &);
	~RsIconEngine();
	void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state) Q_DECL_OVERRIDE;
	QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) Q_DECL_OVERRIDE;
	RsIconEngineEntry *bestMatch(const QSize &size, QIcon::Mode mode, QIcon::State state, bool sizeOnly);
	QSize actualSize(const QSize &size, QIcon::Mode mode, QIcon::State state) Q_DECL_OVERRIDE;
	void addPixmap(const QPixmap &pixmap, QIcon::Mode mode, QIcon::State state) Q_DECL_OVERRIDE;
	void addFile(const QString &fileName, const QSize &size, QIcon::Mode mode, QIcon::State state) Q_DECL_OVERRIDE;
	QString key() const Q_DECL_OVERRIDE;
	QIconEngine *clone() const Q_DECL_OVERRIDE;
	bool read(QDataStream &in) Q_DECL_OVERRIDE;
	bool write(QDataStream &out) const Q_DECL_OVERRIDE;
#if QT_VERSION >= 0x050000
	void virtual_hook(int id, void *data) Q_DECL_OVERRIDE;
#endif

	bool onNotify() const {return m_OnNotify;}
	void setOnNotify(const bool value) {m_OnNotify=value;}
#if QT_VERSION < 0x050700
	bool isNull() { return false; }
#endif


private:
	RsIconEngineEntry *tryMatch(const QSize &size, QIcon::Mode mode, QIcon::State state);
	void checkParam();
	QPixmap drawPixmap(QPixmap &pixmapOrig);
	QVector<RsIconEngineEntry> pixmaps;
	friend Q_GUI_EXPORT QDataStream &operator<<(QDataStream &s, const QIcon &icon);
	friend class QIconThemeEngine;

	bool m_OnNotify;
	QColor m_Color;
	QColor m_ColorOnNotify;
	uint m_Margin;

};

#endif // RSICONENGINE_H

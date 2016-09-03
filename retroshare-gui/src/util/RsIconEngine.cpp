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

#include "RsIconEngine.h"

#include <cmath>

#include <QApplication>
#include <QImageReader>
#include <QPainter>
#include <QPixmapCache>
#include <QStyleOption>

#include "util/misc.h"
#include "gui/settings/rsharesettings.h"

/*********************************************************************************************/
/* Copied fromQIcon private source file:                                                     */
/* https://code.woboq.org/qt5/qtbase/src/gui/image/qicon.cpp.html#_ZN17QPixmapIconEngineC1Ev */
/*********************************************************************************************/

/*! \internal

	Returns the effective device pixel ratio, using
	the provided window pointer if possible.

	if Qt::AA_UseHighDpiPixmaps is not set this function
	returns 1.0 to keep non-hihdpi aware code working.
*/
static qreal qt_effective_device_pixel_ratio(int */*window*/ = 0)//Retroshare Change: Don't use QWindow
{
#if QT_VERSION >= 0x050000
	if (!qApp->testAttribute(Qt::AA_UseHighDpiPixmaps))
		return qreal(1.0);

	//if (window)
	//	return window->devicePixelRatio();

	return qApp->devicePixelRatio(); // Don't know which window to target.
#else
	return qreal(1.0);
#endif
}

RsIconEngine::RsIconEngine()
  : m_OnNotify(false), m_Color(QColor()), m_ColorOnNotify(QColor()), m_Margin(0)
{
}

RsIconEngine::RsIconEngine(const RsIconEngine &other)
  : QIconEngine(other), pixmaps(other.pixmaps), m_OnNotify(false)
  , m_Color(QColor()), m_ColorOnNotify(QColor()), m_Margin(0)
{
}

RsIconEngine::~RsIconEngine()
{
}

void RsIconEngine::paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state)
{
	QSize pixmapSize = rect.size() * qt_effective_device_pixel_ratio(0);
	QPixmap px = pixmap(pixmapSize, mode, state);
	painter->drawPixmap(rect, px);
}

static inline int area(const QSize &s) { return s.width() * s.height(); }

// returns the smallest of the two that is still larger than or equal to size.
static RsIconEngineEntry *bestSizeMatch( const QSize &size, RsIconEngineEntry *pa, RsIconEngineEntry *pb)
{
	int s = area(size);
	if (pa->size == QSize() && pa->pixmap.isNull()) {
		pa->pixmap = QPixmap(pa->fileName);
		pa->size = pa->pixmap.size();
	}
	int a = area(pa->size);
	if (pb->size == QSize() && pb->pixmap.isNull()) {
		pb->pixmap = QPixmap(pb->fileName);
		pb->size = pb->pixmap.size();
	}
	int b = area(pb->size);
	int res = a;
	if (qMin(a,b) >= s)
		res = qMin(a,b);
	else
		res = qMax(a,b);
	if (res == a)
		return pa;
	return pb;
}

RsIconEngineEntry *RsIconEngine::tryMatch(const QSize &size, QIcon::Mode mode, QIcon::State state)
{
	RsIconEngineEntry *pe = 0;
	for (int i = 0; i < pixmaps.count(); ++i)
		if (pixmaps.at(i).mode == mode && pixmaps.at(i).state == state) {
			if (pe)
				pe = bestSizeMatch(size, &pixmaps[i], pe);
			else
				pe = &pixmaps[i];
		}
	return pe;
}

RsIconEngineEntry *RsIconEngine::bestMatch(const QSize &size, QIcon::Mode mode, QIcon::State state, bool sizeOnly)
{
	RsIconEngineEntry *pe = tryMatch(size, mode, state);
	while (!pe){
		QIcon::State oppositeState = (state == QIcon::On) ? QIcon::Off : QIcon::On;
		if (mode == QIcon::Disabled || mode == QIcon::Selected) {
			QIcon::Mode oppositeMode = (mode == QIcon::Disabled) ? QIcon::Selected : QIcon::Disabled;
			if ((pe = tryMatch(size, QIcon::Normal, state)))
				break;
			if ((pe = tryMatch(size, QIcon::Active, state)))
				break;
			if ((pe = tryMatch(size, mode, oppositeState)))
				break;
			if ((pe = tryMatch(size, QIcon::Normal, oppositeState)))
				break;
			if ((pe = tryMatch(size, QIcon::Active, oppositeState)))
				break;
			if ((pe = tryMatch(size, oppositeMode, state)))
				break;
			if ((pe = tryMatch(size, oppositeMode, oppositeState)))
				break;
		} else {
			QIcon::Mode oppositeMode = (mode == QIcon::Normal) ? QIcon::Active : QIcon::Normal;
			if ((pe = tryMatch(size, oppositeMode, state)))
				break;
			if ((pe = tryMatch(size, mode, oppositeState)))
				break;
			if ((pe = tryMatch(size, oppositeMode, oppositeState)))
				break;
			if ((pe = tryMatch(size, QIcon::Disabled, state)))
				break;
			if ((pe = tryMatch(size, QIcon::Selected, state)))
				break;
			if ((pe = tryMatch(size, QIcon::Disabled, oppositeState)))
				break;
			if ((pe = tryMatch(size, QIcon::Selected, oppositeState)))
				break;
		}

		if (!pe)
			return pe;
	}

	if (sizeOnly ? (pe->size.isNull() || !pe->size.isValid()) : pe->pixmap.isNull()) {
		pe->pixmap = QPixmap(pe->fileName);
		if (!pe->pixmap.isNull())
			pe->size = pe->pixmap.size();
	}

	return pe;
}

/// RetroShare Code *****************************///

/// \brief RsIconEngine::checkParam
/// Check if parameters are changed
void RsIconEngine::checkParam()
{
	if (m_Color != Settings->getRsIconColor()) {
		m_Color = Settings->getRsIconColor();
		//pixmaps.clear();
	}
	if (m_ColorOnNotify != Settings->getRsIconColorOnNotify()) {
		m_ColorOnNotify = Settings->getRsIconColorOnNotify();
		//pixmaps.clear();
	}
	if (m_Margin != Settings->getRsIconMarginOnNotify()) {
		m_Margin = Settings->getRsIconMarginOnNotify();
		//pixmaps.clear();
	}
}

/// \brief RsIconEngine::drawPixmap
/// \param imageOrig image from file
/// \return pixmap with colored background
///
QPixmap RsIconEngine::drawPixmap(QPixmap &pixmapOrig)
{
	checkParam();
	QPixmap pixmap = pixmapOrig;
	QPainter painter(&pixmap);
	QRect rect = pixmap.rect();
	QBrush brushON = painter.background();
	//QColor m_Color = Settings->getRsIconColor();
	//QColor m_ColorOnNotify = Settings->getRsIconColorOnNotify();
	//uint m_Margin = Settings->getRsIconMarginOnNotify();

	brushON.setColor(m_OnNotify?m_ColorOnNotify:m_Color);
	QPen pen = painter.pen();
	pen.setColor(Qt::transparent);
	painter.setPen(pen);
	painter.setRenderHint(QPainter::Antialiasing);
	if (m_Margin>0 && m_OnNotify) {
		uint margin = (m_Margin * 0.01) * sqrt(pow(rect.height(),2) + pow(rect.width(),2));
		painter.setBrush(brushON);
		painter.drawEllipse(rect.adjusted(-margin,-margin,margin,margin));
		QBrush brush = painter.background();
		brush.setColor(m_Color);
		painter.setBrush(brush);
		painter.drawEllipse(rect);
	} else {
		painter.setBrush(brushON);
		painter.drawEllipse(rect);
	}
	painter.drawPixmap(rect, pixmapOrig);
	painter.end();
	return pixmap;
}

///End RetroShareCode ***************************///

QPixmap RsIconEngine::pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state)
{
	checkParam(); //RetroShare Change
	QPixmap pm;
	RsIconEngineEntry *pe = bestMatch(size, mode, state, false);
	if (pe)
		pm = drawPixmap(pe->pixmap); //RetroShare Change

	if (pm.isNull()) {
		int idx = pixmaps.count();
		while (--idx >= 0) {
			if (pe == &pixmaps.at(idx)) {
				pixmaps.remove(idx);
				break;
			}
		}
		if (pixmaps.isEmpty())
			return pm;
		else
			return pixmap(size, mode, state);
	}

	QSize actualSize = pm.size();
	if (!actualSize.isNull() && (actualSize.width() > size.width() || actualSize.height() > size.height()))
		actualSize.scale(size, Qt::KeepAspectRatio);

	QString key = QLatin1String("qt_")
	    % HexString<quint64>(pm.cacheKey())
	    % HexString<uint>(pe->mode)
#if QT_VERSION >= 0x050000
	    % HexString<quint64>(QGuiApplication::palette().cacheKey())
#endif
	    % HexString<uint>(actualSize.width())
	    % HexString<uint>(actualSize.height())
	    % HexString<uint>(m_Color.rgba()) /*RetroShare Change */
	    % HexString<uint>(m_ColorOnNotify.rgba()) /*RetroShare Change */
	    % HexString<uint>(m_Margin) /*RetroShare Change */
	    + (m_OnNotify ? "N" : "") ; /*RetroShare Change */
	if (mode == QIcon::Active) {
		if (QPixmapCache::find(key % HexString<uint>(mode), pm))
			return pm; // horray

		if (QPixmapCache::find(key % HexString<uint>(QIcon::Normal), pm)) {
			QPixmap active = pm;

#if QT_VERSION >= 0x050000
			if (/*QGuiApplication *guiApp = */qobject_cast<QGuiApplication *>(qApp)) {
				//Retroshare change as QGuiApplicationPrivate is not visible from here
				QStyleOption opt(0);
				opt.palette = QGuiApplication::palette();
				active = QApplication::style()->generatedIconPixmap(mode, pm, &opt);
				//active = static_cast<QGuiApplicationPrivate*>(QObjectPrivate::get(guiApp))->applyQIconStyleHelper(QIcon::Active, pm);
			}
#endif

			if (pm.cacheKey() == active.cacheKey())
				return pm;
		}
	}

	if (!QPixmapCache::find(key % HexString<uint>(mode), pm)) {
		if (pm.size() != actualSize)
			pm = pm.scaled(actualSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
		if (pe->mode != mode && mode != QIcon::Normal) {
			QPixmap generated = pm;
#if QT_VERSION >= 0x050000
			if (/*QGuiApplication *guiApp = */qobject_cast<QGuiApplication *>(qApp)) {
				//Retroshare change as QGuiApplicationPrivate is not visible from here
				QStyleOption opt(0);
				opt.palette = QGuiApplication::palette();
				generated = QApplication::style()->generatedIconPixmap(mode, pm, &opt);
				//generated = static_cast<QGuiApplicationPrivate*>(QObjectPrivate::get(guiApp))->applyQIconStyleHelper(mode, pm);
			}
#endif
			if (!generated.isNull())
				pm = generated;
		}
		QPixmapCache::insert(key % HexString<uint>(mode), pm);
	}
	return pm;
}

QSize RsIconEngine::actualSize(const QSize &size, QIcon::Mode mode, QIcon::State state)
{
	QSize actualSize;
	if (RsIconEngineEntry *pe = bestMatch(size, mode, state, true))
		actualSize = pe->size;

	if (actualSize.isNull())
		return actualSize;

	if (!actualSize.isNull() && (actualSize.width() > size.width() || actualSize.height() > size.height()))
		actualSize.scale(size, Qt::KeepAspectRatio);
	return actualSize;
}

void RsIconEngine::addPixmap(const QPixmap &pixmap, QIcon::Mode mode, QIcon::State state)
{
	if (!pixmap.isNull()) {
		RsIconEngineEntry *pe = tryMatch(pixmap.size(), mode, state);
		if(pe && pe->size == pixmap.size()) {
			pe->pixmap = pixmap;
			pe->fileName.clear();
		} else {
			pixmaps += RsIconEngineEntry(pixmap, mode, state);
		}
	}
}

// Read out original image depth as set by ICOReader
static inline int origIcoDepth(const QImage &image)
{
#if QT_VERSION >= 0x050000
	const QString s = image.text(QStringLiteral("_q_icoOrigDepth"));
#else
	const QString s = image.text(QString("_q_icoOrigDepth"));
#endif
	return s.isEmpty() ? 32 : s.toInt();
}

static inline int findBySize(const QVector<QImage> &images, const QSize &size)
{
	for (int i = 0; i < images.size(); ++i) {
		if (images.at(i).size() == size)
			return i;
	}
	return -1;
}

// Convenience class providing a bool read() function.
namespace {
class ImageReader
{
public:
	explicit ImageReader(const QString &fileName) : m_reader(fileName), m_atEnd(false) {} //RetroShare Change explicit

	QByteArray format() const { return m_reader.format(); }

	bool read(QImage *image)
	{
		if (m_atEnd)
			return false;
		*image = m_reader.read();
		if (!image->size().isValid()) {
			m_atEnd = true;
			return false;
		}
		m_atEnd = !m_reader.jumpToNextImage();
		return true;
	}

private:
	QImageReader m_reader;
	bool m_atEnd;
};
} // namespace

void RsIconEngine::addFile(const QString &fileName, const QSize &size, QIcon::Mode mode, QIcon::State state)
{
	if (fileName.isEmpty())
		return;
	const QString abs = fileName.startsWith(QLatin1Char(':')) ? fileName : QFileInfo(fileName).absoluteFilePath();
	const bool ignoreSize = !size.isValid();
	ImageReader imageReader(abs);
	const QByteArray format = imageReader.format();
	if (format.isEmpty()) // Device failed to open or unsupported format.
		return;
	QImage image;
	if (format != "ico") {
		if (ignoreSize) { // No size specified: Add all images.
			while (imageReader.read(&image))
				pixmaps += RsIconEngineEntry(abs, image, mode, state);
		} else {
			// Try to match size. If that fails, add a placeholder with the filename and empty pixmap for the size.
			while (imageReader.read(&image) && image.size() != size) {}
			pixmaps += image.size() == size ?
			      RsIconEngineEntry(abs, image, mode, state) : RsIconEngineEntry(abs, size, mode, state);
		}
		return;
	}
	// Special case for reading Windows ".ico" files. Historically (QTBUG-39287),
	// these files may contain low-resolution images. As this information is lost,
	// ICOReader sets the original format as an image text key value. Read all matching
	// images into a list trying to find the highest quality per size.
	QVector<QImage> icoImages;
	while (imageReader.read(&image)) {
		if (ignoreSize || image.size() == size) {
			const int position = findBySize(icoImages, image.size());
			if (position >= 0) { // Higher quality available? -> replace.
				if (origIcoDepth(image) > origIcoDepth(icoImages.at(position)))
					icoImages[position] = image;
			} else {
				icoImages.append(image);
			}
		}
	}
	//RetroShare Change: range-based ‘for’ loops are not allowed in C++98 mode
	for (int i = 0; i < icoImages.size(); ++i) //RetroShare Change
		pixmaps += RsIconEngineEntry(abs, icoImages[i], mode, state);//RetroShare Change
	if (icoImages.isEmpty() && !ignoreSize) // Add placeholder with the filename and empty pixmap for the size.
		pixmaps += RsIconEngineEntry(abs, size, mode, state);
}

QString RsIconEngine::key() const
{
	return QLatin1String("RsIconEngine");
}

QIconEngine *RsIconEngine::clone() const
{
	return new RsIconEngine(*this);
}

bool RsIconEngine::read(QDataStream &in)
{
	int num_entries;
	QPixmap pm;
	QString fileName;
	QSize sz;
	uint mode;
	uint state;

	in >> num_entries;
	for (int i=0; i < num_entries; ++i) {
		if (in.atEnd()) {
			pixmaps.clear();
			return false;
		}
		in >> pm;
		in >> fileName;
		in >> sz;
		in >> mode;
		in >> state;
		if (pm.isNull()) {
			addFile(fileName, sz, QIcon::Mode(mode), QIcon::State(state));
		} else {
			RsIconEngineEntry pe(fileName, sz, QIcon::Mode(mode), QIcon::State(state));
			pe.pixmap = pm;
			pixmaps += pe;
		}
	}
	return true;
}

bool RsIconEngine::write(QDataStream &out) const
{
	int num_entries = pixmaps.size();
	out << num_entries;
	for (int i=0; i < num_entries; ++i) {
		if (pixmaps.at(i).pixmap.isNull())
			out << QPixmap(pixmaps.at(i).fileName);
		else
			out << pixmaps.at(i).pixmap;
		out << pixmaps.at(i).fileName;
		out << pixmaps.at(i).size;
		out << (uint) pixmaps.at(i).mode;
		out << (uint) pixmaps.at(i).state;
	}
	return true;
}

#if QT_VERSION >= 0x050000
void RsIconEngine::virtual_hook(int id, void *data)
{
	switch (id) {
		case QIconEngine::AvailableSizesHook: {
			QIconEngine::AvailableSizesArgument &arg =
			    *reinterpret_cast<QIconEngine::AvailableSizesArgument*>(data);
			arg.sizes.clear();
			for (int i = 0; i < pixmaps.size(); ++i) {
				RsIconEngineEntry &pe = pixmaps[i];
				if (pe.size == QSize() && pe.pixmap.isNull()) {
					pe.pixmap = QPixmap(pe.fileName);
					pe.size = pe.pixmap.size();
				}
				if (pe.mode == arg.mode && pe.state == arg.state && !pe.size.isEmpty())
					arg.sizes.push_back(pe.size);
			}
			break;
		}
		default:
			QIconEngine::virtual_hook(id, data);
	}
}
#endif

/*******************************************************************************
 * gui/common/RsLogo.cpp                                                       *
 *                                                                             *
 * Copyright (C) 2026, Retroshare Team <retroshare.project@gmail.com>          *
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

#include "RsLogo.h"

#include "gui/settings/rsharesettings.h"

#include <QApplication>
#include <QBuffer>
#include <QDomDocument>
#include <QDomElement>
#include <QDomNodeList>
#include <QEvent>
#include <QFile>
#include <QHash>
#include <QImage>
#include <QSvgRenderer>
#include <QtDebug>
#include <QPainter>
#include <QStringList>
#include <QWidget>

#define LOGO_SVG               ":/icons/svg/retroshare-symbol_gradients.svg"
#define LOGO_FALLBACK          ":/icons/logo_128.png"

/* ids inside retroshare-symbol_gradients.svg */
#define LOGO_BACKGROUND_ID     "path4140"
#define LOGO_GRAY_STOPS_ID     "rsLogoGrayStops"
#define LOGO_STOP_LIGHT_ID     "stop4185"   /* #abe3ff */
#define LOGO_STOP_DARK_ID      "stop4187"   /* #0e8ecb */
#define LOGO_DEFAULT_LIGHT     "#abe3ff"

/* property set on windows whose icon is handled by RsLogo */
#define LOGO_WINDOW_PROPERTY   "rsLogoWindowIcon"

namespace {

/* Nodes of the symbol in the order they get highlighted by the tray icon:
 * top right, bottom, left (same as logo_[012]_connected_128.png) */
const char *const sNodeIds[] = { "path4148", "path2987-4", "path4146" };

const int sIconSizes[] = { 16, 20, 24, 32, 48, 64, 128, 256 };

QHash<QString, QIcon> sIconCache;

QString colorKey(const QColor& background, const QColor& foreground, int litNodes)
{
	return background.name(QColor::HexArgb) + foreground.name(QColor::HexArgb) + QString::number(litNodes);
}

/* light end of the symbol gradient */
QColor lightColor(const QColor& color)
{
	if (color.rgb() == RsLogo::defaultForegroundColor().rgb()) {
		/* keep the original blue gradient of the logo */
		return QColor(LOGO_DEFAULT_LIGHT);
	}

	/* user defined color: flat, no gradient */
	return color;
}

/* recursive search, QDomDocument::elementsByTagName() has no "*" wildcard */
QDomElement findById(const QDomElement& parent, const QString& id)
{
	for (QDomElement element = parent.firstChildElement(); !element.isNull(); element = element.nextSiblingElement()) {
		if (element.attribute("id") == id) {
			return element;
		}
		QDomElement found = findById(element, id);
		if (!found.isNull()) {
			return found;
		}
	}
	return QDomElement();
}

QDomElement findById(const QDomDocument& doc, const QString& id)
{
	QDomElement root = doc.documentElement();
	if (root.attribute("id") == id) {
		return root;
	}
	return findById(root, id);
}

QString styleProperty(const QDomElement& element, const QString& name)
{
	foreach (const QString& prop, element.attribute("style").split(';')) {
		int pos = prop.indexOf(':');
		if (pos > 0 && prop.left(pos).trimmed() == name) {
			return prop.mid(pos + 1).trimmed();
		}
	}
	return QString();
}

void setStyleProperty(QDomElement& element, const QString& name, const QString& value)
{
	QStringList props;
	foreach (const QString& prop, element.attribute("style").split(';')) {
		if (!prop.trimmed().isEmpty()) {
			props.append(prop);
		}
	}
	bool found = false;
	for (int i = 0; i < props.size(); ++i) {
		int pos = props[i].indexOf(':');
		if (pos > 0 && props[i].left(pos).trimmed() == name) {
			props[i] = name + ":" + value;
			found = true;
		}
	}
	if (!found) {
		props.append(name + ":" + value);
	}
	element.setAttribute("style", props.join(';'));
}

const QByteArray& logoSource()
{
	static QByteArray source;
	if (source.isEmpty()) {
		QFile file(LOGO_SVG);
		if (file.open(QIODevice::ReadOnly)) {
			source = file.readAll();
		} else {
			qWarning() << "RsLogo: cannot open" << LOGO_SVG;
		}
	}
	return source;
}

/* Returns the logo svg with the colors and the gray (inactive) nodes applied */
QByteArray logoSvg(const QColor& background, const QColor& foreground, int litNodes)
{
	QDomDocument doc;
	if (!doc.setContent(logoSource())) {
		qWarning() << "RsLogo: cannot parse" << LOGO_SVG;
		return QByteArray();
	}

	/* background circle */
	QDomElement circle = findById(doc, LOGO_BACKGROUND_ID);
	if (!circle.isNull()) {
		setStyleProperty(circle, "fill", background.name(QColor::HexRgb));
		setStyleProperty(circle, "fill-opacity", QString::number(background.alphaF(), 'f', 3));
	} else {
		qWarning() << "RsLogo: background circle" << LOGO_BACKGROUND_ID << "not found";
	}

	/* symbol color (gradient used by all highlighted nodes) */
	QDomElement stopLight = findById(doc, LOGO_STOP_LIGHT_ID);
	QDomElement stopDark = findById(doc, LOGO_STOP_DARK_ID);
	if (!stopLight.isNull() && !stopDark.isNull()) {
		setStyleProperty(stopLight, "stop-color", lightColor(foreground).name(QColor::HexRgb));
		setStyleProperty(stopDark, "stop-color", foreground.name(QColor::HexRgb));
		setStyleProperty(stopLight, "stop-opacity", QString::number(foreground.alphaF(), 'f', 3));
		setStyleProperty(stopDark, "stop-opacity", QString::number(foreground.alphaF(), 'f', 3));
	} else {
		qWarning() << "RsLogo: symbol gradient stops not found";
	}

	/* gray out the nodes which are not highlighted */
	QDomNodeList defsList = doc.elementsByTagName("defs");
	if (defsList.isEmpty()) {
		return doc.toByteArray();
	}
	QDomElement defs = defsList.at(0).toElement();

	QDomElement grayStops = doc.createElement("linearGradient");
	grayStops.setAttribute("id", LOGO_GRAY_STOPS_ID);
	const char *const grayColors[] = { "#e6e6e6", "#6e6e6e" };
	for (int i = 0; i < 2; ++i) {
		QDomElement stop = doc.createElement("stop");
		stop.setAttribute("offset", QString::number(i));
		stop.setAttribute("style", QString("stop-color:%1;stop-opacity:1").arg(grayColors[i]));
		grayStops.appendChild(stop);
	}
	defs.appendChild(grayStops);

	for (int node = qMax(litNodes, 0); node < 3; ++node) {
		QDomElement path = findById(doc, sNodeIds[node]);
		if (path.isNull()) {
			continue;
		}

		/* fill:url(#gradientId) */
		QString fill = styleProperty(path, "fill");
		if (!fill.startsWith("url(#") || !fill.endsWith(")")) {
			continue;
		}
		QString gradientId = fill.mid(5, fill.length() - 6);

		QDomElement gradient = findById(doc, gradientId);
		if (gradient.isNull()) {
			continue;
		}

		/* same geometry, gray colors */
		QDomElement grayGradient = gradient.cloneNode(true).toElement();
		grayGradient.setAttribute("id", gradientId + "_gray");
		grayGradient.setAttribute("xlink:href", QString("#") + LOGO_GRAY_STOPS_ID);
		defs.appendChild(grayGradient);

		setStyleProperty(path, "fill", QString("url(#%1_gray)").arg(gradientId));
	}

	return doc.toByteArray();
}

QPixmap renderSvg(const QByteArray& svg, int size)
{
	if (svg.isEmpty()) {
		return QPixmap();
	}

	QSvgRenderer renderer(svg);
	if (!renderer.isValid()) {
		qWarning() << "RsLogo: cannot render the logo svg";
		return QPixmap();
	}

	/* the symbol is not exactly square, keep the aspect ratio and center it */
	QSizeF svgSize = renderer.viewBoxF().size();
	if (svgSize.isEmpty()) {
		svgSize = QSizeF(size, size);
	}
	svgSize.scale(size, size, Qt::KeepAspectRatio);
	QRectF target((size - svgSize.width()) / 2.0, (size - svgSize.height()) / 2.0, svgSize.width(), svgSize.height());

	QImage image(size, size, QImage::Format_ARGB32_Premultiplied);
	image.fill(Qt::transparent);
	QPainter painter(&image);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setRenderHint(QPainter::SmoothPixmapTransform);
	renderer.render(&painter, target);
	painter.end();

	return QPixmap::fromImage(image);
}

/* Image of the stock logo, used to detect windows using it as window icon */
const QImage& stockLogoImage()
{
	static QImage image = QIcon(LOGO_FALLBACK).pixmap(32, 32).toImage();
	return image;
}

bool usesStockLogo(QWidget *widget)
{
	if (widget->property(LOGO_WINDOW_PROPERTY).toBool()) {
		return true;
	}

	if (!widget->testAttribute(Qt::WA_SetWindowIcon)) {
		/* uses the application icon */
		return false;
	}

	QIcon icon = widget->windowIcon();
	if (icon.isNull() || stockLogoImage().isNull()) {
		return false;
	}

	return icon.pixmap(32, 32).toImage() == stockLogoImage();
}

void updateWindowIcon(QWidget *widget)
{
	if (!widget || !widget->isWindow()) {
		return;
	}

	if (usesStockLogo(widget)) {
		widget->setProperty(LOGO_WINDOW_PROPERTY, true);
		widget->setWindowIcon(RsLogo::icon());
	}
}

/* Replaces the stock logo window icon of windows when they are shown */
class RsLogoWindowIconFilter : public QObject
{
public:
	explicit RsLogoWindowIconFilter(QObject *parent) : QObject(parent) {}

protected:
	bool eventFilter(QObject *object, QEvent *event) override
	{
		if (event->type() == QEvent::Show && object->isWidgetType()) {
			QWidget *widget = static_cast<QWidget*>(object);
			if (widget->isWindow()) {
				updateWindowIcon(widget);
			}
		}
		return QObject::eventFilter(object, event);
	}
};

} // namespace

QColor RsLogo::defaultBackgroundColor()
{
	return QColor(Qt::black);
}

QColor RsLogo::defaultForegroundColor()
{
	return QColor(0x0e, 0x8e, 0xcb);
}

QColor RsLogo::backgroundColor()
{
	if (!Settings) {
		return defaultBackgroundColor();
	}
	return Settings->getLogoBackgroundColor();
}

QColor RsLogo::foregroundColor()
{
	if (!Settings) {
		return defaultForegroundColor();
	}
	return Settings->getLogoForegroundColor();
}

QIcon RsLogo::icon(int litNodes)
{
	return icon(backgroundColor(), foregroundColor(), litNodes);
}

QIcon RsLogo::icon(const QColor& background, const QColor& foreground, int litNodes)
{
	QString key = colorKey(background, foreground, litNodes);
	QHash<QString, QIcon>::const_iterator it = sIconCache.constFind(key);
	if (it != sIconCache.constEnd()) {
		return it.value();
	}

	QByteArray svg = logoSvg(background, foreground, litNodes);

	QIcon result;
	for (int size : sIconSizes) {
		QPixmap pixmap = renderSvg(svg, size);
		if (!pixmap.isNull()) {
			result.addPixmap(pixmap);
		}
	}

	if (result.isNull()) {
		/* svg rendering failed, use the stock logo */
		qWarning() << "RsLogo: using fallback logo" << LOGO_FALLBACK;
		result = QIcon(LOGO_FALLBACK);
	}

	sIconCache.insert(key, result);
	return result;
}

QPixmap RsLogo::pixmap(int size, int litNodes)
{
	return pixmap(backgroundColor(), foregroundColor(), size, litNodes);
}

QPixmap RsLogo::pixmap(const QColor& background, const QColor& foreground, int size, int litNodes)
{
	return icon(background, foreground, litNodes).pixmap(size, size);
}

void RsLogo::applyToApplication()
{
	static RsLogoWindowIconFilter *filter = nullptr;
	if (!filter && qApp) {
		filter = new RsLogoWindowIconFilter(qApp);
		qApp->installEventFilter(filter);

		/* release the cached pixmaps before the application is destroyed */
		QObject::connect(qApp, &QCoreApplication::aboutToQuit, filter, []() {
			sIconCache.clear();
		});
	}

	/* the colors may have changed, keep only the icons of the current colors */
	sIconCache.clear();

	QApplication::setWindowIcon(icon());

	foreach (QWidget *widget, QApplication::topLevelWidgets()) {
		updateWindowIcon(widget);
	}
}

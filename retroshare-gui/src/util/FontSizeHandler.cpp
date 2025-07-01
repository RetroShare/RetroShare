/*******************************************************************************
 * util/FontSizeHandler.cpp                                                    *
 *                                                                             *
 * Copyright (C) 2025, Retroshare Team <retroshare.project@gmail.com>          *
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

#include <QMap>
#include <QWidget>
#include <QAbstractItemView>

#include "rshare.h"
#include "FontSizeHandler.h"
#include "gui/settings/rsharesettings.h"
#include "gui/notifyqt.h"

// Data for QAbstractItemView
struct FontSizeHandlerWidgetData
{
	std::function<void(QWidget*, int)> callback;
};

// Data for QWidget
struct FontSizeHandlerViewData
{
	float iconHeightFactor;
	std::function<void(QAbstractItemView*, int)> callback;
};

class FontSizeHandlerObject : public QObject
{
public:
	FontSizeHandlerObject(FontSizeHandlerBase *fontSizeHandler): QObject()
	{
		mFontSizeHandlerBase = fontSizeHandler;
	}

	bool eventFilter(QObject* object, QEvent* event)
	{
		if (event->type() == QEvent::StyleChange) {
			QMap<QAbstractItemView*, FontSizeHandlerViewData>::iterator itView = mView.find((QAbstractItemView*) object);
			if (itView != mView.end()) {
				mFontSizeHandlerBase->updateFontSize(itView.key(), itView.value().iconHeightFactor, itView.value().callback);
			}

			QMap<QWidget*, FontSizeHandlerWidgetData>::iterator itWidget = mWidget.find((QWidget*) object);
			if (itWidget != mWidget.end()) {
				mFontSizeHandlerBase->updateFontSize(itWidget.key(), itWidget.value().callback);
			}
		}

		return false;
	}

	void registerFontSize(QWidget *widget, std::function<void(QWidget*, int)> callback)
	{
		FontSizeHandlerWidgetData data;
		data.callback = callback;
		mWidget.insert(widget, data);

		QObject::connect(NotifyQt::getInstance(), &NotifyQt::settingsChanged, widget, [this, widget, callback]() {
			mFontSizeHandlerBase->updateFontSize(widget, callback);
		});

		widget->installEventFilter(this);
		QObject::connect(widget, &QObject::destroyed, this, [this, widget](QObject *object) {
			if (widget == object) {
				mWidget.remove(widget);
				widget->removeEventFilter(this);
			}
		});

		mFontSizeHandlerBase->updateFontSize(widget, callback, true);
	}

	void registerFontSize(QAbstractItemView *view, float iconHeightFactor, std::function<void(QAbstractItemView*, int)> callback)
	{
		FontSizeHandlerViewData data;
		data.iconHeightFactor = iconHeightFactor;
		data.callback = callback;
		mView.insert(view, data);

		QObject::connect(NotifyQt::getInstance(), &NotifyQt::settingsChanged, view, [this, view, iconHeightFactor, callback]() {
			mFontSizeHandlerBase->updateFontSize(view, iconHeightFactor, callback);
		});

		view->installEventFilter(this);
		QObject::connect(view, &QObject::destroyed, this, [this, view](QObject *object) {
			if (view == object) {
				mView.remove(view);
				view->removeEventFilter(this);
			}
		});

		mFontSizeHandlerBase->updateFontSize(view, iconHeightFactor, callback, true);
	}

private:
	FontSizeHandlerBase *mFontSizeHandlerBase;
	QMap<QAbstractItemView*, FontSizeHandlerViewData> mView;
	QMap<QWidget*, FontSizeHandlerWidgetData> mWidget;
};

FontSizeHandlerBase::FontSizeHandlerBase(Type type)
{
	mType = type;
	mObject = nullptr;
}

FontSizeHandlerBase::~FontSizeHandlerBase()
{
	if (mObject) {
		mObject->deleteLater();
		mObject = nullptr;
	}
}

int FontSizeHandlerBase::getFontSize()
{
	switch (mType) {
	case FONT_SIZE:
		return Settings->getFontSize();

	case MESSAGE_FONT_SIZE:
		return Settings->getMessageFontSize();

	case FORUM_FONT_SIZE:
		return Settings->getForumFontSize();
	}

	return 0;
}

void FontSizeHandlerBase::registerFontSize(QWidget *widget, std::function<void(QWidget*, int)> callback)
{
	if (!widget) {
		return;
	}

	if (!mObject) {
		mObject = new FontSizeHandlerObject(this);
	}

	mObject->registerFontSize(widget, callback);
}

void FontSizeHandlerBase::registerFontSize(QAbstractItemView *view, float iconHeightFactor, std::function<void(QAbstractItemView*, int)> callback)
{
	if (!view) {
		return;
	}

	if (!mObject) {
		mObject = new FontSizeHandlerObject(this);
	}

	mObject->registerFontSize(view, iconHeightFactor, callback);
}

void FontSizeHandlerBase::updateFontSize(QWidget *widget, std::function<void (QWidget *, int)> callback, bool force)
{
	if (!widget) {
		return;
	}

	int fontSize = getFontSize();
	QFont font = widget->font();
	if (force || font.pointSize() != fontSize) {
		font.setPointSize(fontSize);
		widget->setFont(font);

		if (callback) {
			callback(widget, fontSize);
		}
	}
}

void FontSizeHandlerBase::updateFontSize(QAbstractItemView *view, float iconHeightFactor, std::function<void (QAbstractItemView *, int)> callback, bool force)
{
	if (!view) {
		return;
	}

	int fontSize = getFontSize();
	QFont font = view->font();
	if (force || font.pointSize() != fontSize) {
		font.setPointSize(fontSize);
		view->setFont(font);

		if (iconHeightFactor) {
			QFontMetricsF fontMetrics(font);
			int iconHeight = fontMetrics.height() * iconHeightFactor;
			view->setIconSize(QSize(iconHeight, iconHeight));
		}

		if (callback) {
			callback(view, fontSize);
		}
	}
}

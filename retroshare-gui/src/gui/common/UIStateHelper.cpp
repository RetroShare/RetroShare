/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2013 RetroShare Team
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

#include <QLabel>
#include <QWidget>
#include <QLineEdit>

#include "UIStateHelper.h"
#include "RSTreeWidget.h"
#include "LinkTextBrowser.h"

class UIStateHelperObject
{
public:
	UIStateHelperObject(QLabel *widget)
	{
		init();
		mLabel = widget;
	}
	UIStateHelperObject(QLineEdit *widget)
	{
		init();
		mLineEdit = widget;
	}
	UIStateHelperObject(RSTreeWidget *widget)
	{
		init();
		mTreeWidget = widget;
	}
	UIStateHelperObject(LinkTextBrowser *widget)
	{
		init();
		mLinkTextBrowser = widget;
	}

	void setPlaceholder(bool loading, const QString &text, bool clear) const
	{
		if (mLabel) {
			mLabel->setText(text);
		}

		if (mLineEdit) {
			if (loading && clear) {
				mLineEdit->clear();
			}

#if QT_VERSION >= QT_VERSION_CHECK (4, 8, 0)
			mLineEdit->setPlaceholderText(text);
#else
			mLineEdit->setText(text);
#endif
		}

		if (mTreeWidget) {
			if (loading && clear) {
				mTreeWidget->clear();
			}

			mTreeWidget->setPlaceholderText(text);
		}

		if (mLinkTextBrowser) {
			if (loading && clear) {
				mLinkTextBrowser->clear();
			}

			mLinkTextBrowser->setPlaceholderText(text);
		}
	}

	void clear()
	{
		if (mLabel) {
			mLabel->clear();
		}

		if (mLineEdit) {
			mLineEdit->clear();
		}

		if (mTreeWidget) {
			mTreeWidget->clear();
		}

		if (mLinkTextBrowser) {
			mLinkTextBrowser->clear();
		}
	}

	bool operator ==(const UIStateHelperObject &data) const
	{
		if (mLabel == data.mLabel &&
			mLineEdit == data.mLineEdit &&
			mTreeWidget == data.mTreeWidget &&
			mLinkTextBrowser == data.mLinkTextBrowser) {
			return true;
		}

		return false;
	}

	bool operator <(const UIStateHelperObject &data) const
	{
		if (mLabel < data.mLabel ||
			mLineEdit < data.mLineEdit ||
			mTreeWidget < data.mTreeWidget ||
			mLinkTextBrowser < data.mLinkTextBrowser) {
			return true;
		}

		return false;
	}

private:
	void init()
	{
		mLabel = NULL;
		mLineEdit = NULL;
		mTreeWidget = NULL;
		mLinkTextBrowser = NULL;
	}

private:
	QLabel *mLabel;
	QLineEdit *mLineEdit;
	RSTreeWidget *mTreeWidget;
	LinkTextBrowser *mLinkTextBrowser;
};

class UIStateHelperData
{
public:
	UIStateHelperData()
	{
		mLoading = false;
		mActive = true;
	}

public:
	bool mLoading;
	bool mActive;
	QMap<QWidget*, UIStates> mWidgets;
	QMap<UIStateHelperObject, QPair<QString, bool> > mLoad;
	QList<UIStateHelperObject> mClear;
};

UIStateHelper::UIStateHelper(QObject *parent) :
	QObject(parent)
{
}

UIStateHelper::~UIStateHelper()
{
	QMap<long, UIStateHelperData*>::iterator it;
	for (it = mData.begin(); it != mData.end(); ++it) {
		delete(it.value());
	}
}

UIStateHelperData *UIStateHelper::findData(int index, bool create)
{
	UIStateHelperData *data = NULL;

	QMap<long, UIStateHelperData*>::iterator it = mData.find(index);
	if (it == mData.end()) {
		if (create) {
			data = new UIStateHelperData;
			mData[index] = data;
		}
	} else {
		data = it.value();
	}

	return data;
}

void UIStateHelper::addWidget(int index, QWidget *widget, UIStates states)
{
	UIStateHelperData *data = findData(index, true);
	data->mWidgets.insert(widget, states);
}

void UIStateHelper::addLoadPlaceholder(int index, QLabel *widget, bool clear, const QString &text)
{
	UIStateHelperData *data = findData(index, true);
	data->mLoad.insert(UIStateHelperObject(widget), QPair<QString, bool>(text.isEmpty() ? tr("Loading") : text, clear));
}

void UIStateHelper::addLoadPlaceholder(int index, QLineEdit *widget, bool clear, const QString &text)
{
	UIStateHelperData *data = findData(index, true);
	data->mLoad.insert(UIStateHelperObject(widget), QPair<QString, bool>(text.isEmpty() ? tr("Loading") : text, clear));
}

void UIStateHelper::addLoadPlaceholder(int index, RSTreeWidget *widget, bool clear, const QString &text)
{
	UIStateHelperData *data = findData(index, true);
	data->mLoad.insert(UIStateHelperObject(widget), QPair<QString, bool>(text.isEmpty() ? tr("Loading") : text, clear));
}

void UIStateHelper::addLoadPlaceholder(int index, LinkTextBrowser *widget, bool clear, const QString &text)
{
	UIStateHelperData *data = findData(index, true);
	data->mLoad.insert(UIStateHelperObject(widget), QPair<QString, bool>(text.isEmpty() ? tr("Loading") : text, clear));
}

void UIStateHelper::addClear(int index, QLabel *widget)
{
	UIStateHelperData *data = findData(index, true);
	if (data->mClear.contains(widget)) {
		return;
	}
	data->mClear.push_back(UIStateHelperObject(widget));
}

void UIStateHelper::addClear(int index, QLineEdit *widget)
{
	UIStateHelperData *data = findData(index, true);
	if (data->mClear.contains(widget)) {
		return;
	}
	data->mClear.push_back(UIStateHelperObject(widget));
}

void UIStateHelper::addClear(int index, RSTreeWidget *widget)
{
	UIStateHelperData *data = findData(index, true);
	if (data->mClear.contains(widget)) {
		return;
	}
	data->mClear.push_back(UIStateHelperObject(widget));
}

void UIStateHelper::addClear(int index, LinkTextBrowser *widget)
{
	UIStateHelperData *data = findData(index, true);
	if (data->mClear.contains(widget)) {
		return;
	}
	data->mClear.push_back(UIStateHelperObject(widget));
}

bool UIStateHelper::isWidgetVisible(QWidget *widget)
{
	bool visible = true;

	QMap<QWidget*, bool>::iterator itVisible = mWidgetVisible.find(widget);
	if (itVisible != mWidgetVisible.end()) {
		visible = itVisible.value();
	}

	if (visible) {
		QMap<long, UIStateHelperData*>::iterator dataIt;
		for (dataIt = mData.begin(); dataIt != mData.end(); ++dataIt) {
			UIStateHelperData *data = dataIt.value();
			QMap<QWidget*, UIStates>::iterator it = data->mWidgets.find(widget);
			if (it == data->mWidgets.end()) {
				continue;
			}

			UIStates states = it.value();

			if (states & (UISTATE_LOADING_VISIBLE | UISTATE_LOADING_INVISIBLE)) {
				if (states & UISTATE_LOADING_VISIBLE) {
					if (!data->mLoading) {
						visible = false;
						break;
					}
				} else if (states & UISTATE_LOADING_INVISIBLE) {
					if (data->mLoading) {
						visible = false;
						break;
					}
				}
			}

			if (states & (UISTATE_ACTIVE_VISIBLE | UISTATE_ACTIVE_INVISIBLE)) {
				if (states & UISTATE_ACTIVE_VISIBLE) {
					if (!data->mActive) {
						visible = false;
						break;
					}
				} else if (states & UISTATE_ACTIVE_INVISIBLE) {
					if (data->mActive) {
						visible = false;
						break;
					}
				}
			}
		}
	}

	return visible;
}

bool UIStateHelper::isWidgetEnabled(QWidget *widget)
{
	bool enabled = true;

	QMap<QWidget*, bool>::iterator itEnabled = mWidgetEnabled.find(widget);
	if (itEnabled != mWidgetEnabled.end()) {
		enabled = itEnabled.value();
	}

	if (enabled) {
		QMap<long, UIStateHelperData*>::iterator dataIt;
		for (dataIt = mData.begin(); dataIt != mData.end(); ++dataIt) {
			UIStateHelperData *data = dataIt.value();
			QMap<QWidget*, UIStates>::iterator it = data->mWidgets.find(widget);
			if (it == data->mWidgets.end()) {
				continue;
			}

			UIStates states = it.value();

			if (states & (UISTATE_LOADING_ENABLED | UISTATE_LOADING_DISABLED)) {
				if (states & UISTATE_LOADING_ENABLED) {
					if (!data->mLoading) {
						enabled = false;
						break;
					}
				} else if (states & UISTATE_LOADING_DISABLED) {
					if (data->mLoading) {
						enabled = false;
						break;
					}
				}
			}

			if (states & (UISTATE_ACTIVE_ENABLED | UISTATE_ACTIVE_DISABLED)) {
				if (states & UISTATE_ACTIVE_ENABLED) {
					if (!data->mActive) {
						enabled = false;
						break;
					}
				} else if (states & UISTATE_ACTIVE_DISABLED) {
					if (data->mActive) {
						enabled = false;
						break;
					}
				}
			}
		}
	}

	return enabled;
}

void UIStateHelper::updateData(UIStateHelperData *data)
{
	QMap<QWidget*, UIStates>::iterator it;
	for (it = data->mWidgets.begin(); it != data->mWidgets.end(); ++it) {
		QWidget *widget = it.key();
		UIStates states = it.value();

		if (states & (UISTATE_LOADING_VISIBLE | UISTATE_LOADING_INVISIBLE | UISTATE_ACTIVE_VISIBLE | UISTATE_ACTIVE_INVISIBLE)) {
			widget->setVisible(isWidgetVisible(widget));
		}

		if (states & (UISTATE_LOADING_ENABLED | UISTATE_LOADING_DISABLED | UISTATE_ACTIVE_ENABLED | UISTATE_ACTIVE_DISABLED)) {
			widget->setEnabled(isWidgetEnabled(widget));
		}
	}
}

void UIStateHelper::setLoading(int index, bool loading)
{
	UIStateHelperData *data = findData(index, false);
	if (!data) {
		return;
	}
	data->mLoading = loading;

	updateData(data);

	QMap<UIStateHelperObject, QPair<QString, bool> >::iterator it;
	for (it = data->mLoad.begin(); it != data->mLoad.end(); ++it) {
		if (loading) {
			it.key().setPlaceholder(loading, it.value().first, it.value().second);
		} else {
			it.key().setPlaceholder(loading, "", it.value().second);
		}
	}
}

void UIStateHelper::setActive(int index, bool active)
{
	UIStateHelperData *data = findData(index, false);
	if (!data) {
		return;
	}
	data->mActive = active;

	updateData(data);
}

void UIStateHelper::clear(int index)
{
	UIStateHelperData *data = findData(index, false);
	if (!data) {
		return;
	}
	QList<UIStateHelperObject>::iterator it;
	for (it = data->mClear.begin(); it != data->mClear.end(); ++it) {
		it->clear();
	}
}

bool UIStateHelper::isLoading(int index)
{
	UIStateHelperData *data = findData(index, false);
	if (data) {
		return data->mLoading;
	}

	return false;
}

bool UIStateHelper::isActive(int index)
{
	UIStateHelperData *data = findData(index, false);
	if (data) {
		return data->mActive;
	}

	return true;
}

void UIStateHelper::setWidgetVisible(QWidget *widget, bool visible)
{
	mWidgetVisible[widget] = visible;

	QMap<long, UIStateHelperData*>::iterator dataIt;
	for (dataIt = mData.begin(); dataIt != mData.end(); ++dataIt) {
		updateData(*dataIt);
	}
}

void UIStateHelper::setWidgetEnabled(QWidget *widget, bool enabled)
{
	mWidgetEnabled[widget] = enabled;

	QMap<long, UIStateHelperData*>::iterator dataIt;
	for (dataIt = mData.begin(); dataIt != mData.end(); ++dataIt) {
		updateData(*dataIt);
	}
}

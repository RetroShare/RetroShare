/*
 * RetroShare
 * Copyright (C) 2021  Gioacchino Mazzurco <gio@eigenlab.org>
 * Copyright (C) 2021  Asociación Civil Altermundi <info@altermundi.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-FileCopyrightText: Retroshare Team <contact@retroshare.cc>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

package org.retroshare.service;

public class ErrorConditionWrap
{
    public ErrorConditionWrap(
        int value, String message, String categoryName )
    {
        mValue = value;
        mMessage = message;
        mCategoryName = categoryName;
    }

    public int value() { return mValue; }
    public String message() { return mMessage; }
    public String categoryName() { return mCategoryName; }

    public boolean toBool() { return mValue != 0; }

    @Override
    public String toString()
    { return String.format("%d", mValue)+" "+mMessage+" [" + mCategoryName+ "]"; }

    private int mValue = 0;
    private String mMessage;
    private String mCategoryName;
}

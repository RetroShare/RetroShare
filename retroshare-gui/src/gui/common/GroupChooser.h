/*******************************************************************************
 * gui/common/GroupChooser.h                                                   *
 *                                                                             *
 * Copyright (C) 2013, Robert Fernie   <retroshare.project@gmail.com>          *
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

#include "gui/common/RSComboBox.h"

#include <retroshare/rspeers.h>

class GroupChooser : public RSComboBox
{
public:
	GroupChooser(QWidget *parent = nullptr);

    void loadGroups(uint32_t chooserFlags, const RsNodeGroupId& defaultId);
    bool getChosenGroup(RsNodeGroupId& id);

    static bool makeNodeGroupDesc(const RsGroupInfo& info, QString &desc);
private:
    void loadGroups();
    uint32_t mFlags;

    RsNodeGroupId mDefaultGroupId;
};


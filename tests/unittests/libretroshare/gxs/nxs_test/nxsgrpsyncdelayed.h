/*******************************************************************************
 * unittests/libretroshare/gxs/nxs_test/nxsgrpsyncdelayed.h                    *
 *                                                                             *
 * Copyright (C) 2014, Crispy <retroshare.team@gmailcom>                       *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 ******************************************************************************/

#ifndef NXSGRPSYNCDELAYED_H_
#define NXSGRPSYNCDELAYED_H_

#include "nxsgrpsync_test.h"

namespace rs_nxs_test {

class NxsGrpSyncDelayed : public NxsGrpSync {
public:
	NxsGrpSyncDelayed();
	virtual ~NxsGrpSyncDelayed();
};

} /* namespace rs_nxs_test */
#endif /* NXSGRPSYNCDELAYED_H_ */

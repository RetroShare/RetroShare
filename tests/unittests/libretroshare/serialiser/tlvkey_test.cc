/*******************************************************************************
 * unittests/libretroshare/serialiser/tlvkey_test.cc                           *
 *                                                                             *
 * Copyright 2007-2008 by Robert Fernie <retroshare.project@gmail.com>         *
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


#include <gtest/gtest.h>

#include "support.h"
#include "serialiser/rstlvkeys.h"

TEST(libretroshare_serialiser, test_RsTlvKeySignatureSet)
{
    RsTlvKeySignatureSet set;

    init_item(set);

    char data[set.TlvSize()];
    uint32_t offset = 0;
    set.SetTlv(data, set.TlvSize(), &offset);

    RsTlvKeySignatureSet setConfirm;

    offset = 0;
    setConfirm.GetTlv(data, set.TlvSize(), &offset);

    EXPECT_TRUE(setConfirm == set);

}

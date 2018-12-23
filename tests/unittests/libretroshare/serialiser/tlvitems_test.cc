/*******************************************************************************
 * unittests/libretroshare/serialiser/tlvitems_test.cc                         *
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

#include <string.h>
#include <iostream>
#include "serialiser/rstlvbinary.h"

#include "rstlvutil.h"

#define BIN_LEN 65536  /* bigger than 64k */

TEST(libretroshare_serialiser, test_RsTlvBinData2)
{
	RsTlvBinaryData  d1(1023);
	RsTlvBinaryData  d2(1023);

	char data[BIN_LEN] = {0};
	int i, j;
	for(i = 0; i < BIN_LEN; i++)
	{
		data[i] = i%13;
	}

	for(j = 1; j < BIN_LEN; j *= 2)
	{
		d1.setBinData(data, j);
		EXPECT_TRUE(test_SerialiseTlvItem(std::cerr, &d1, &d2));

		EXPECT_TRUE(d1.bin_len == d2.bin_len);
		EXPECT_TRUE(0 == memcmp(d1.bin_data, d2.bin_data, d1.bin_len));
	}

}




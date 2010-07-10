/*
 * bitdht/bencode_test.cc
 *
 * BitDHT: An Flexible DHT library.
 *
 * Copyright 2010 by Robert Fernie
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 3 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "bitdht@lunamutt.com".
 *
 */


#include "bitdht/bencode.h"
#include <stdio.h>

int main(int argc, char **argv)
{

	char msg[100];

	msg[0] = 'd';
	msg[1] = '1';
	msg[2] = ':';
	msg[3] = 't';
	msg[4] = 'L';
	msg[5] = 'b';
	msg[6] = 'U';
	msg[7] = 0xd9;
	msg[8] = 0xfa;
	msg[9] = 0xff;
	msg[10] = 0xff;
	msg[11] = 0xff;
	msg[12] = 0xff;
	msg[13] = '\n';
	msg[14] = 'H';
	msg[15] = '#';
	msg[16] = '#';
	msg[17] = '#';
	msg[18] = '#';
	msg[19] = '#';
	msg[20] = '#';
	msg[21] = '#';

	be_node *n = be_decoden(msg, 16);

	if (n)
	{
		be_dump(n);
		be_free(n);
	}
	else
	{
		fprintf(stderr, "didn't crash!\n");
	}

	msg[0] = 'd';
	msg[1] = '1';
	msg[2] = ':';
	msg[3] = 'a';
	msg[4] = 'L';
	msg[5] = 0x8d;
	msg[6] = 0xd6;
	msg[7] = '\r';
	msg[8] = 0x9d;
	msg[9] = ';';
	msg[10] = 0xff;
	msg[11] = 0xff;
	msg[12] = 0xff;
	msg[13] = 0xff;
	msg[14] = 'H';
	msg[15] = '#';
	msg[16] = '#';
	msg[17] = '#';
	msg[18] = '#';
	msg[19] = '#';
	msg[20] = '#';
	msg[21] = '#';

	n = be_decoden(msg, 14);

	if (n)
	{
		be_dump(n);
		be_free(n);
	}
	else
	{
		fprintf(stderr, "didn't crash!\n");
	}

	msg[0] = 'd';
	msg[1] = '1';
	msg[2] = ':';
	msg[3] = 't';
	msg[4] = '4';
	msg[5] = ':';
	msg[6] = 'a';
	msg[7] = 'b';
	msg[8] = 'c';
	msg[9] = 'd';
	msg[10] = '1';
	msg[11] = ':';
	msg[12] = 'y';
	msg[13] = '1';
	msg[14] = ':';
	msg[15] = 'r';
	msg[16] = '1';
	msg[17] = ':';
	msg[18] = 'r';
	msg[19] = 'd';
	msg[20] = '2';
	msg[21] = ':';
	msg[22] = 'i';
	msg[23] = 'd';
	msg[24] = '2';
	msg[25] = '0';
	msg[26] = ':';
	msg[27] = '1';
	msg[28] = '2';
	msg[29] = '3';
	msg[30] = '4';
	msg[31] = '5';
	msg[32] = '6';
	msg[33] = '7';
	msg[34] = '8';
	msg[35] = '9';
	msg[36] = '0';
	msg[37] = 'a';
	msg[38] = 'b';
	msg[39] = 'c';
	msg[40] = 'd';
	msg[41] = 'e';
	msg[42] = 'f';
	msg[43] = 'g';
	msg[44] = 'h';
	msg[45] = 'i';
	msg[46] = '.';
	msg[47] = '5';
	msg[48] = ':';
	msg[49] = 'n';
	msg[50] = 'o';
	msg[51] = 'd';
	msg[52] = 'e';
	msg[53] = 's';
	msg[54] = '2';
	msg[55] = '0';
	msg[56] = '8';
	msg[57] = ':';
	msg[58] = '\0';
	msg[59] = '\0';
	msg[60] = '\0';

	n = be_decoden(msg, 58);

	if (n)
	{
		be_dump(n);
		be_free(n);
	}
	else
	{
		fprintf(stderr, "didn't crash!\n");
	}

	return 1;
}



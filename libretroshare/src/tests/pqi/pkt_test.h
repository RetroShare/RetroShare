#ifndef RS_TEST_PACKETS
#define RS_TEST_PACKETS
/*
 * libretroshare/src/tests/pqi pkt_tst.cc
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2009-2010 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2 as published by the Free Software Foundation.
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
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */

/******
 * pkt_tst.
 * 
 * This is not testing serialisation.... so we only need 1 type of packet.
 * using FileData packets - as these can have arbitary size.
 */

#include "serialiser/rsbaseitems.h"

#define TEST_PKT_SIZE 1234567
#define TEST_PKT_HASH "HASHTESTHASH"
#define TEST_PKT_OFFSET 1234

#define TEST_DATA_LEN (1024 * 1024)  // MB

void generate_test_data();
bool check_data(void *data_in, int len);

RsFileData *gen_packet(int datasize, std::string pid);
bool check_packet(RsFileData *pkt);


#endif

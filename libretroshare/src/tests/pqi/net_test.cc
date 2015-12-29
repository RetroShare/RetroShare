/*
 * libretroshare/src/test/pqi net_test.cc
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2007-2010 by Robert Fernie.
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
 * NETWORKING Test to check Big/Little Endian behaviour
 * as well as socket behaviour
 *
 */

#include "util/utest.h"

#include "pqi/pqinetwork.h"
#include "util/rsnet.h"
#include <iostream>
#include <sstream>


bool test_byte_manipulation();
bool test_local_address_manipulation();


INITTEST();

int main(int argc, char **argv)
{
	std::cerr << "libretroshare:pqi " << argv[0] << std::endl;

	test_byte_manipulation();

	FINALREPORT("libretroshare Net Basics Tests");
	return TESTRESULT();
}

	/* test 1: byte manipulation */
bool test_byte_manipulation()
{
	uint64_t num1 = 0x0000000000000000ffULL; /* 255 */
	uint64_t num2 = 0x00000000000000ff00ULL; /*  */

	uint64_t n_num1 = htonll(num1);
	uint64_t n_num2 = htonll(num2);

	uint64_t h_num1 = ntohll(n_num1);
	uint64_t h_num2 = ntohll(n_num2);

	std::ostringstream out;
	out << std::hex;
	out << "num1: " << num1 << " netOrder: " << n_num1 << " hostOrder: " << h_num1 << std::endl;
	out << "num2: " << num2 << " netOrder: " << n_num2 << " hostOrder: " << h_num2 << std::endl;
	
	std::cerr << out.str();

        CHECK( num1 == h_num1 );
        CHECK( num2 == h_num2 );

        REPORT("Test Byte Manipulation");

	return true;
}

const char * loopback_addrstr = "127.0.0.1";
const char * localnet1_addrstr = "192.168.0.1";
const char * localnet2_addrstr = "10.0.0.1";
const char * localnet3_addrstr = "10.5.63.78";
const char * localnet4_addrstr = "192.168.74.91";

bool test_bind_addr(struct sockaddr_in addr);

bool test_bind_addr(struct sockaddr_in addr)
{

	int err;

        std::cerr << "test_bind_addr()";
        std::cerr << std::endl;

        std::cerr << "\tAddress Family: " << (int) addr.sin_family;
        std::cerr << std::endl;
        std::cerr << "\tAddress: " << rs_inet_ntoa(addr.sin_addr);
        std::cerr << std::endl;
        std::cerr << "\tPort: " << ntohs(addr.sin_port);
        std::cerr << std::endl;

	int sockfd = unix_socket(PF_INET, SOCK_STREAM, 0);
        CHECK( 0 < sockfd );
	
        if (0 != (err = bind(sockfd, (struct sockaddr *) &addr, sizeof(addr))))
        {
                std::cerr <<  " Failed to Bind to Local Address!" << std::endl;
					 std::string out ;
                showSocketError(out) ;
					 std::cerr << out << std::endl;

        	FAILED("Couldn't Bind to socket");

		return false;
        }

        std::cerr <<  " Successfully Bound Socket to Address" << std::endl;
        unix_close(sockfd);
	
	return true;
}

	

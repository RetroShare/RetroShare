#ifndef PPG_HARNESS_TEST_H
#define PPG_HARNESS_TEST_H

/*
 * libretroshare/src/test/pqi ppg_harness.cc
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2007-2008 by Robert Fernie.
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
 * test harness for pqipersongrp / pqihandler.
 */

#include "pqi/pqiperson.h"
#include "pqi/pqipersongrp.h"

/*******************************************************
 *
 * Test structure
 *****************/

void setupPqiPersonGrpTH();
void tickPqiPersonGrpTH();

class pqipersongrpTestHarness;
extern pqipersongrpTestHarness *mPqiPersonGrpTH;

class pqipersongrpTestHarness: public pqipersongrp
{
        public:
        pqipersongrpTestHarness(SecurityPolicy *pol, unsigned long flags);

        protected:

        /********* FUNCTIONS to OVERLOAD for specialisation ********/
virtual pqilistener *createListener(struct sockaddr_in laddr);
virtual pqiperson   *createPerson(std::string id, pqilistener *listener);

        /********* FUNCTIONS to OVERLOAD for specialisation ********/
};

#endif

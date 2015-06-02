/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (c) 2015, RetroShare Team
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

#include <QCoreApplication>
#include <retroshare/rsbanlist.h>

#include "RsBanListDefs.h"

//const QString RsBanListDefs::resultState(uint32_t result)
//{
//	switch (result) {
//	case RSBANLIST_CHECK_RESULT_UNKNOWN:
//		break;
//	case RSBANLIST_CHECK_RESULT_NOCHECK:
//		return QCoreApplication::translate("RsBanListDefs", "not checked");
//	case RSBANLIST_CHECK_RESULT_BLACKLISTED:
//		return QCoreApplication::translate("RsBanListDefs", "blacklisted");
//	case RSBANLIST_CHECK_RESULT_NOT_WHITELISTED:
//		return QCoreApplication::translate("RsBanListDefs", "not whitelisted");
//	case RSBANLIST_CHECK_RESULT_ACCEPTED:
//		return QCoreApplication::translate("RsBanListDefs", "accepted");
//	}

//	return QCoreApplication::translate("RsBanListDefs", "Unknown");
//}

const QString RsBanListDefs::resultString(uint32_t result)
{
	switch (result) {
	case RSBANLIST_CHECK_RESULT_UNKNOWN:
		break;
	case RSBANLIST_CHECK_RESULT_NOCHECK:
		return QCoreApplication::translate("RsBanListDefs", "IP address not checked");
	case RSBANLIST_CHECK_RESULT_BLACKLISTED:
		return QCoreApplication::translate("RsBanListDefs", "IP address is blacklisted");
	case RSBANLIST_CHECK_RESULT_NOT_WHITELISTED:
		return QCoreApplication::translate("RsBanListDefs", "IP address is not whitelisted");
	case RSBANLIST_CHECK_RESULT_ACCEPTED:
		return QCoreApplication::translate("RsBanListDefs", "IP address accepted");
	}

	return QCoreApplication::translate("RsBanListDefs", "Unknown");
}

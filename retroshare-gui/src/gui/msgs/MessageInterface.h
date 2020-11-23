/*******************************************************************************
 * retroshare-gui/src/gui/msgs/MessageInterface.h                              *
 *                                                                             *
 * Copyright (C) 2007 by Retroshare Team     <retroshare.project@gmail.com>    *
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

#ifndef MRK_MESSAGE_INTERFACE_H
#define MRK_MESSAGE_INTERFACE_H

/* This header will be used to switch the GUI from 
 * the old RsMsg to the new RsMail interface.
 *
 * The GUI uses similar interface, with the differences
 * specified here.
 *
 */

#define USE_OLD_MAIL 	1

#ifdef USE_OLD_MAIL

    #include <retroshare/rsmsgs.h>

    using namespace Rs::Msgs;

    #define rsMail rsMsgs

#else

    #include <retroshare/rsmail.h>

    using namespace Rs::Mail;

#endif


#endif // MRK_MESSAGE_INTERFACE

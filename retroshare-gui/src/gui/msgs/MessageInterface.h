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

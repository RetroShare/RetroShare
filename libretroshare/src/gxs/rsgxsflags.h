#ifndef RSGXSFLAGS_H
#define RSGXSFLAGS_H

#include "inttypes.h"

// this serves a single point of call for definining grp and msg modes
// GXS. These modes say
namespace GXS_SERV {


    /*** GROUP FLAGS ***/

    /* type of group */

    static const uint32_t FLAG_GRP_TYPE_MASK = 0;

    // pub key encrypted
    static const uint32_t FLAG_GRP_TYPE_PRIVATE = 0;

    // single publisher, read only
    static const uint32_t FLAG_GRP_TYPE_RESTRICTED = 0;

    // anyone can publish
    static const uint32_t FLAG_GRP_TYPE_PUBLIC = 0;


    /* type of msgs allowed */

    static const uint32_t FLAG_MSG_TYPE_MASK = 0;

    // only signee can edit, and sign required
    static const uint32_t FLAG_MSG_TYPE_SIGNED = 0;

    // no sign required, but signee can edit if signed
    static const uint32_t FLAG_MSG_TYPE_ANON = 0;

    // anyone can mod but sign must be provided (needed for wikis)
    static const uint32_t FLAG_MSG_TYPE_SIGNED_SHARED = 0;

    /*** GROUP FLAGS ***/



    /*** MESSAGE FLAGS ***/

    // indicates message edits an existing message
    static const uint32_t FLAG_MSG_EDIT = 0;

    // indicates msg is id signed
    static const uint32_t FLAG_MSG_ID_SIGNED = 0;

    /*** MESSAGE FLAGS ***/


    // Subscription Flags. (LOCAL)

    static const uint32_t GROUP_SUBSCRIBE_ADMIN = 0x00000001;
    static const uint32_t GROUP_SUBSCRIBE_PUBLISH = 0x00000002;
    static const uint32_t GROUP_SUBSCRIBE_SUBSCRIBED = 0x00000004;
    static const uint32_t GROUP_SUBSCRIBE_MONITOR = 0x00000008;
    static const uint32_t GROUP_SUBSCRIBE_MASK = 0x0000000f;

}


#endif // RSGXSFLAGS_H

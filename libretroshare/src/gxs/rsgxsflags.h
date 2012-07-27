#ifndef RSGXSFLAGS_H
#define RSGXSFLAGS_H

#include "inttypes.h"

namespace GXS_SERV {


    /*** GROUP FLAGS ***/

    /* type of group */

    static const uint32_t FLAG_GRP_TYPE_MASK;

    // pub key encrypted
    static const uint32_t FLAG_GRP_TYPE_PRIVATE;

    // single publisher, read only
    static const uint32_t FLAG_GRP_TYPE_RESTRICTED;

    // anyone can publish
    static const uint32_t FLAG_GRP_TYPE_PUBLIC;


    /* type of msgs allowed */

    static const uint32_t FLAG_MSG_TYPE_MASK;

    // only signee can edit, and sign required
    static const uint32_t FLAG_MSG_TYPE_SIGNED;

    // no sign required, but signee can edit if signed
    static const uint32_t FLAG_MSG_TYPE_ANON;

    // anyone can mod but sign must be provided (needed for wikis)
    static const uint32_t FLAG_MSG_TYPE_SIGNED_SHARED;

    /*** GROUP FLAGS ***/



    /*** MESSAGE FLAGS ***/

    // indicates message edits an existing message
    static const uint32_t FLAG_MSG_EDIT;

    // indicates msg is id signed
    static const uint32_t FLAG_MSG_ID_SIGNED;

    /*** MESSAGE FLAGS ***/

}


#endif // RSGXSFLAGS_H

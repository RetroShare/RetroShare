/*******************************************************************************
 * unittests/libretroshare/gxs/data_service/rsgxsdata_test.cc                  *
 *                                                                             *
 * Copyright (C) 2018, Retroshare team <retroshare.team@gmailcom>              *
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

#include "libretroshare/serialiser/support.h"
#include "libretroshare/gxs/common/data_support.h"
#include "gxs/rsgxsdata.h"

#include <sstream>

std::string testEqual(const RsGxsGrpMetaData& d1,const RsGxsGrpMetaData& d2)
{
    std::ostringstream out;
    if(d1.mGroupId      !=  d2.mGroupId       ) { out << "mGroupId"       << ": " << d1.mGroupId.toStdString()       << "!=" << d2.mGroupId.toStdString()       ; return out.str();}
    if(d1.mOrigGrpId    !=  d2.mOrigGrpId     ) { out << "mOrigGrpId"     << ": " << d1.mOrigGrpId.toStdString()     << "!=" << d2.mOrigGrpId.toStdString()     ; return out.str();}
    if(d1.mAuthorId     !=  d2.mAuthorId      ) { out << "mAuthorId"      << ": " << d1.mAuthorId.toStdString()      << "!=" << d2.mAuthorId.toStdString()      ; return out.str();}
    if(d1.mCircleId     !=  d2.mCircleId      ) { out << "mCircleId"      << ": " << d1.mCircleId.toStdString()      << "!=" << d2.mCircleId.toStdString()      ; return out.str();}
    if(d1.mParentGrpId  !=  d2.mParentGrpId   ) { out << "mParentGrpId"   << ": " << d1.mParentGrpId.toStdString()   << "!=" << d2.mParentGrpId.toStdString()   ; return out.str();}
    if(d1.mGroupName    !=  d2.mGroupName     ) { out << "mGroupName"     << ": " << d1.mGroupName     << "!=" << d2.mGroupName     ; return out.str();}
    if(d1.mGroupFlags   !=  d2.mGroupFlags    ) { out << "mGroupFlags"    << ": " << d1.mGroupFlags    << "!=" << d2.mGroupFlags    ; return out.str();}
    if(d1.mPublishTs    !=  d2.mPublishTs     ) { out << "mPublishTs"     << ": " << d1.mPublishTs     << "!=" << d2.mPublishTs     ; return out.str();}
    if(d1.mSignFlags    !=  d2.mSignFlags     ) { out << "mSignFlags"     << ": " << d1.mSignFlags     << "!=" << d2.mSignFlags     ; return out.str();}
    if(d1.mCircleType   !=  d2.mCircleType    ) { out << "mCircleType"    << ": " << d1.mCircleType    << "!=" << d2.mCircleType    ; return out.str();}
    if(d1.mServiceString!=  d2.mServiceString ) { out << "mServiceString" << ": " << d1.mServiceString << "!=" << d2.mServiceString ; return out.str();}
    if(d1.mAuthenFlags  !=  d2.mAuthenFlags   ) { out << "mAuthenFlags"   << ": " << d1.mAuthenFlags   << "!=" << d2.mAuthenFlags   ; return out.str();}

    // if(d1.signSet       !=  d2.signSet        ) { out << "signSet"        << ": " << d1.signSet        << "!=" << d2.signSet        ; return out.str();}
    // if(d1.keys          !=  d2.keys           ) { out << "keys"           << ": " << d1.keys           << "!=" << d2.keys           ; return out.str();}

    return "" ;
}

std::string testEqual(const RsGxsMsgMetaData& d1,const RsGxsMsgMetaData& d2)
{
    std::ostringstream out;
    if(d1.mGroupId      !=  d2.mGroupId       ) { out << "mGroupId"       << ": " << d1.mGroupId.toStdString()       << " != " << d2.mGroupId.toStdString()       ; return out.str();}
    if(d1.mOrigMsgId    !=  d2.mOrigMsgId     ) { out << "mOrigMsgId"     << ": " << d1.mOrigMsgId.toStdString()     << " != " << d2.mOrigMsgId.toStdString()     ; return out.str();}
    if(d1.mAuthorId     !=  d2.mAuthorId      ) { out << "mAuthorId"      << ": " << d1.mAuthorId.toStdString()      << " != " << d2.mAuthorId.toStdString()      ; return out.str();}
    if(d1.mMsgId        !=  d2.mMsgId         ) { out << "mMsgId"         << ": " << d1.mMsgId.toStdString()         << " != " << d2.mMsgId.toStdString()         ; return out.str();}
    if(d1.mThreadId     !=  d2.mThreadId      ) { out << "mThreadId"      << ": " << d1.mThreadId.toStdString()      << " != " << d2.mThreadId.toStdString()      ; return out.str();}
    if(d1.mParentId     !=  d2.mParentId      ) { out << "mParentId"      << ": " << d1.mParentId.toStdString()      << " != " << d2.mParentId.toStdString()      ; return out.str();}
    if(d1.mPublishTs    !=  d2.mPublishTs     ) { out << "mPublishTs"     << ": " << d1.mPublishTs     << " != " << d2.mPublishTs     ; return out.str();}
    if(d1.mMsgName      !=  d2.mMsgName       ) { out << "mMsgName"       << ": " << d1.mMsgName       << " != " << d2.mMsgName       ; return out.str();}
    if(d1.mPublishTs    !=  d2.mPublishTs     ) { out << "mPublishTs"     << ": " << d1.mPublishTs     << " != " << d2.mPublishTs     ; return out.str();}
    if(d1.mMsgFlags     !=  d2.mMsgFlags      ) { out << "mMsgFlags"      << ": " << d1.mMsgFlags      << " != " << d2.mMsgFlags      ; return out.str();}

    // if(d1.refcount      !=  d2.refcount       ) { out << "refcount"       << ": " << d1.refcount       << " != " << d2.refcount       ; return out.str();} //Is Static
    // if(d1.signSet       !=  d2.signSet        ) { out << "signSet"        << ": " << d1.signSet        << " != " << d2.signSet        ; return out.str();}

    return "" ;
}

TEST(libretroshare_gxs, RsGxsData)
{

    RsGxsGrpMetaData grpMeta1, grpMeta2;

    grpMeta1.clear();
    init_item(&grpMeta1);

    uint32_t pktsize = grpMeta1.serial_size(RS_GXS_GRP_META_DATA_CURRENT_API_VERSION);
    char grp_data[pktsize];

    bool grpSerialise_OK = grpMeta1.serialise(grp_data, pktsize, RS_GXS_GRP_META_DATA_CURRENT_API_VERSION);
    EXPECT_TRUE(grpSerialise_OK);
    grpMeta2.clear();
    bool grpDeserialise_OK = grpMeta2.deserialise(grp_data, pktsize);
    EXPECT_TRUE(grpDeserialise_OK);

    EXPECT_EQ(testEqual(grpMeta1, grpMeta2), "");


    RsGxsMsgMetaData msgMeta1, msgMeta2;

    msgMeta1.clear();
    init_item(&msgMeta1);

    pktsize = msgMeta1.serial_size();
    char msg_data[pktsize];

    bool msgSerialise_OK = msgMeta1.serialise(msg_data, &pktsize);
    EXPECT_TRUE(msgSerialise_OK);
    msgMeta2.clear();
    bool msgDeserialise_OK = msgMeta2.deserialise(msg_data, &pktsize);
    EXPECT_TRUE(msgDeserialise_OK);

    EXPECT_EQ(testEqual(msgMeta1, msgMeta2), "");
}



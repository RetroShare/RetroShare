/*
 * libretroshare/src/test rsphotoitem_test.cc
 *
 * Test for photo item serialisation
 *
 * Copyright 2012-2012 by Christopher Evi-Parker
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


#include "rsphotoitem_test.h"


RsSerialType* init_item(RsGxsPhotoAlbumItem &album)
{
    RsPhotoAlbum& a = album.album;

    randString(SHORT_STR, a.mCaption);
    randString(SHORT_STR, a.mCategory);
    randString(SHORT_STR, a.mDescription);
    randString(SHORT_STR, a.mHashTags);
    randString(SHORT_STR, a.mOther);
    randString(SHORT_STR, a.mPhotoPath);
    randString(SHORT_STR, a.mPhotographer);
    randString(SHORT_STR, a.mWhen);
    randString(SHORT_STR, a.mWhere);
    randString(SHORT_STR, a.mThumbnail.type);
    std::string rStr;
    randString(SHORT_STR, rStr);

    a.mThumbnail.data = new uint8_t[SHORT_STR];
    memcpy(a.mThumbnail.data, rStr.data(), SHORT_STR);
    a.mThumbnail.size = SHORT_STR;

    return new RsGxsPhotoSerialiser();
}

RsSerialType* init_item(RsGxsPhotoPhotoItem &photo)
{

    RsPhotoPhoto& p = photo.photo;

    randString(SHORT_STR, p.mCaption);
    randString(SHORT_STR, p.mCategory);
    randString(SHORT_STR, p.mDescription);
    randString(SHORT_STR, p.mHashTags);
    randString(SHORT_STR, p.mOther);
    randString(SHORT_STR, p.mPhotographer);
    randString(SHORT_STR, p.mWhen);
    randString(SHORT_STR, p.mWhere);
    randString(SHORT_STR, p.mThumbnail.type);
    std::string rStr;
    randString(SHORT_STR, rStr);

    p.mThumbnail.data = new uint8_t[SHORT_STR];
    memcpy(p.mThumbnail.data, rStr.data(), SHORT_STR);
    p.mThumbnail.size = SHORT_STR;

    return new RsGxsPhotoSerialiser();
}


bool operator == (RsGxsPhotoAlbumItem& l, RsGxsPhotoAlbumItem& r)
{
    RsPhotoAlbum& la = l.album;
    RsPhotoAlbum& ra = r.album;

    if(la.mCaption != ra.mCaption) return false;
    if(la.mCategory != ra.mCategory) return false;
    if(la.mDescription != ra.mDescription) return false;
    if(la.mHashTags != ra.mHashTags) return false;
    if(la.mOther != ra.mOther) return false;
    if(la.mPhotographer!= ra.mPhotographer) return false;
    if(la.mPhotoPath != ra.mPhotoPath) return false;
    if(la.mWhere != ra.mWhere) return false;
    if(la.mWhen != ra.mWhen) return false;
    if(!(la.mThumbnail == ra.mThumbnail)) return false;

    return true;
}



bool operator == (RsGxsPhotoPhotoItem& l, RsGxsPhotoPhotoItem& r)
{
    RsPhotoPhoto& la = l.photo;
    RsPhotoPhoto& ra = r.photo;

    if(la.mCaption != ra.mCaption) return false;
    if(la.mCategory != ra.mCategory) return false;
    if(la.mDescription != ra.mDescription) return false;
    if(la.mHashTags != ra.mHashTags) return false;
    if(la.mOther != ra.mOther) return false;
    if(la.mPhotographer!= ra.mPhotographer) return false;
    if(la.mWhere != ra.mWhere) return false;
    if(la.mWhen != ra.mWhen) return false;
    if(!(la.mThumbnail == ra.mThumbnail)) return false;

    return true;
}

bool operator == (RsPhotoThumbnail& l, RsPhotoThumbnail& r)
{
    if(l.size != r.size) return false;
    if(l.type != r.type) return false;
    if(memcmp(l.data, r.data,l.size) != 0) return false;

    return true;
}

INITTEST()

int main()
{
    std::cerr << "RsPhotoItem Tests" << std::endl;

    test_RsItem<RsGxsPhotoAlbumItem>(); REPORT("Serialise/Deserialise RsGxsPhotoAlbumItem");
    test_RsItem<RsGxsPhotoPhotoItem>(); REPORT("Serialise/Deserialise RsGxsPhotoPhotoItem");

    FINALREPORT("RsPhotoItem Tests");

    return TESTRESULT();
}

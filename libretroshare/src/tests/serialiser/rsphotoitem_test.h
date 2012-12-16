/*
 * libretroshare/src/test rsphotoitem_test.h
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

#ifndef RSPHOTOITEM_TEST_H_
#define RSPHOTOITEM_TEST_H_

#include "serialiser/rsphotov2items.h"
#include "support.h"



RsSerialType* init_item(RsGxsPhotoAlbumItem& album);
RsSerialType* init_item(RsGxsPhotoPhotoItem& photo);

bool operator == (RsGxsPhotoAlbumItem& l, RsGxsPhotoAlbumItem& r);
bool operator == (RsGxsPhotoPhotoItem& l, RsGxsPhotoPhotoItem& r);
bool operator == (RsPhotoThumbnail& l, RsPhotoThumbnail& r);

#endif /* RSPHOTOITEM_TEST_H_ */

/*******************************************************************************
 * unittests/libretroshare/serialiser/support.cc                               *
 *                                                                             *
 * Copyright 2007-2008 by Christopher Evi-Parker <retroshare.project@gmail.com>*
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

#include <stdlib.h>

#include "support.h"
#include "serialiser/rstlvbase.h"
#include "gxs/gxssecurity.h"

void randString(const uint32_t length, std::string& outStr)
{
	char alpha = 'a';
	char* stringData = NULL;

	stringData = new char[length];

	for(uint32_t i=0; i != length; i++)
		stringData[i] = alpha + (rand() % 26);

	outStr.assign(stringData, length);
	delete[] stringData;

	return;
}

void randString(const uint32_t length, std::wstring& outStr)
{
	wchar_t alpha = L'a';
	wchar_t* stringData = NULL;

	stringData = new wchar_t[length];

	for(uint32_t i=0; i != length; i++)
		stringData[i] = (alpha + (rand() % 26));

	outStr.assign(stringData, length);
	delete[] stringData;

	return;
}


void init_item(RsTlvSecurityKeySet& ks)
{
	int n = rand()%24;
	randString(SHORT_STR, ks.groupId);
	for(int i=1; i<n; i++)
	{
		RsTlvPublicRSAKey pub_key;
		RsTlvPrivateRSAKey pri_key;
		GxsSecurity::generateKeyPair(pub_key, pri_key);
		ks.public_keys[pub_key.keyId] = pub_key;
		ks.private_keys[pri_key.keyId] = pri_key;
	}
}

bool operator==(const RsTlvSecurityKeySet& l, const RsTlvSecurityKeySet& r)
{

	if(l.groupId != r.groupId) return false;

	std::map<RsGxsId, RsTlvPublicRSAKey>::const_iterator l_cit = l.public_keys.begin(),
	r_cit = r.public_keys.begin();

	for(; l_cit != l.public_keys.end(); l_cit++, r_cit++){
		if(l_cit->first != r_cit->first) return false;
		if(!(l_cit->second == r_cit->second)) return false;
	}

	return true;
}

bool operator==(const RsTlvPublicRSAKey& sk1, const RsTlvPublicRSAKey& sk2)
{

	if(sk1.startTS != sk2.startTS) return false;
	if(sk1.endTS != sk2.endTS) return false;
	if(sk1.keyFlags != sk2.keyFlags) return false;
	if(sk1.keyId != sk2.keyId) return false;
	if(!(sk1.keyData == sk1.keyData)) return false;

	return true;
}

bool operator==(const RsTlvKeySignature& ks1, const RsTlvKeySignature& ks2)
{

	if(ks1.keyId != ks2.keyId) return false;
	if(!(ks1.signData == ks2.signData)) return false;

	return true;
}

bool operator==(const RsTlvKeySignatureSet& kss1, const RsTlvKeySignatureSet& kss2)
{
    const std::map<SignType, RsTlvKeySignature>& set1 = kss1.keySignSet,
    &set2  = kss2.keySignSet;

    if(set1.size() != set2.size()) return false;

    std::map<SignType, RsTlvKeySignature>::const_iterator it1 = set1.begin(), it2;

    for(; it1 != set1.end(); it1++)
    {
        SignType st1 = it1->first;

        if( (it2 =set2.find(st1)) == set2.end())
            return false;

        if(!(it1->second == it2->second))
            return false;

    }

    return true;
}

bool operator==(const RsTlvPeerIdSet& pids1, const RsTlvPeerIdSet& pids2)
{
    std::set<RsPeerId>::const_iterator it1 = pids1.ids.begin(),
			it2 = pids2.ids.begin();


	for(; ((it1 != pids1.ids.end()) && (it2 != pids2.ids.end())); it1++, it2++)
	{
		if(*it1 != *it2) return false;
	}

	return true;

}


void init_item(RsTlvImage& im)
{
	std::string imageData;
	randString(LARGE_STR, imageData);
	im.binData.setBinData(imageData.c_str(), imageData.size());
	im.image_type = RSTLV_IMAGE_TYPE_PNG;

	return;
}

bool operator==(const RsTlvBinaryData& bd1, const RsTlvBinaryData& bd2)
{
	if(bd1.tlvtype != bd2.tlvtype) return false;
	if(bd1.bin_len != bd2.bin_len) return false;

	unsigned char *bin1 = (unsigned char*)(bd1.bin_data),
			*bin2 = (unsigned char*)(bd2.bin_data);

	for(uint32_t i=0; i < bd1.bin_len; bin1++, bin2++, i++)
	{
		if(*bin1 != *bin2)
			return false;
	}

	return true;
}

void init_item(RsTlvKeySignature& ks)
{
    ks.keyId = RsGxsId::random();

	std::string signData;
	randString(LARGE_STR, signData);

	ks.signData.setBinData(signData.c_str(), signData.size());

	return;
}
void init_item(RsTlvKeySignatureSet &kss)
{
    int numSign = rand()%21;

    for(int i=0; i < numSign; i++)
    {
        RsTlvKeySignature sign;
        SignType sType = rand()%2452;
        init_item(sign);
        kss.keySignSet.insert(std::make_pair(sType, sign));
    }
}

bool operator==(const RsTlvImage& img1, const RsTlvImage& img2)
{
	if(img1.image_type != img2.image_type) return false;
	if(!(img1.binData == img2.binData)) return false;

	return true;

}

/** channels, forums and blogs **/

void init_item(RsTlvHashSet& hs)
{
	for(int i=0; i < 10; i++)
        hs.ids.insert(RsFileHash::random());

	return;
}

void init_item(RsTlvPeerIdSet& ps)
{
	for(int i=0; i < 10; i++)
        ps.ids.insert(RsPeerId::random());

	return;
}

bool operator==(const RsTlvHashSet& hs1,const RsTlvHashSet& hs2)
{
    std::set<RsFileHash>::const_iterator it1 = hs1.ids.begin(),
			it2 = hs2.ids.begin();

	for(; ((it1 != hs1.ids.end()) && (it2 != hs2.ids.end())); it1++, it2++)
	{
		if(*it1 != *it2) return false;
	}

	return true;
}

void init_item(RsTlvFileItem& fi)
{
	fi.age = rand()%200;
	fi.filesize = rand()%34030313;
	fi.hash = RsFileHash::random();
	randString(SHORT_STR, fi.name);
	randString(SHORT_STR, fi.path);
	fi.piecesize = rand()%232;
	fi.pop = rand()%2354;
	init_item(fi.hashset);

	return;
}

void init_item(RsTlvBinaryData& bd){
    bd.TlvClear();
    std::string data;
    randString(LARGE_STR, data);
    bd.setBinData(data.data(), data.length());
}

void init_item(RsTlvFileSet& fSet){

	randString(LARGE_STR, fSet.comment);
	randString(SHORT_STR, fSet.title);
	RsTlvFileItem fi1, fi2;
	init_item(fi1);
	init_item(fi2);
	fSet.items.push_back(fi1);
	fSet.items.push_back(fi2);

	return;
}

bool operator==(const RsTlvFileSet& fs1,const  RsTlvFileSet& fs2)
{
	if(fs1.comment != fs2.comment) return false;
	if(fs1.title != fs2.title) return false;

	std::list<RsTlvFileItem>::const_iterator it1 = fs1.items.begin(),
			it2 = fs2.items.begin();

	for(;  ((it1 != fs1.items.end()) && (it2 != fs2.items.end())); it1++, it2++)
		if(!(*it1 == *it2)) return false;

	return true;
}

bool operator==(const RsTlvFileItem& fi1,const RsTlvFileItem& fi2)
{
	if(fi1.age != fi2.age) return false;
	if(fi1.filesize != fi2.filesize) return false;
	if(fi1.hash != fi2.hash) return false;
	if(!(fi1.hashset == fi2.hashset)) return false;
	if(fi1.name != fi2.name) return false;
	if(fi1.path != fi2.path) return false;
	if(fi1.piecesize != fi2.piecesize) return false;
	if(fi1.pop != fi2.pop) return false;

	return true;
}

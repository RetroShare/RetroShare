/*******************************************************************************
 * unittests/libretroshare/gxs/nxs_test/nxsdummyservice.cpp                    *
 *                                                                             *
 * Copyright (C) 2014, Crispy <retroshare.team@gmailcom>                       *
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

#include "nxsdummyservices.h"



rs_nxs_test::RsNxsSimpleDummyCircles::RsNxsSimpleDummyCircles() {
}

bool rs_nxs_test::RsNxsSimpleDummyCircles::isLoaded(
		const RsGxsCircleId& /*circleId*/) {
	return true;
}

bool rs_nxs_test::RsNxsSimpleDummyCircles::loadCircle(
		const RsGxsCircleId& /*circleId*/) {
	return true;
}

int rs_nxs_test::RsNxsSimpleDummyCircles::canSend(const RsGxsCircleId& /*circleId*/,
		const RsPgpId& /*id*/, bool &/*should_encrypt*/) {
	return true;
}

int rs_nxs_test::RsNxsSimpleDummyCircles::canReceive(
		const RsGxsCircleId& /*circleId*/, const RsPgpId& /*id*/) {
	return true;
}

bool rs_nxs_test::RsNxsSimpleDummyCircles::isRecipient(const RsGxsCircleId &/*circleId*/, const RsGxsGroupId &/*destination_group*/, const RsGxsId& /*id*/)
{
	return true ;
}
            
bool rs_nxs_test::RsNxsSimpleDummyCircles::recipients(
		const RsGxsCircleId& /*circleId*/, std::list<RsPgpId>& /*friendlist*/) {
	return true;
}

bool rs_nxs_test::RsNxsSimpleDummyCircles::recipients(const RsGxsCircleId& /*circleId*/, const RsGxsGroupId &/*destination_group*/, std::list<RsGxsId>& /*friendlist*/) {
	return true;
}

rs_nxs_test::RsNxsSimpleDummyReputation::RsNxsSimpleDummyReputation(
		RepMap& /*repMap*/, bool /*cached*/) {
}

bool rs_nxs_test::RsNxsSimpleDummyReputation::haveReputation(
		const RsGxsId& /*id*/) {
	return true;
}

bool rs_nxs_test::RsNxsSimpleDummyReputation::loadReputation(const RsGxsId& /*id*/,
		const std::list<RsPeerId>& /*peers*/) {
	return true;
}

bool rs_nxs_test::RsNxsSimpleDummyReputation::getReputation(const RsGxsId& /*id*/,
		GixsReputation& rep) {
	rep.reputation_level = 5;
	return true;
}

rs_nxs_test::RsNxsDelayedDummyCircles::RsNxsDelayedDummyCircles(
		int countBeforePresent) : mCountBeforePresent(countBeforePresent)
{
}

rs_nxs_test::RsNxsDelayedDummyCircles::~RsNxsDelayedDummyCircles() {
}

bool rs_nxs_test::RsNxsDelayedDummyCircles::isLoaded(
		const RsGxsCircleId& circleId) {
	return allowed(circleId);
}

bool rs_nxs_test::RsNxsDelayedDummyCircles::loadCircle(
		const RsGxsCircleId& circleId) {
	return allowed(circleId);
}

int rs_nxs_test::RsNxsDelayedDummyCircles::canSend(const RsGxsCircleId& circleId, const RsPgpId& /*id*/, bool &/*should_encrypt*/) {
	return allowed(circleId);
}

int rs_nxs_test::RsNxsDelayedDummyCircles::canReceive(
		const RsGxsCircleId& circleId, const RsPgpId& /*id*/) {
	return allowed(circleId);
}

bool rs_nxs_test::RsNxsDelayedDummyCircles::recipients(
		const RsGxsCircleId& circleId, std::list<RsPgpId>& /*friendlist*/) {
	return allowed(circleId);
}

bool rs_nxs_test::RsNxsDelayedDummyCircles::allowed(
		const RsGxsCircleId& circleId) {

	if(mMembershipCallCount.find(circleId) == mMembershipCallCount.end())
		mMembershipCallCount[circleId] = 0;

	if(mMembershipCallCount[circleId] >= mCountBeforePresent)
	{
		mMembershipCallCount[circleId]++;
                return true;
	}
	else
	{
		mMembershipCallCount[circleId]++;
                return false;
	}

}

const RsPgpId& rs_nxs_test::RsDummyPgpUtils::getPGPOwnId() {
	return mOwnId;
}

RsPgpId rs_nxs_test::RsDummyPgpUtils::getPGPId(const RsPeerId& /*sslid*/) {
	return RsPgpId().random();
}

bool rs_nxs_test::RsDummyPgpUtils::getGPGAllList(std::list<RsPgpId>& /*ids*/) {
	return true;
}

bool rs_nxs_test::RsDummyPgpUtils::getKeyFingerprint(const RsPgpId& /*id*/,
                                                     PGPFingerprintType& /*fp*/
                                                     ) const {
	return true;
}

bool rs_nxs_test::RsDummyPgpUtils::parseSignature(unsigned char* /*sign*/,
                                                  unsigned int /*signlen*/,
                                                  RsPgpId& /*issuer*/
                                                  ) const {
	return true;
}

bool rs_nxs_test::RsDummyPgpUtils::VerifySignBin(const void* /*data*/,
                                                 uint32_t /*len*/,
                                                 unsigned char* /*sign*/,
                                                 unsigned int /*signlen*/,
                                                 const PGPFingerprintType& /*withfingerprint*/
                                                 ) {
	return true;
}

bool rs_nxs_test::RsDummyPgpUtils::askForDeferredSelfSignature(const void* /*data*/,
                                                               const uint32_t /*len*/,
                                                               unsigned char* /*sign*/,
                                                               unsigned int* /*signlen*/,
                                                               int& /*signature_result*/,
                                                               std::string /*reason*/
                                                               ) {
	return true;
}




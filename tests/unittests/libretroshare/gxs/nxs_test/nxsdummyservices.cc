/*
 * nxsdummyservices.cc
 *
 *  Created on: 13 Apr 2014
 *      Author: crispy
 */


#include "nxsdummyservices.h"



rs_nxs_test::RsNxsSimpleDummyCircles::RsNxsSimpleDummyCircles(
		std::list<Membership>& membership, bool cached) {
}

bool rs_nxs_test::RsNxsSimpleDummyCircles::isLoaded(
		const RsGxsCircleId& circleId) {
	return true;
}

bool rs_nxs_test::RsNxsSimpleDummyCircles::loadCircle(
		const RsGxsCircleId& circleId) {
	return true;
}

int rs_nxs_test::RsNxsSimpleDummyCircles::canSend(const RsGxsCircleId& circleId,
		const RsPgpId& id) {
	return true;
}

int rs_nxs_test::RsNxsSimpleDummyCircles::canReceive(
		const RsGxsCircleId& circleId, const RsPgpId& id) {
	return true;
}

bool rs_nxs_test::RsNxsSimpleDummyCircles::recipients(
		const RsGxsCircleId& circleId, std::list<RsPgpId>& friendlist) {
	return true;
}

rs_nxs_test::RsNxsSimpleDummyReputation::RsNxsSimpleDummyReputation(
		RepMap& repMap, bool cached) {
}

bool rs_nxs_test::RsNxsSimpleDummyReputation::haveReputation(
		const RsGxsId& id) {
	return true;
}

bool rs_nxs_test::RsNxsSimpleDummyReputation::loadReputation(const RsGxsId& id,
		const std::list<RsPeerId>& peers) {
	return true;
}

bool rs_nxs_test::RsNxsSimpleDummyReputation::getReputation(const RsGxsId& id,
		GixsReputation& rep) {
	rep.score = 5;
	return true;
}

#pragma once

#include <chrono>

#include "util/rstime.h"

#include <retroshare/rstokenservice.h>

namespace WikiTokenWaiter
{
template <typename RequestStatusFn>
bool waitForToken(RequestStatusFn requestStatus, uint32_t token, uint32_t timeoutMs = 5000)
{
	using clock = std::chrono::steady_clock;
	const auto start = clock::now();

	RsTokenService::GxsRequestStatus status = requestStatus(token);
	while (status == RsTokenService::PENDING)
	{
		rstime::rs_usleep(50 * 1000);
		if (timeoutMs > 0 &&
		    std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - start).count() >= timeoutMs)
		{
			return false;
		}
		status = requestStatus(token);
	}

	return status == RsTokenService::COMPLETE;
}
}

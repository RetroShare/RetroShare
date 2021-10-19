/*******************************************************************************
 * libretroshare/src/gxs: rsgxsobserver.h                                      *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2011-2012 Robert Fernie                                       *
 * Copyright (C) 2011-2012 Christopher Evi-Parker                              *
 * Copyright (C) 2021  Gioacchino Mazzurco <gio@eigenlab.org>                  *
 * Copyright (C) 2021  Asociación Civil Altermundi <info@altermundi.net>       *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
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
 *******************************************************************************/
#pragma once

#include <system_error>
#include <vector>

#include "retroshare/rsgxsiface.h"
#include "rsitems/rsnxsitems.h"
#include "util/rsdebug.h"

typedef uint32_t TurtleRequestId;

enum class RsNxsObserverErrorNum : int32_t
{
	NOT_OVERRIDDEN_BY_OBSERVER = 2004,
};

struct RsNxsObserverErrorCategory: std::error_category
{
	const char* name() const noexcept override
	{ return "RetroShare NXS Observer"; }

	std::string message(int ev) const override
	{
		switch (static_cast<RsNxsObserverErrorNum>(ev))
		{
		case RsNxsObserverErrorNum::NOT_OVERRIDDEN_BY_OBSERVER:
			return "Method not overridden by observer";
		default:
			return rsErrorNotInCategory(ev, name());
		}
	}

	std::error_condition default_error_condition(int ev) const noexcept override;

	const static RsNxsObserverErrorCategory instance;
};


namespace std
{
/** Register RsNxsObserverErrorNum as an error condition enum, must be in std
 * namespace */
template<> struct is_error_condition_enum<RsNxsObserverErrorNum> : true_type {};
}

/** Provide RsJsonApiErrorNum conversion to std::error_condition, must be in
 * same namespace of RsJsonApiErrorNum */
inline std::error_condition make_error_condition(RsNxsObserverErrorNum e) noexcept
{
	return std::error_condition(
	            static_cast<int>(e), RsNxsObserverErrorCategory::instance );
};

class RsNxsObserver
{
public:

    /*!
     * @param messages messages are deleted after function returns
     */
    virtual void receiveNewMessages(const std::vector<RsNxsMsg*>& messages) = 0;

    /*!
     * @param groups groups are deleted after function returns
     */
    virtual void receiveNewGroups(const std::vector<RsNxsGrp*>& groups) = 0;

    /*!
     * \brief receiveDistantSearchResults
     * 				Called when new distant search result arrive.
     * \param grpId
     */
    virtual void receiveDistantSearchResults(TurtleRequestId /*id*/,const RsGxsGroupId& /*grpId*/)
    {
        std::cerr << __PRETTY_FUNCTION__ << ": not overloaded but still called. Nothing will happen." << std::endl;
    }

	/** If advanced search functionalities like deep indexing are supported at
	 * observer/service level, this method should be overridden to handle search
	 * requests there.
	 * @param[in] requestData search query
	 * @param[in] requestSize search query size
	 * @param[out] resultData results data storage for a pointer to search
	 * result reply data or nullptr if no matching results where found
	 * @param[out] resultSize storage for results data size or 0 if no matching
	 * results where found
	 * @return Error details or success, NOT_OVERRIDDEN_BY_OBSERVER is
	 * returned to inform the caller that this method was not overridden by the
	 * observer so do not use it for other meanings. */
	virtual std::error_condition handleDistantSearchRequest(
	        rs_view_ptr<uint8_t> requestData, uint32_t requestSize,
	        rs_owner_ptr<uint8_t>& resultData, uint32_t& resultSize )
	{
		/* Avoid unused paramethers warning this way so doxygen can still parse
		 * paramethers documentation */
		(void) requestData; (void) requestSize;
		(void) resultData; (void) resultSize;
		return RsNxsObserverErrorNum::NOT_OVERRIDDEN_BY_OBSERVER;
	}

	/** If advanced search functionalities like deep indexing are supported at
	 * observer/service level, this method should be overridden to handle search
	 * results there.
	 * @param[in] requestId search query id
	 * @param[out] resultData results data
	 * @param[out] resultSize results data size
	 * @return Error details or success, NOT_OVERRIDDEN_BY_OBSERVER is
	 * returned to inform the caller that this method was not overridden by the
	 * observer so do not use it for other meanings. */
	virtual std::error_condition receiveDistantSearchResult(
	        const TurtleRequestId requestId,
	        rs_owner_ptr<uint8_t>& resultData, uint32_t& resultSize )
	{
		(void) requestId; (void) resultData; (void) resultSize;
		return RsNxsObserverErrorNum::NOT_OVERRIDDEN_BY_OBSERVER;
	}

    /*!
     * @param grpId group id
     */
    virtual void notifyReceivePublishKey(const RsGxsGroupId &grpId) = 0;

    /*!
     * \brief notifyChangedGroupSyncParams
     * \param 	caled when a group sync parameter is updated
     */
    virtual void notifyChangedGroupSyncParams(const RsGxsGroupId &grpId) = 0;
    /*!
     * @param grpId group id
     */
    virtual void notifyChangedGroupStats(const RsGxsGroupId &grpId) = 0;

	RsNxsObserver() = default;
	virtual ~RsNxsObserver() = default;
};

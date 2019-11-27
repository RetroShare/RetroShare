/*******************************************************************************
 * libretroshare/src/rsserver: rswebui.h                                       *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2019-2019 Cyril Soler <csoler@users.sourceforge.net>              *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License version 3 as    *
 * published by the Free Software Foundation.                                  *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
#pragma once

#include <string>

class RsWebUi;

/**
 * Pointer to global instance of RsWebUi service implementation
 * jsonapi_temporarly_disabled{development} because it breaks compilation when
 * webui is disabled
 */
extern RsWebUi* rsWebUi;

class RsWebUi
{
public:
	static const std::string DEFAULT_BASE_DIRECTORY;

	/**
	 * @brief Restart WebUI
	 * @jsonapi{development}
	 */
	virtual bool restart() = 0;

	/**
	 * @brief Stop WebUI
	 * @jsonapi{development}
	 */
	virtual bool stop() = 0;

	/**
	 * @brief Set WebUI static files directory, need restart to apply
	 * @param[in] htmlDir directory path
	 * @jsonapi{development}
	 */
	virtual void setHtmlFilesDirectory(const std::string& htmlDir) = 0;

	/**
	 * @brief Set WebUI user password
	 * @param[in] password new password for WebUI
	 * @jsonapi{development}
	 */
	virtual void setUserPassword(const std::string& password) =0;

	/**
	 * @brief check if WebUI is running
	 * @jsonapi{development}
	 * @return true if running, false otherwise
	 */
	virtual bool isRunning() const = 0;

	virtual ~RsWebUi() = default;
};

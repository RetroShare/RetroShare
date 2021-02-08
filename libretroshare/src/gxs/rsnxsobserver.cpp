/*******************************************************************************
 * RetroShare General eXchange System                                          *
 *                                                                             *
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

#include "gxs/rsnxsobserver.h"

const RsNxsObserverErrorCategory RsNxsObserverErrorCategory::instance;

std::error_condition RsNxsObserverErrorCategory::default_error_condition(int ev)
const noexcept
{
	switch(static_cast<RsNxsObserverErrorNum>(ev))
	{
	case RsNxsObserverErrorNum::NOT_OVERRIDDEN_BY_OBSERVER:
		return std::errc::operation_not_supported;
	default:
		return std::error_condition(ev, *this);
	}
}

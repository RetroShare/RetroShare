/*******************************************************************************
 * libretroshare/src/util: cxx17retrocompat.h                                  *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2019  Gioacchino Mazzurco <gio@eigenlab.org>                  *
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

#if __cplusplus < 201703L

#include <type_traits>

namespace std
{
using namespace std;
template <class T> constexpr typename add_const<T>::type& as_const(T& t) noexcept
{ return t; }
template <class T> void as_const(const T&&) = delete;
}
#endif // __cplusplus < 201703L

/*******************************************************************************
 *                                                                             *
 * libretroshare < C++14 retro-compatibility helpers                           *
 *                                                                             *
 * Copyright (C) 2020  Gioacchino Mazzurco <gio@eigenlab.org>                  *
 * Copyright (C) 2020  Asociación Civil Altermundi <info@altermundi.net>       *
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

#if __cplusplus < 201402L

#include <type_traits>

namespace std
{
template<class T> using decay_t = typename decay<T>::type;

template<bool B, class T = void>
using enable_if_t = typename enable_if<B,T>::type;

template<class T>
using remove_const_t = typename remove_const<T>::type;
}
#endif // __cplusplus < 201402L

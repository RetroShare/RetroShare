#pragma once
/*
 * RetroShare time utils
 * Copyright (C) 2016  Gioacchino Mazzurco <gio@eigenlab.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <thread>         // for std::this_thread::sleep_for
#include <chrono>         // for std::chrono::microseconds

/**
 * @brief rs_usleep sleep for the given microseconds
 * @param usecs time to sleep for in microseconds
 */
inline void rs_usleep(unsigned int usecs)
{ std::this_thread::sleep_for(std::chrono::microseconds(usecs)); }

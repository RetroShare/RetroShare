/*******************************************************************************
 * util/NonCopyable.h                                                          *
 *                                                                             *
 * Copyright (c) 2006, Crypton          <retroshare.project@gmail.com>         *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#ifndef NONCOPYABLE_H
#define NONCOPYABLE_H

#include <util/rsutildll.h>

/**
 * Ensures derived classes have private copy constructor and copy assignment.
 *
 * Example:
 * <pre>
 * class MyClass : NonCopyable {
 * public:
 *    ...
 * };
 * </pre>
 *
 * Taken from Boost library.
 *
 * @see boost::noncopyable
 * 
 */
class NonCopyable {
protected:

    RSUTIL_API NonCopyable() {}

    RSUTIL_API ~NonCopyable() {}

private:

	/**
	 * Copy constructor is private.
	 */
	NonCopyable(const NonCopyable &);

	/**
	 * Copy assignement is private.
	 */
	const NonCopyable & operator=(const NonCopyable &);
};

#endif	//NONCOPYABLE_H

#pragma once
/*
 * RetroShare Serialiser.
 * Copyright (C) 2016-2018  Gioacchino Mazzurco <gio@eigenlab.org>
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

#include "serialiser/rsserializer.h"


/** @brief Minimal ancestor for all serializable structs in RetroShare.
 * If you want your struct to be easly serializable you should inherit from this
 * struct.
 */
struct RsSerializable
{
	/** Process struct members to serialize in this method taking advantage of
	 * the helper macro @see RS_SERIAL_PROCESS(I)
	 */
	virtual void serial_process(RsGenericSerializer::SerializeJob j,
	                            RsGenericSerializer::SerializeContext& ctx) = 0;
};

/** @def RS_SERIAL_PROCESS(I)
 * Use this macro to process the members of `YourSerializable` for serial
 * processing inside `YourSerializable::serial_process(j, ctx)`
 *
 * Pay special attention for member of enum type which must be declared
 * specifying the underlying type otherwise the serialization format may differ
 * in an uncompatible way depending on the compiler/platform.
 *
 * Inspired by http://stackoverflow.com/a/39345864
 */
#define RS_SERIAL_PROCESS(I) do { \
	RsTypeSerializer::serial_process(j, ctx, I, #I ); \
	} while(0)

/*******************************************************************************
 * libretroshare/src/util: rscompress.h                                        *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2013 Cyril Soler <csoler@users.sourceforge.net>                   *
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

class RsCompress
{
	public:
		static bool compress_memory_chunk(const uint8_t *input_mem,const uint32_t input_size,uint8_t *& output_mem,uint32_t& output_size) ;
		static bool uncompress_memory_chunk(const uint8_t *input_mem,const uint32_t input_size,uint8_t *& output_mem,uint32_t& output_size) ;
};


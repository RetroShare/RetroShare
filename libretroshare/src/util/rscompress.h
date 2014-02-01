/*
 * libretroshare/src/utils: rscompress.h
 *
 * Basic memory chunk compression, based on openpgp-sdk
 *
 * Copyright 2013 by Cyril Soler
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "csoler@users.sourceforge.net".
 *
 */

#pragma once 

class RsCompress
{
	public:
		static bool compress_memory_chunk(const uint8_t *input_mem,const uint32_t input_size,uint8_t *& output_mem,uint32_t& output_size) ;
		static bool uncompress_memory_chunk(const uint8_t *input_mem,const uint32_t input_size,uint8_t *& output_mem,uint32_t& output_size) ;
};


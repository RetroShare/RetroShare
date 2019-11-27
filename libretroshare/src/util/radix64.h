/*******************************************************************************
 * libretroshare/src/util: radix64.h                                           *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2015 Retroshare Team <retroshare.project@gmail.com>           *
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

#include <string>
#include <string.h>
#include <vector>
#include <stdint.h>

class Radix64
{
public:
	static std::vector<uint8_t> decode(const std::string& buffer)
		{
			char val;
			int c = 0, c2;/* init c because gcc is not clever
								  enough for the continue */
			int idx;
			size_t buffer_pos = 0;

			radix64_init();

            std::vector<uint8_t> buf ;
			idx = 0;
			val = 0;

			for (buffer_pos = 0; buffer_pos < buffer.length(); buffer_pos++)
			{
				c = buffer[buffer_pos];

again:
				if (c == '\n' || c == ' ' || c == '\r' || c == '\t')
					continue;
				else if (c == '=')
				{	
					/* pad character: stop */
					/* some mailers leave quoted-printable
					 * encoded characters so we try to
					 * workaround this */
					if (buffer_pos + 2 < buffer.length())
					{
						int cc1, cc2, cc3;
						cc1 = buffer[buffer_pos];
						cc2 = buffer[buffer_pos + 1];
						cc3 = buffer[buffer_pos + 2];

						if (isxdigit((unsigned char)cc1) && isxdigit((unsigned char)cc2) && strchr("=\n\r\t ", cc3))
						{
							/* well it seems to be the case -
							 * adjust */
							c =
								isdigit((unsigned char)cc1) ? (cc1 -
										'0')
								: (toupper((unsigned char)cc1) - 'A' + 10);
							c <<= 4;
							c |=
								isdigit((unsigned char)cc2) ? (cc2 -
										'0')
								: (toupper((unsigned char)cc2) - 'A' + 10);
							buffer_pos += 2;
							goto again;
						}
					}

					if (idx == 1)
						buf.push_back(val) ;// buf[n++] = val;
					break;
				}
				else if ((c = asctobin()[(c2 = c)]) == 255)
				{
					/* invalid radix64 character %02x skipped\n", c2; */
					continue;
				}

				switch (idx)
				{
					case 0:
						val = c << 2;
						break;
					case 1:
						val |= (c >> 4) & 3;
						buf.push_back(val);//buf[n++] = val;
						val = (c << 4) & 0xf0;
						break;
					case 2:
						val |= (c >> 2) & 15;
						buf.push_back(val);//buf[n++] = val;
						val = (c << 6) & 0xc0;
						break;
					case 3:
						val |= c & 0x3f;
						buf.push_back(val);//buf[n++] = val;
						break;
				}
				idx = (idx + 1) % 4;
			}

			//idx = idx;

			return buf ;
		}

		/****************
		 * create a radix64 encoded string.
		 */
	static void encode(
	        const unsigned char* data, size_t len, std::string& out_string )
		{
			char *buffer, *p;

			radix64_init();

			size_t size = (len + 2) / 3 * 4 +1;
			buffer = p = new char[size] ;

			for (; len >= 3; len -= 3, data += 3)
			{
				*p++ = bintoasc()[(data[0] >> 2) & 077];
				*p++ =
					bintoasc()[
					(((data[0] << 4) & 060) |
					 ((data[1] >> 4) & 017)) & 077];
				*p++ =
					bintoasc()[
					(((data[1] << 2) & 074) |
					 ((data[2] >> 6) & 03)) & 077];
				*p++ = bintoasc()[data[2] & 077];
			}
			if (len == 2)
			{
				*p++ = bintoasc()[(data[0] >> 2) & 077];
				*p++ =
					bintoasc()[
					(((data[0] << 4) & 060) |
					 ((data[1] >> 4) & 017)) & 077];
				*p++ = bintoasc()[((data[1] << 2) & 074)];
				*p++ = '=' ;
			}
			else if (len == 1)
			{
				*p++ = bintoasc()[(data[0] >> 2) & 077];
				*p++ = bintoasc()[(data[0] << 4) & 060];
				*p++ = '=' ;
				*p++ = '=' ;
			}
			//*p = 0;
			out_string = std::string(buffer,p-buffer) ;
			delete[] buffer ;
		}

	private:
		static inline char *bintoasc() { static char bta[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"; return bta ; }
		static inline uint8_t *asctobin() { static uint8_t s[256]; return s ; }	/* runtime radix64_initd */
		static int& is_radix64_initd() { static int is_inited = false ; return is_inited ; }

		/* hey, guess what: this is a read-only table.
		 * we don't _care_ if multiple threads get to initialise it
		 * at the same time, _except_ that is_radix64_initd=1 _must_
		 * be done at the end...
		 */
		static bool radix64_init()
		{
			if (is_radix64_initd())
				return true;

			int i;
			char *s;

			/* build the helpapr_table_t for radix64 to bin conversion */
			for (i = 0; i < 256; i++)
				asctobin()[i] = 255;	/* used to detect invalid characters */
			for (s = bintoasc(), i = 0; *s; s++, i++)
				asctobin()[(int)*s] = i;

			is_radix64_initd() = 1;
			return true ;
		}
};



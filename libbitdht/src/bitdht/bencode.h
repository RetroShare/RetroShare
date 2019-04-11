/*******************************************************************************
 * bitdht/bdencode.h                                                           *
 *                                                                             *
 * BitDHT: An Flexible DHT library.                                            *
 *                                                                             *
 * Copyright 2011 by Robert Fernie <bitdht@lunamutt.com>                       *
 *                by Mike Frysinger <vapier@gmail.com>                         *
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
#ifndef _BENCODE_H
#define _BENCODE_H

/* USAGE:
 *  - pass the string full of the bencoded data to be_decode()
 *  - parse the resulting tree however you like
 *  - call be_free() on the tree to release resources
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	BE_STR,
	BE_INT,
	BE_LIST,
	BE_DICT,
} be_type;

struct be_dict;
struct be_node;

/*
 * XXX: the "val" field of be_dict and be_node can be confusing ...
 */

typedef struct be_dict {
	char *key;
	struct be_node *val;
} be_dict;

typedef struct be_node {
	be_type type;
	union {
		char *s;
		long long i;
		struct be_node **l;
		struct be_dict *d;
	} val;
} be_node;

extern long long be_str_len(be_node *node);
// This function uses strlen, so is unreliable.
//extern be_node *be_decode(const char *bencode);
extern be_node *be_decoden(const char *bencode, long long bencode_len);
extern void be_free(be_node *node);
extern void be_dump(be_node *node);
extern void be_dump_str(be_node *node);

// New Functions for the other half of the work - encoding */

extern int be_encode(be_node *node, char *str, int len);


// Creating the data structure.
extern int be_add_list(be_node *list, be_node *node);
extern int be_add_keypair(be_node *dict, const char *str, be_node *node);
extern be_node *be_create_int(long long int num);
extern be_node *be_create_list();
extern be_node *be_create_str(const char *str);
extern be_node *be_create_str_wlen(const char *str, int len); /* not including \0 */
extern be_node *be_create_dict();


#ifdef __cplusplus
}
#endif

#endif

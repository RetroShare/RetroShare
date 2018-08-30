/*******************************************************************************
 * bitdht/bdencode.cc                                                          *
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

/*
 * This implementation isn't optimized at all as I wrote it to support
 * a bogus system.  I have no real interest in this format.  Feel free
 * to send me patches (so long as you don't copyright them and you release
 * your changes into the public domain as well).
 */

#include <stdio.h>
#include <stdlib.h> /* malloc() realloc() free() strtoll() */
#include <string.h> /* memset() */
#include "util/bdstring.h"

#include "bitdht/bencode.h"

/***
 * #define BE_DEBUG_DECODE 1
 * #define BE_DEBUG 1 // controlled from Makefile too.
 ***/

#ifdef BE_DEBUG_DECODE 
	#include <stdio.h> /* debug */
#endif

static be_node *be_alloc(be_type type)
{
	be_node *ret = (be_node *) malloc(sizeof(*ret));
	if (ret) {
		memset(ret, 0x00, sizeof(*ret));
		ret->type = type;
	}
	return ret;
}

static long long _be_decode_int(const char **data, long long *data_len)
{
	char *endp;
	long long ret = strtoll(*data, &endp, 10);
	*data_len -= (endp - *data);
	*data = endp;

#ifdef BE_DEBUG_DECODE
	fprintf(stderr, "bencode::_be_decode_int(pnt: %p, rem: %lld) = %lld\n", *data, *data_len, ret);
#endif
	return ret;
}

long long be_str_len(be_node *node)
{
	long long ret = 0;
	if (node->val.s)
		memcpy(&ret, node->val.s - sizeof(ret), sizeof(ret));
	return ret;
}

static char *_be_decode_str(const char **data, long long *data_len)
{
#ifdef BE_DEBUG_DECODE 
	fprintf(stderr, "bencode::_be_decode_str(pnt: %p, rem: %lld)\n", *data, *data_len);
#endif
	long long sllen = _be_decode_int(data, data_len);
	long slen = sllen;
	unsigned long len;
	char *ret = NULL;

	/* slen is signed, so negative values get rejected */
	if (sllen < 0)
	{
#ifdef BE_DEBUG_DECODE 
		fprintf(stderr, "bencode::_be_decode_str() reject bad length\n");
#endif
		return ret;
	}

	/* reject attempts to allocate large values that overflow the
	 * size_t type which is used with malloc()
	 */
	if (sizeof(long long) != sizeof(long))
		if (sllen != slen)
		{
#ifdef BE_DEBUG_DECODE 
			fprintf(stderr, "bencode::_be_decode_str() reject large_values\n");
#endif
			return ret;
		}

	/* make sure we have enough data left */
	if (sllen > *data_len - 1)
	{
#ifdef BE_DEBUG_DECODE 
		fprintf(stderr, "bencode::_be_decode_str() reject large_values\n");
#endif
		return ret;
	}

	/* switch from signed to unsigned so we don't overflow below */
	len = slen;

	if (**data == ':') {
		char *_ret = (char *) malloc(sizeof(sllen) + len + 1);

		if(_ret == NULL)
		{
			fprintf(stderr, "(EE) bencode::_be_decode_str(): "
			                "ERROR. cannot allocate memory for %lu bytes.\n"
			        , len+1+sizeof(sllen) );
			return ret;
		}

		memcpy(_ret, &sllen, sizeof(sllen));
		ret = _ret + sizeof(sllen);
		memcpy(ret, *data + 1, len);
		ret[len] = '\0';
		*data += len + 1;
		*data_len -= len + 1;

#ifdef BE_DEBUG_DECODE 
		fprintf(stderr, "bencode::_be_decode_str() read %ld bytes\n", len+1);
#endif
	}
	else
	{
#ifdef BE_DEBUG_DECODE 
		fprintf(stderr, "bencode::_be_decode_str() reject missing :\n");
#endif
	}
	return ret;
}

static be_node *_be_decode(const char **data, long long *data_len)
{
#ifdef BE_DEBUG_DECODE 
	fprintf(stderr, "bencode::_be_decode(pnt: %p, rem: %lld)\n", *data, *data_len);
#endif
	be_node *ret = NULL;

	if (!*data_len)
	{
#ifdef BE_DEBUG_DECODE 
		fprintf(stderr, "bencode::_be_decode() reject invalid datalen\n");
#endif
		return ret;
	}

	switch (**data) {
		/* lists */
		case 'l': {
			unsigned int i = 0;
#ifdef BE_DEBUG_DECODE 
			fprintf(stderr, "bencode::_be_decode() found list\n");
#endif

			ret = be_alloc(BE_LIST);

			--(*data_len);
			++(*data);
			while (**data != 'e') {
#ifdef BE_DEBUG_DECODE 
			fprintf(stderr, "bencode::_be_decode() list get item (%d)\n", i);
#endif
				ret->val.l = (be_node **) realloc(ret->val.l, (i + 2) * sizeof(*ret->val.l));
				ret->val.l[i] = _be_decode(data, data_len);
				if (ret->val.l[i] == NULL)
				{
					/* failed decode - kill decode */
#ifdef BE_DEBUG_DECODE 
			fprintf(stderr, "bencode::_be_decode() failed list decode - kill\n");
#endif
					be_free(ret);
					return NULL;
				}
				++i;
			}
			--(*data_len);
			++(*data);

			/* empty list case. */
			if (i == 0)
			{
				ret->val.l = (be_node **) realloc(ret->val.l, 1 * sizeof(*ret->val.l));
			}

			ret->val.l[i] = NULL;

			return ret;
		}

		/* dictionaries */
		case 'd': {
			unsigned int i = 0;
#ifdef BE_DEBUG_DECODE 
			fprintf(stderr, "bencode::_be_decode() found dictionary\n");
#endif

			ret = be_alloc(BE_DICT);

			--(*data_len);
			++(*data);
			while (**data != 'e') {
#ifdef BE_DEBUG_DECODE 
			fprintf(stderr, "bencode::_be_decode() dictionary get key (%d)\n", i);
#endif
				ret->val.d = (be_dict *) realloc(ret->val.d, (i + 2) * sizeof(*ret->val.d));
				ret->val.d[i].key = _be_decode_str(data, data_len);
#ifdef BE_DEBUG_DECODE 
			fprintf(stderr, "bencode::_be_decode() dictionary get val\n");
#endif
				ret->val.d[i  ].val = _be_decode(data, data_len);
				ret->val.d[i+1].val = NULL ; // ensures termination of loops based on 0x0 value, otherwise, uninitialized 
													  // memory occurs if(ret->val.d[i].key == 0x0 && ret->val.d[i].val != NULL)
													  // when calling be_free 8 lines below this point...

				if ((ret->val.d[i].key == NULL) || (ret->val.d[i].val == NULL))
				{
					/* failed decode - kill decode */
#ifdef BE_DEBUG_DECODE 
			fprintf(stderr, "bencode::_be_decode() failed dict decode - kill\n");
#endif
					be_free(ret);
					return NULL;
				}
				++i;
			}
			--(*data_len);
			++(*data);

			/* empty dictionary case. */
			if (i == 0)
			{
				ret->val.d = (be_dict *) realloc(ret->val.d, 1 * sizeof(*ret->val.d));
			}


			ret->val.d[i].val = NULL;
			return ret;
		}

		/* integers */
		case 'i': {
			ret = be_alloc(BE_INT);
#ifdef BE_DEBUG_DECODE 
			fprintf(stderr, "bencode::_be_decode() found int\n");
#endif

			--(*data_len);
			++(*data);
			ret->val.i = _be_decode_int(data, data_len);
			if (**data != 'e')
			{
#ifdef BE_DEBUG_DECODE 
				fprintf(stderr, "bencode::_be_decode() reject data != e - kill\n");
#endif
				be_free(ret);
				return NULL;
			}
			--(*data_len);
			++(*data);

			return ret;
		}

		/* byte strings */
		case '0'...'9': {
			ret = be_alloc(BE_STR);
#ifdef BE_DEBUG_DECODE 
			fprintf(stderr, "bencode::_be_decode() found string\n");
#endif

			ret->val.s = _be_decode_str(data, data_len);

			return ret;
		}

		/* invalid */
		default:
#ifdef BE_DEBUG_DECODE 
			fprintf(stderr, "bencode::_be_decode() found invalid - kill\n");
#endif
			return NULL;
			break;
	}

	return ret;
}

be_node *be_decoden(const char *data, long long len)
{
	return _be_decode(&data, &len);
}

be_node *be_decode(const char *data)
{
	return be_decoden(data, strlen(data));
}

static inline void _be_free_str(char *str)
{
	if (str)
		free(str - sizeof(long long));
}
void be_free(be_node *node)
{
	switch (node->type) {
		case BE_STR:
			_be_free_str(node->val.s);
			break;

		case BE_INT:
			break;

		case BE_LIST: {
			unsigned int i;
			for (i = 0; node->val.l[i]; ++i)
				be_free(node->val.l[i]);
			free(node->val.l);
			break;
		}

		case BE_DICT: {
			unsigned int i;
			for (i = 0; node->val.d[i].val; ++i) {
				_be_free_str(node->val.d[i].key);
				be_free(node->val.d[i].val);
			}
			free(node->val.d);
			break;
		}
	}
	free(node);
}

#ifdef BE_DEBUG
#include <stdio.h>
#include <stdint.h>

static void _be_dump_indent(ssize_t indent)
{
	while (indent-- > 0)
		printf("    ");
}
static void _be_dump(be_node *node, ssize_t indent)
{
	size_t i;

	_be_dump_indent(indent);
	indent = abs(indent);

	switch (node->type) {
		case BE_STR:
			be_dump_str(node);
			//printf("str = %s (len = %lli)\n", node->val.s, be_str_len(node));
			break;

		case BE_INT:
			printf("int = %lli\n", node->val.i);
			break;

		case BE_LIST:
			puts("list [");

			for (i = 0; node->val.l[i]; ++i)
				_be_dump(node->val.l[i], indent + 1);

			_be_dump_indent(indent);
			puts("]");
			break;

		case BE_DICT:
			puts("dict {");

			for (i = 0; node->val.d[i].val; ++i) {
				_be_dump_indent(indent + 1);
				printf("%s => ", node->val.d[i].key);
				_be_dump(node->val.d[i].val, -(indent + 1));
			}

			_be_dump_indent(indent);
			puts("}");
			break;
	}
}
void be_dump(be_node *node)
{
	_be_dump(node, 0);
}

void be_dump_str(be_node *node)
{
	if (node->type != BE_STR)
	{
		printf("be_dump_str(): error not a string\n");
		return;
	}

	int len = be_str_len(node);
	int i = 0;
	printf("str[%d] = ", len);
	for(i = 0; i < len; i++)
	{
		/* sensible chars */
		if ((node->val.s[i] > 31) && (node->val.s[i] < 127))
		{
			printf("%c", node->val.s[i]);
		}
		else
		{
			printf("[%d]", node->val.s[i]);
		}
	}
	printf("\n");
}


#endif

/******************** New Functions added by drBob *************
 * Output bencode
 *
 */

int be_encode(be_node *node, char *str, int len)
{
	size_t i;
	int loc = 0;

	switch (node->type) {
		case BE_STR:
			bd_snprintf(str, len, "%lli:", be_str_len(node));
			loc += strlen(&(str[loc]));

			memcpy(&(str[loc]), node->val.s, be_str_len(node));
			loc += be_str_len(node);
			break;

		case BE_INT:
			bd_snprintf(str, len, "i%llie", node->val.i);
			loc += strlen(&(str[loc]));
			break;

		case BE_LIST:

			snprintf(str, len, "l");
			loc += 1;

			for (i = 0; node->val.l[i]; ++i)
			{
				loc += be_encode(node->val.l[i], &(str[loc]), len-loc);
			}

			snprintf(&(str[loc]), len - loc, "e");
			loc += 1;

			break;

		case BE_DICT:
			snprintf(str, len, "d");
			loc += 1;

			for (i = 0; node->val.d[i].val; ++i) {
				
				/* assumption that key must be ascii! */
				snprintf(&(str[loc]), len-loc, "%i:%s", 
						(int) strlen(node->val.d[i].key),
						node->val.d[i].key);
				loc += strlen(&(str[loc]));
				loc += be_encode(node->val.d[i].val, &(str[loc]), len-loc);
			}

			snprintf(&(str[loc]), len - loc, "e");
			loc += 1;

			break;
	}
	return loc;
}

/* hackish way to create nodes! */
be_node *be_create_dict()
{
	be_node *n = be_decode("de");
	return n;
}


be_node *be_create_list()
{
	be_node *n = be_decode("le");
	return n;
}

be_node *be_create_str(const char *str)
{
	
	/* must */
	be_node *n = NULL;
	int len = strlen(str);
	long long int sllen = len;
	char *_ret = (char *) malloc(sizeof(sllen) + len + 1);
	if(_ret == NULL)
	{
		fprintf(stderr, "(EE) bencode::be_create_str(): "
		                "ERROR. cannot allocate memory for %lu bytes.\n"
		        , len+1+sizeof(sllen) );
		return n;
	}
	char *ret = NULL;
	n = be_alloc(BE_STR);

	memcpy(_ret, &sllen, sizeof(sllen));
	ret = _ret + sizeof(sllen);
	memcpy(ret, str, len);
	ret[len] = '\0';

	n->val.s = ret;
	
	return n;
}

be_node *be_create_str_wlen(const char *str, int len) /* not including \0 */
{
	
	/* must */
	be_node *n = NULL;
	long long int sllen = len;
	char *_ret = (char *) malloc(sizeof(sllen) + len + 1);
	if(_ret == NULL)
	{
		fprintf(stderr, "(EE) bencode::be_create_str_wlen(): "
		                "ERROR. cannot allocate memory for %lu bytes.\n"
		        , len+1+sizeof(sllen) );
		return n;
	}
	char *ret = NULL;
	n = be_alloc(BE_STR);

	memcpy(_ret, &sllen, sizeof(sllen));
	ret = _ret + sizeof(sllen);
	memcpy(ret, str, len);
	ret[len] = '\0';

	n->val.s = ret;
	
	return n;
}

be_node *be_create_int(long long int num)
{
	/* must */
	be_node *n = be_alloc(BE_INT);
	n->val.i = num;
	return n;
}

int be_add_keypair(be_node *dict, const char *str, be_node *node)
{
	int i = 0;

	/* only if dict type */
	if (dict->type != BE_DICT)
	{
		return 0;
	}	

	// get to end of dict.
	for(i = 0; dict->val.d[i].val; i++)
		;//Silent empty body for loop for clang

	//fprintf(stderr, "be_add_keypair() i = %d\n",i);

	/* realloc space */
        dict->val.d = (be_dict *) realloc(dict->val.d, (i + 2) * sizeof(*dict->val.d));

	/* stupid key storage system */
	int len = strlen(str);
	long long int sllen = len;
	char *_ret = (char *) malloc(sizeof(sllen) + len + 1);
	if(_ret == NULL)
	{
		fprintf(stderr, "(EE) bencode::be_create_str_wlen(): "
		                "ERROR. cannot allocate memory for %lu bytes.\n"
		        , len+1+sizeof(sllen) );
		return 0;
	}
	char *ret = NULL;

	//fprintf(stderr, "be_add_keypair() key len = %d\n",len);

	memcpy(_ret, &sllen, sizeof(sllen));
	ret = _ret + sizeof(sllen);
	memcpy(ret, str, len);
	ret[len] = '\0';

        dict->val.d[i].key = ret;
        dict->val.d[i].val = node;
	i++;
        dict->val.d[i].val = NULL;

	return 1;
}


int be_add_list(be_node *list, be_node *node)
{
	int i = 0;

	/* only if dict type */
	if (list->type != BE_LIST)
	{
		return 0;
	}	

	// get to end of dict.
	for(i = 0; list->val.l[i]; i++)
		;//Silent empty body for loop for clang

	/* realloc space */
        list->val.l = (be_node **) realloc(list->val.l, (i + 2) * sizeof(*list->val.l));
        list->val.l[i] = node;
        ++i;
        list->val.l[i] = NULL;

	return 1;
}





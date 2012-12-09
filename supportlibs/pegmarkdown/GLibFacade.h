/*
 *	GLibFacade.h
 *	MultiMarkdown
 *	
 *	Created by Daniel Jalkut on 7/26/11.
 *	Copyright 2011 __MyCompanyName__. All rights reserved.
 */

#ifndef __MARKDOWN_GLIB_FACADE__
#define __MARKDOWN_GLIB_FACADE__

/* peg_markdown uses the link symbol for its own purposes */
#define link MARKDOWN_LINK_IGNORED
#include <unistd.h>
#undef link

#include <stdbool.h>
#include <ctype.h>

typedef int gboolean;
typedef char gchar;

/* This style of bool is used in shared source code */
#define FALSE false
#define TRUE true

/* WE implement minimal mirror implementations of GLib's GString and GSList 
 * sufficient to cover the functionality required by MultiMarkdown.
 *
 * NOTE: THese are 100% clean, from-scratch implementations using only the 
 * GLib function prototype as guide for behavior.
 */

typedef struct 
{	
	/* Current UTF8 byte stream this string represents */
	char* str;

	/* Where in the str buffer will we add new characters */
	/* or append new strings? */
	int currentStringBufferSize;
	int currentStringLength;
} GString;

GString* g_string_new(char *startingString);
char* g_string_free(GString* ripString, bool freeCharacterData);

void g_string_append_c(GString* baseString, char appendedCharacter);
void g_string_append(GString* baseString, char *appendedString);

void g_string_prepend(GString* baseString, char* prependedString);

void g_string_append_printf(GString* baseString, char* format, ...);

/* Just implement a very simple singly linked list. */

typedef struct _GSList
{
	void* data;	
	struct _GSList* next;
} GSList;

void g_slist_free(GSList* ripList);
GSList* g_slist_prepend(GSList* targetElement, void* newElementData);
GSList* g_slist_reverse(GSList* theList);

#endif

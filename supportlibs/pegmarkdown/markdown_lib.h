#ifndef MARKDOWN_LIB_H
#define MARKDOWN_LIB_H

#include <stdlib.h>
#include <stdio.h>
#include <glib.h>


#ifdef __cplusplus
extern "C" {
#endif


enum markdown_extensions {
    EXT_SMART            = 0x01,
    EXT_NOTES            = 0x02,
    EXT_FILTER_HTML      = 0x04,
    EXT_FILTER_STYLES    = 0x08
};

enum markdown_formats {
    HTML_FORMAT,
    LATEX_FORMAT,
    GROFF_MM_FORMAT,
    ODF_FORMAT
};

GString * markdown_to_g_string(char *text, int extensions, int output_format);
char * markdown_to_string(char *text, int extensions, int output_format);


#ifdef __cplusplus
}
#endif

/* vim: set ts=4 sw=4 : */
#endif


/**********************************************************************

  odf.c - Utility routines to enable ODF support in peg-multimarkdown.
  (c) 2011 Fletcher T. Penney (http://fletcherpenney.net/).

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License or the MIT
  license.  See LICENSE for details.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

 ***********************************************************************/

#include "odf.h"


void print_odf_header(GString *out){
    
    /* Insert required XML header */
    g_string_append_printf(out,
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" \
"<office:document xmlns:office=\"urn:oasis:names:tc:opendocument:xmlns:office:1.0\"\n" \
"     xmlns:style=\"urn:oasis:names:tc:opendocument:xmlns:style:1.0\"\n" \
"     xmlns:text=\"urn:oasis:names:tc:opendocument:xmlns:text:1.0\"\n" \
"     xmlns:table=\"urn:oasis:names:tc:opendocument:xmlns:table:1.0\"\n" \
"     xmlns:draw=\"urn:oasis:names:tc:opendocument:xmlns:drawing:1.0\"\n" \
"     xmlns:fo=\"urn:oasis:names:tc:opendocument:xmlns:xsl-fo-compatible:1.0\"\n" \
"     xmlns:xlink=\"http://www.w3.org/1999/xlink\"\n" \
"     xmlns:dc=\"http://purl.org/dc/elements/1.1/\"\n" \
"     xmlns:meta=\"urn:oasis:names:tc:opendocument:xmlns:meta:1.0\"\n" \
"     xmlns:number=\"urn:oasis:names:tc:opendocument:xmlns:datastyle:1.0\"\n" \
"     xmlns:svg=\"urn:oasis:names:tc:opendocument:xmlns:svg-compatible:1.0\"\n" \
"     xmlns:chart=\"urn:oasis:names:tc:opendocument:xmlns:chart:1.0\"\n" \
"     xmlns:dr3d=\"urn:oasis:names:tc:opendocument:xmlns:dr3d:1.0\"\n" \
"     xmlns:math=\"http://www.w3.org/1998/Math/MathML\"\n" \
"     xmlns:form=\"urn:oasis:names:tc:opendocument:xmlns:form:1.0\"\n" \
"     xmlns:script=\"urn:oasis:names:tc:opendocument:xmlns:script:1.0\"\n" \
"     xmlns:config=\"urn:oasis:names:tc:opendocument:xmlns:config:1.0\"\n" \
"     xmlns:ooo=\"http://openoffice.org/2004/office\"\n" \
"     xmlns:ooow=\"http://openoffice.org/2004/writer\"\n" \
"     xmlns:oooc=\"http://openoffice.org/2004/calc\"\n" \
"     xmlns:dom=\"http://www.w3.org/2001/xml-events\"\n" \
"     xmlns:xforms=\"http://www.w3.org/2002/xforms\"\n" \
"     xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\"\n" \
"     xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n" \
"     xmlns:rpt=\"http://openoffice.org/2005/report\"\n" \
"     xmlns:of=\"urn:oasis:names:tc:opendocument:xmlns:of:1.2\"\n" \
"     xmlns:xhtml=\"http://www.w3.org/1999/xhtml\"\n" \
"     xmlns:grddl=\"http://www.w3.org/2003/g/data-view#\"\n" \
"     xmlns:tableooo=\"http://openoffice.org/2009/table\"\n" \
"     xmlns:field=\"urn:openoffice:names:experimental:ooo-ms-interop:xmlns:field:1.0\"\n" \
"     xmlns:formx=\"urn:openoffice:names:experimental:ooxml-odf-interop:xmlns:form:1.0\"\n" \
"     xmlns:css3t=\"http://www.w3.org/TR/css3-text/\"\n" \
"     office:version=\"1.2\"\n" \
"     grddl:transformation=\"http://docs.oasis-open.org/office/1.2/xslt/odf2rdf.xsl\"\n" \
"     office:mimetype=\"application/vnd.oasis.opendocument.text\">\n");
    
    /* Font Declarations */
    g_string_append_printf(out, "<office:font-face-decls>\n" \
    "   <style:font-face style:name=\"Courier New\" svg:font-family=\"'Courier New'\"\n" \
    "                    style:font-adornments=\"Regular\"\n" \
    "                    style:font-family-generic=\"modern\"\n" \
    "                    style:font-pitch=\"fixed\"/>\n" \
    "</office:font-face-decls>\n");
    
    /* Append basic style information */
    g_string_append_printf(out, "<office:styles>\n" \
    "<style:style style:name=\"Standard\" style:family=\"paragraph\" style:class=\"text\">\n" \
    "      <style:paragraph-properties fo:margin-top=\"0in\" fo:margin-bottom=\"0.15in\"" \
    "     fo:text-align=\"justify\" style:justify-single-word=\"false\"/>\n" \
    "   </style:style>\n" \
    "<style:style style:name=\"Preformatted_20_Text\" style:display-name=\"Preformatted Text\"\n" \
    "             style:family=\"paragraph\"\n" \
    "             style:parent-style-name=\"Standard\"\n" \
    "             style:class=\"html\">\n" \
    "   <style:paragraph-properties fo:margin-top=\"0in\" fo:margin-bottom=\"0in\" fo:text-align=\"start\"\n" \
    "                               style:justify-single-word=\"false\"/>\n" \
    "   <style:text-properties style:font-name=\"Courier New\" fo:font-size=\"11pt\"\n" \
    "                          style:font-name-asian=\"Courier New\"\n" \
    "                          style:font-size-asian=\"11pt\"\n" \
    "                          style:font-name-complex=\"Courier New\"\n" \
    "                          style:font-size-complex=\"11pt\"/>\n" \
    "</style:style>\n" \
    "<style:style style:name=\"Source_20_Text\" style:display-name=\"Source Text\"\n" \
    "             style:family=\"text\">\n" \
    "   <style:text-properties style:font-name=\"Courier New\" style:font-name-asian=\"Courier New\"\n" \
    "                          style:font-name-complex=\"Courier New\"\n" \
    "                          fo:font-size=\"11pt\"/>\n" \
    "</style:style>\n" \
    "<style:style style:name=\"List\" style:family=\"paragraph\"\n" \
    "             style:parent-style-name=\"Standard\"\n" \
    "             style:class=\"list\">\n" \
    "   <style:paragraph-properties fo:text-align=\"start\" style:justify-single-word=\"false\"/>\n" \
    "   <style:text-properties style:font-size-asian=\"12pt\"/>\n" \
    "</style:style>\n" \
    "<style:style style:name=\"Quotations\" style:family=\"paragraph\"\n" \
    "             style:parent-style-name=\"Standard\"\n" \
    "             style:class=\"html\">\n" \
    "   <style:paragraph-properties fo:margin-left=\"0.3937in\" fo:margin-right=\"0.3937in\" fo:margin-top=\"0in\"\n" \
    "                               fo:margin-bottom=\"0.1965in\"\n" \
    "                               fo:text-align=\"justify\"" \
    "                               style:justify-single-word=\"false\"" \
    "                               fo:text-indent=\"0in\"\n" \
    "                               style:auto-text-indent=\"false\"/>\n" \
    "</style:style>\n" \
    "<style:style style:name=\"Table_20_Heading\" style:display-name=\"Table Heading\"\n" \
    "             style:family=\"paragraph\"\n" \
    "             style:parent-style-name=\"Table_20_Contents\"\n" \
    "             style:class=\"extra\">\n" \
    "   <style:paragraph-properties fo:text-align=\"center\" style:justify-single-word=\"false\"\n" \
    "                               text:number-lines=\"false\"\n" \
    "                               text:line-number=\"0\"/>\n" \
    "   <style:text-properties fo:font-weight=\"bold\" style:font-weight-asian=\"bold\"\n" \
    "                          style:font-weight-complex=\"bold\"/>\n" \
    "</style:style>\n" \
    "<style:style style:name=\"Horizontal_20_Line\" style:display-name=\"Horizontal Line\"\n" \
    "             style:family=\"paragraph\"\n" \
    "             style:parent-style-name=\"Standard\"\n" \
    "             style:class=\"html\">\n" \
    "   <style:paragraph-properties fo:margin-top=\"0in\" fo:margin-bottom=\"0.1965in\"\n" \
    "                               style:border-line-width-bottom=\"0.0008in 0.0138in 0.0008in\"\n" \
    "                               fo:padding=\"0in\"\n" \
    "                               fo:border-left=\"none\"\n" \
    "                               fo:border-right=\"none\"\n" \
    "                               fo:border-top=\"none\"\n" \
    "                               fo:border-bottom=\"0.0154in double #808080\"\n" \
    "                               text:number-lines=\"false\"\n" \
    "                               text:line-number=\"0\"\n" \
    "                               style:join-border=\"false\"/>\n" \
    "   <style:text-properties fo:font-size=\"6pt\" style:font-size-asian=\"6pt\" style:font-size-complex=\"6pt\"/>\n" \
    "</style:style>\n" \
    "</office:styles>\n");

    /* Automatic style information */
    g_string_append_printf(out, "<office:automatic-styles>" \
    "   <style:style style:name=\"MMD-Italic\" style:family=\"text\">\n" \
    "      <style:text-properties fo:font-style=\"italic\" style:font-style-asian=\"italic\"\n" \
    "                             style:font-style-complex=\"italic\"/>\n" \
    "   </style:style>\n" \
    "   <style:style style:name=\"MMD-Bold\" style:family=\"text\">\n" \
    "      <style:text-properties fo:font-weight=\"bold\" style:font-weight-asian=\"bold\"\n" \
    "                             style:font-weight-complex=\"bold\"/>\n" \
    "   </style:style>\n" \
    "<style:style style:name=\"MMD-Table\" style:family=\"paragraph\" style:parent-style-name=\"Standard\">\n" \
    "   <style:paragraph-properties fo:margin-top=\"0in\" fo:margin-bottom=\"0.05in\"/>\n" \
    "</style:style>\n" \
    "<style:style style:name=\"MMD-Table-Center\" style:family=\"paragraph\" style:parent-style-name=\"MMD-Table\">\n" \
    "   <style:paragraph-properties fo:text-align=\"center\" style:justify-single-word=\"false\"/>\n" \
    "</style:style>\n" \
    "<style:style style:name=\"MMD-Table-Right\" style:family=\"paragraph\" style:parent-style-name=\"MMD-Table\">\n" \
    "   <style:paragraph-properties fo:text-align=\"right\" style:justify-single-word=\"false\"/>\n" \
    "</style:style>\n" \
    "<style:style style:name=\"P2\" style:family=\"paragraph\" style:parent-style-name=\"Standard\"\n" \
    "             style:list-style-name=\"L2\">\n" \
    "<style:paragraph-properties fo:text-align=\"start\" style:justify-single-word=\"false\"/>\n" \
    "</style:style>\n" \
	"<style:style style:name=\"fr1\" style:family=\"graphic\" style:parent-style-name=\"Frame\">\n" \
	"   <style:graphic-properties style:print-content=\"false\" style:vertical-pos=\"top\"\n" \
	"                             style:vertical-rel=\"baseline\"\n" \
	"                             fo:padding=\"0in\"\n" \
	"                             fo:border=\"none\"\n" \
	"                             style:shadow=\"none\"/>\n" \
	"</style:style>\n" \
    "</office:automatic-styles>\n" \
    "<style:style style:name=\"P1\" style:family=\"paragraph\" style:parent-style-name=\"Standard\"\n" \
    "             style:list-style-name=\"L1\"/>\n" \
    "<text:list-style style:name=\"L1\">\n" \
    "   <text:list-level-style-bullet />\n" \
    "</text:list-style>\n" \
    "<text:list-style style:name=\"L2\">\n" \
    "   <text:list-level-style-number />\n" \
    "</text:list-style>\n");
}

void print_odf_footer(GString *out) {
    g_string_append_printf(out, "</office:text>\n</office:body>\n</office:document>");
}


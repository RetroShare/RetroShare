<xsl:stylesheet version="1.0"
   xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

   <xsl:output method="text"/>

   <xsl:strip-space elements="*"/>

   <xsl:template match="context">
     <xsl:text>!insertmacro LANG_STRING </xsl:text>
     <xsl:value-of select="./name"/>
     <xsl:text> "</xsl:text>
     <xsl:choose>
       <xsl:when test="./message/translation!=''"><xsl:value-of select="./message/translation"/></xsl:when>
       <xsl:otherwise><xsl:value-of select="./message/source"/></xsl:otherwise>
     </xsl:choose>
     <xsl:text>"
</xsl:text>
   </xsl:template>

</xsl:stylesheet>
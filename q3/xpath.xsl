<xsl:stylesheet
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:saxon="http://icl.com/saxon"
  version="1.0">
  <xsl:output method="text"/>
  <xsl:param name="path"/>
  <xsl:template match="/">
    <xsl:value-of select="saxon:evaluate($path)"/>
  </xsl:template>
</xsl:stylesheet>

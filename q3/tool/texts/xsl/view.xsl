<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
	<xsl:output method="html"/>
	
	<xsl:template match="texts">
		<table border="1">
			<tr>
				<td><strong>Name</strong></td>
				<td colspan="2"></td>
				<td colspan="2"></td>
			</tr>
			<xsl:for-each select="text">
				<tr>
					<td><xsl:value-of select="@name"/></td>
					<td><a href="#" onclick="edit('{position()}')">Edit</a></td>
					<td><a href="#" onclick="remove('{position()}')">Delete</a></td>
					<td>
						<xsl:if test="position() != 1">
							<a href="#" onclick="up('{position()}')">Up</a>
						</xsl:if>
					</td>
					<td>
						<xsl:if test="position() != last()">
							<a href="#" onclick="down('{position()}')">Down</a>
						</xsl:if>
					</td>
				</tr>
			</xsl:for-each>
		</table>
		<p>
			<input type="button" value="New" onclick="create()"/>
		</p>
		<p>
			<input type="button" value="Save" onclick="save()"/>
			<input type="button" value="Reload" onclick="reload()"/>
		</p>
	</xsl:template>
</xsl:stylesheet>

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
	<xsl:output method="html"/>
	
	<xsl:template match="signatures">
		<table border="1">
			<tr>
				<td><strong>Name</strong></td>
				<td><strong>Account</strong></td>
				<td><strong>Default</strong></td>
				<td colspan="2"></td>
			</tr>
			<xsl:for-each select="signature">
				<tr>
					<td><xsl:value-of select="@name"/></td>
					<td><xsl:value-of select="@account"/></td>
					<td><xsl:value-of select="@default"/></td>
					<td><input type="button" value="Edit" onclick="edit('{position()}')"/></td>
					<td><input type="button" value="Delete" onclick="remove('{position()}')"/></td>
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

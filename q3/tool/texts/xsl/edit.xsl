<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
	<xsl:output method="html"/>
	
	<xsl:template match="text">
		<form name="editForm">
			<table>
				<tr>
					<td><strong>Name: </strong></td>
					<td width="100%"><input name="name" type="text" value="{@name}"/></td>
				</tr>
				<tr>
					<td colspan="2">
						<textarea name="body" cols="60" rows="10"><xsl:value-of select="."/></textarea>
					</td>
				</tr>
			</table>
			<p>
				<input type="button" value="OK" onclick="edit_ok()"/>
				<input type="button" value="Cancel" onclick="edit_cancel()"/>
			</p>
		</form>
	</xsl:template>
</xsl:stylesheet>

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
	<xsl:output method="html"/>
	
	<xsl:param name="accounts"/>
	
	<xsl:template match="signature">
		<form name="editForm">
			<table>
				<tr>
					<td><strong>Name: </strong></td>
					<td width="100%"><input name="name" type="text" value="{@name}"/></td>
				</tr>
				<tr>
					<td><strong>Account: </strong></td>
					<td width="100%">
						<xsl:variable name="account" select="@account"/>
						<xsl:variable name="regex" select="$account and not($accounts/accounts/account[.=$account])"/>
						<xsl:text>Select: </xsl:text>
						<select name="account">
							<option value="__none__">
								<xsl:if test="not($account)">
									<xsl:attribute name="selected">selected</xsl:attribute>
								</xsl:if>
								<xsl:text>(None)</xsl:text>
							</option>
							<xsl:for-each select="$accounts/accounts/account">
								<option value="{.}">
									<xsl:if test=".=$account">
										<xsl:attribute name="selected">selected</xsl:attribute>
									</xsl:if>
									<xsl:value-of select="."/>
								</option>
							</xsl:for-each>
							
							<option value="__regex__">
								<xsl:if test="$regex">
									<xsl:attribute name="selected">selected</xsl:attribute>
								</xsl:if>
								<xsl:text>(Regex)</xsl:text>
							</option>
						</select>
						<xsl:text> Regex: </xsl:text>
						<input name="account_regex" type="text">
							<xsl:if test="$regex">
								<xsl:attribute name="value">
									<xsl:value-of select="@account"/>
								</xsl:attribute>
							</xsl:if>
						</input>
					</td>
				</tr>
				<tr>
					<td><strong>Default: </strong></td>
					<td width="100%">
						<input name="isdefault" type="checkbox">
							<xsl:if test="@default = 'true'">
								<xsl:attribute name="checked">checked</xsl:attribute>
							</xsl:if>
						</input>
					</td>
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

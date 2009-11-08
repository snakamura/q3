=begin
=印刷するときにも引用を線で表示するにはどうすればよいですか?

((<印刷用テンプレート|URL:OtherTemplate.html>))で引用を<blockquote>に変換して、CSSで左側に線を入れると引用を線で表示して印刷できます。たとえば以下のテンプレートをprint.templateとして保存するとそれらしくなります。

 <!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">
 <html>
 
 <head>
 	<title>{@HtmlEscape(@Subject())}</title>
 	<style type="text/css">
 	body {{ margin-left: 3em; margin-right: 3em; }}
 	.header-title {{ font-weight: bold; font-size: 90%; }}
 	.header-value {{ font-size: 90%; }}
 	.body {{ font-family: monospace; }}
 	blockquote {{
 		border-width: 0px 0px 0px 2px;
 		border-color: blue;
 		border-style:solid;
 		padding-left:1em;
 		margin-left: 0.5em;
 	}}
 	</style>
 </head>
 
 <body>
 	<table border="0" cellspacing="0" cellpadding="0">
 		<tr>
 			<td colspan="2"><hr size="3" style="color: lightblue"></td>
 		</tr>
 		<tr style="padding-bottom: 5px;">
 			<td width="70" nowrap class="header-title">To: </td>
 			<td width="100%" class="header-value">{@HtmlEscape(@FormatAddress(To))}</td>
 		</tr>
 		{@If(Cc, @Concat('
 		<tr style="padding-bottom: 5px;">
 			<td width="70" nowrap class="header-title">Cc: </td>
 			<td width="100%" class="header-value">',
 			@HtmlEscape(@FormatAddress(Cc)),
 			'</td>
 		</tr>'), '')}
 		<tr style="padding-bottom: 5px;">
 			<td width="70" nowrap class="header-title">From: </td>
 			<td width="100%" class="header-value">{@HtmlEscape(@FormatAddress(From))}</td>
 		</tr>
 		<tr style="padding-bottom: 5px;">
 			<td width="70" nowrap class="header-title">Date: </td>
 			<td width="100%" class="header-value">{@FormatDate(@Date(Date), '%D %M1 %Y4 %h:%m:%s %z')}</td>
 		</tr>
 		<tr>
 			<td width="70" nowrap class="header-title">Subject: </td>
 			<td width="100%" class="header-value">{@HtmlEscape(@Subject())}</td>
 		</tr>
 		<tr>
 			<td colspan="2"><hr size="3" style="color: lightblue"></td>
 		</tr>
 		<tr>
 			<td colspan="2" class="body"><p>{-@Script(<<END
 function getQuoteDepth(s) {
 	var depth = 0;
 	var n = 0;
 	for (n = 0; n < s.length; ++n) {
 		var c = s.charAt(n);
 		if (c == '>')
 			++depth;
 		else if (c == ' ' || c == '\t')
 			;
 		else
 			break;
 	}
 	return { "index": n, "depth": depth };
 }
 
 function escape(s) {
 	return s.replace(/&/g, "&amp;").replace(/</g, "&lt;");
 }
 
 var body = "";
 var lines = arguments(0).split("\n");
 var quote = 0;
 for (var n = 0; n < lines.length; ++n) {
 	var line = lines[n];
 	var qd = getQuoteDepth(line);
 	var q = qd.depth;
 	
 	if (q > quote) {
 		for (var m = 0; m < q - quote; ++m)
 			body += "<blockquote>";
 	}
 	else if (q < quote) {
 		for (var m = 0; m < quote - q; ++m)
 			body += "</blockquote>";
 	}
 	quote = q;
 	body += escape(line.substring(qd.index));
 	body += "<br>";
 }
 result.value = body;
 END
 , 'JScript', @Body('', :BODY-RFC822INLINE))-}</p></td>
 		</tr>
 	</table>
 </body>
 
 </html>

=end

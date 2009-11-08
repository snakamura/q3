=begin
=フォント

各ビューのフォントは((<オプションの設定|URL:Options.html>))で個別に設定することができます。


==フォントグループ

さらにメッセージビューとプレビューはメッセージに応じてフォントを切り替えることができます。例えば、英語のメッセージはTahomaで、メールマガジンは等幅のＭＳ ゴシックで、それ以外はＭＳ Ｐゴシックで、というような設定ができます。

この機能はUIからは設定することができませんので、手動で設定する必要があります。まず、((<fonts.xml|URL:FontsXml.html>))でフォントグループを定義します。そして、((<qmail.xml|URL:QmailXml.html>))でメッセージビュー、またはプレビューで使用するフォントグループを指定します。指定するのはそれぞれMessageWindow/FontGroupと、PreviewWindow/FontGroupです。

例えば、Content-Typeのcharsetがus-asciiまたはiso-8859-xの場合にはTahomaで、「メルマガ」フォルダのメッセージはＭＳ ゴシックで、それ以外はＭＳ Ｐゴシックで表示するには以下のようなfonts.xmlを作成します。

 <?xml version="1.0" encoding="utf-8"?>
 <fonts>
  <group name="main">
   <fontSet match="@Progn(@Set('charset', @BodyCharset()),
                          @Or(@BeginWith($charset, 'iso-8859-'),
                              @Equal($charset, 'us-ascii')))">
    <font face="Tahoma" size="9"/>
   </fontSet>
   <fontSet match="@Equal(@Folder(), 'メルマガ')">
    <font face="ＭＳ ゴシック" size="9"/>
   </fontSet>
   <fontSet>
    <font face="ＭＳ Ｐゴシック" size="9"/>
   </fontSet>
  </group>
 </fonts>

そして、qmail.xmlのPreviewWindow/FontGroupにmainと指定します。また、((<ViewFontGroupアクション|URL:ViewFontGroupAction.html>))を使用してフォントグループを切り替えることもできます。

フォントグループの定義の仕方の詳細は、((<fonts.xml|URL:FontsXml.html>))を参照してください。

=end

=begin
=@Decode

 String @Decode(String s)


==説明
指定された文字列にASCII文字のみが含まれているときに、((<RFC2047|URL:http://www.ietf.org/rfc/rfc2047.txt>))に基づいてデコードします。非ASCII文字が含まれているときには何もしません。

たとえば、

 @Decode('=?iso-2022-jp?B?GyRCJUYlOSVIGyhC?=')

は「テスト」を返します。

通常ヘッダから取得した文字列はデコードされているため、この関数を使う必要はありません。


==引数
:String s
  文字列


==エラー
*引数の数が合っていない場合


==条件
なし


==例

=end

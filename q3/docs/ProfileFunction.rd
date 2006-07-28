=begin
=@Profile

 String @Profile(String path, String section, String key, String default?)


==説明
プロファイル形式のXMLファイルから値を取り出します。プロファイル形式は((<qmail.xml|URL:QmailXml.html>))と同様の形式です。詳細は、((<qmail.xml|URL:QmailXml.html>))を参照してください。

pathに空文字列を指定するとデフォルトの((<qmail.xml|URL:QmailXml.html>))から値を取得します。この場合、すでにロードされているqmail.xmlから値をロードするため高速にロードできますが、内部的に書き換えられている場合には現在のファイルの内容と異なる可能性があります。それ以外のパスを指定すると指定されたファイルをプロファイル形式のXMLファイルとして読み込み、値を返します。相対パスを指定するとメールボックスディレクトリからの相対パスとして解釈します。

sectionとkeyにはセクションの名前とキーの名前を指定します。defaultには値が存在しなかった場合に返す値を指定します。指定しなかった場合に値が存在しないと空文字列を返します。


==引数
:String path
  ファイルのパス
:String section
  セクションの名前
:String key
  キーの名前
:String default
  値が存在しなかったときに返す値


==エラー
*引数の数が合っていない場合
*ファイルの読み込みに失敗した場合（pathに空文字列以外を指定した場合）


==条件
なし


==例
 # qmail.xmlのGlobalセクションのBccの値を取得（デフォルトは1）
 @Profile('', 'Global', 'Bcc', '1')
 
 # C:\test.xmlからTestセクションのFooの値を取得
 @Profile('C:\\test.xml', 'Test', 'Foo')

=end

=begin
=signatures.xml

((<署名|URL:Signature.html>))の設定をするXMLファイルです。このファイルには、((<署名の設定|URL:OptionSignatures.html>))で設定した情報が保存されます。


==書式

===signaturesエレメント

 <signatures>
  <!-- signature -->
 </signatures>

signaturesエレメントがトップレベルエレメントになります。0個以上のsignatureエレメントを子エレメントとしておくことが出来ます。


===signatureエレメント

 <signature
  account="アカウント名"
  name="名前"
  default="デフォルトにするかどうか">
  <!-- 署名 -->
 </signature>

signatureエレメントで実際の署名を指定します。account属性にアカウント名を指定すると、そのアカウントのみで使用されるようになります。指定しない場合には全てのアカウントで使用されます。//で囲むことにより正規表現が使用できます。

name属性には名前を指定します。ここで指定した名前がエディットビューで使用されます。default属性にtrueを指定すると、デフォルトになることが出来ます。デフォルトになるとエディットビューで開いたときに最初に選択されます。複数のデフォルトがある場合には、最初に見つかったものが選択されます。


==サンプル

 <?xml version="1.0" encoding="utf-8"?>
 <signatures>
  <signature account="test1" name="Default"
   default="true">私の署名です</signature>
  <signature account="/test.*/" name="テスト"><![CDATA[
 -- 
 Hogehoge Fugafuga <hoge@fuga.com>
 ]]></signature>
 </signatures>



==スキーマ

 element signatures {
   element signature {
     ## シグニチャ
     xsd:string,
     ## 名前
     attribute name {
       xsd:string
     },
     ## このシグニチャを使用するアカウント
     ## 指定されない場合、全てのアカウント
     attribute account {
       xsd:string
     }?,
     ## デフォルトになるかどうか
     ## 指定されない場合、false
     attribute default {
       xsd:boolean
     }?
   }*
 }

=end

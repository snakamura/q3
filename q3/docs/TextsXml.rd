=begin
=texts.xml

((<定型文|URL:FixedFormText.html>))の設定をするXMLファイルです。このファイルには、((<定型文の設定|URL:OptionTexts.html>))で設定した情報が保存されます。


==書式

===textsエレメント

 <texts>
   <!-- text -->
 </texts>

textsエレメントがトップレベルエレメントになります。子エレメントとして0個以上のtextエレメントを置くことが出来ます。


===textエレメント

 <text
  name="名前">
  <!-- テキスト -->
 </text>

textエレメントには挿入する定形分を記述します。name属性に名前を指定します。


==サンプル

 <?xml version="1.0" encoding="utf-8"?>
 <texts>
  <text name="hello">こんにちは</text>
 </texts>


==スキーマ

 element texts {
   element text {
     ## テキスト
     xsd:string,
     ## 名前
     attribute name {
       xsd:string
     }
   }*
 }

=end

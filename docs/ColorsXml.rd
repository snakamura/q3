=begin
=colors.xml

((<リストビューの色|URL:Colors.html>))の設定をするXMLファイルです。このファイルには、((<色の設定|URL:OptionColors.html>))で設定した情報が保存されます。


==書式

===colorsエレメント

 <colors>
  <! -- colorSet -->
 </colors>

colorsエレメントがトップレベルエレメントになります。colorsエレメント以下には、0個以上のcolorSetエレメントを置くことが出来ます。


===colorSetエレメント

 <colorSet
  account="アカウント名"
  folder="フォルダ名">
  <!-- color -->
 </colorSet>

colorSetエレメントは色分け時に使用される色セットを指定します。account属性にアカウント名を、folder属性にフォルダ名を指定することが出来ます。両方とも//で囲むことにより正規表現が使用できます。これらの属性は省略可能で、省略された場合にはそれぞれ全てのアカウント・フォルダにマッチします。たとえば、account属性のみを指定してfolder属性を指定しないと、指定したアカウントの全てのフォルダにマッチします。


===colorエレメント

 <color
  match="マクロ"
  description="説明">
  <!-- foreground, background, style -->
 </color>

colorエレメントは色とフォントのスタイルを指定します。match属性にはマクロを、description属性には説明を指定します。


===foregroundエレメント

 <foreground>
  色
 </foreground>

foregroundエレメントは文字色を指定します。色はRRGGBB形式で指定します。


===backgroundエレメント

 <background>
  色
 </background>

backgroundエレメントは背景色を指定します。色はRRGGBB形式で指定します。


===styleエレメント

 <style>
  フォントのスタイル
 </style>

styleエレメントにはフォントのスタイルを指定することができます。指定可能なのは、regularとboldです。


==サンプル

 <?xml version="1.0" encoding="utf-8"?>
 <colors>
  <colorSet account="test" folder="Inbox">
   <color match="@Not(@Seen())">
    <foreground>ff0000</foreground>
    <style>bold</style>
   </color>
   <color match="@BeginWith(%Subject, '[Qs:')">
    <foreground>00ff00</foreground>
   </color>
  </colorSet>
 </colors>


==スキーマ

 element colors {
   element colorSet {
     element color {
       ## 文字色
       element foreground {
         xsd:string {
           pattern = "[0-9a-fA-F]{6}"
         }
       }?,
       ## 背景色
       element background {
         xsd:string {
           pattern = "[0-9a-fA-F]{6}"
         }
       }?,
       ## フォントスタイル
       element style {
         "regular" | "bold"
       }?,
       ## 色が適用される条件（マクロ）
       attribute match {
         xsd:string
       },
       attribute description {
         xsd:string
       }?
     }*,
     ## アカウント
     ## 指定されない場合、全てのアカウント
     attribute account {
       xsd:string
     }?,
     ## フォルダ
     ## 指定されない場合、全てのフォルダ
     attribute folder {
       xsd:string
     }?
   }*
 }

=end

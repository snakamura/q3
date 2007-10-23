=begin
=fonts.xml

フォントの設定をするXMLファイルです。


==書式

===fontsエレメント

 <fonts>
  <!-- group -->
 </fonts>

fontsエレメントがトップレベルエレメントになります。fontsエレメント以下には0個以上のgroupエレメントを置くことができます。


===groupエレメント

 <group
  name="名前">
  <!-- fontSet -->
 </group>

groupエレメントはフォントグループを表します。name属性にグループ名を指定します。groupエレメント以下に0個以上のfontSetエレメントを置くことができます。


===fontSetエレメント

 <fontSet
  match="マクロ"
  lineSpacing="行間">
  <!-- font -->
 </fontSet>

fontSetエレメントはフォントセットを表します。

match属性にマクロを指定します。表示するメッセージに対してこのマクロを評価した結果がTrueになるとこのフォントセットが使われます。

lineSpacing属性には行間をピクセル単位で指定します。


===fontエレメント

 <font
  face="フォント名"
  size="サイズ"
  style="スタイル"
  charset="文字セット"/>

font属性には実際に使われるフォントを指定します。face属性にはフォント名を、size属性にはサイズをポイント単位で指定します。sizeを指定しない場合には9ポイントになります。style属性にはbold, italic, underline, strikeoutの組み合わせを空白文字区切りで指定できます。文字セットにはフォントの文字セットを指定します。


==サンプル

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

==スキーマ

 element fonts {
   element group {
     element fontSet {
       element font {
         ## フォント名
         attribute face {
           xsd:string
         },
         ## サイズ（ポイント）
         attribute size {
           xsd:float
         }?,
         ## スタイル
         ## bold, italic, underline, strikeout
         attribute style {
           xsd:string
         }?,
         ## 文字セット
         attribute charset {
           xsd:nonNegativeInteger
         }?
       },
       ## マクロ
       attribute match {
         xsd:string
       }?,
       ## 行間
       attribute lineSpacing {
         xsd:nonNegativeInteger
       }?
     }*,
     ## グループ名
     attribute name {
       xsd:string
     }
   }*
 }

=end

=begin
=filters.xml

((<フィルタ|URL:Filter.html>))の設定をするXMLファイルです。このファイルには、((<フィルタの設定|URL:OptionFilters.html>))で設定した情報が保存されます。


==書式

===filtersエレメント

 <filters>
  <!-- filter -->
 </filters>

filtersエレメントがトップレベルエレメントになります。filtersエレメント以下には0個以上のfilterエレメントを置くことができます。


===filterエレメント

 <filter
  name="名前">
  マクロ
 </filter>

filterエレメントにはフィルタの名前とフィルタするのに使用するマクロを指定します。name属性にはフィルタの名前を指定します。


==サンプル

 <?xml version="1.0" encoding="utf-8"?>
 <filters>
  <filter name="未読のみ">@Not(@Seen())</filter>
  <filter name="10KB以上">@Greater(@Size(), 10240)</filter>
 </filters>


==スキーマ

 element filters {
   element filter {
     ## マクロ
     xsd:string,
     ## 名前
     attribute name {
       xsd:string
     }
   }*
 }

=end

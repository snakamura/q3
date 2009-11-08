=begin
=autopilot.xml

((<自動巡回|URL:AutoPilot.html>))の設定をするXMLファイルです。このファイルには、((<自動巡回の設定|URL:OptionAutoPilot.html>))で設定した情報が保存されます。


==書式

===autoPilotエレメント

 <autoPilot>
  <!-- entry -->
 </autoPilot>

autoPilotエレメントがトップレベルエレメントになります。この下に0個以上のentryエレメントをおくことができます。


===entryエレメント

 <entry
  enabled="true|false">
  <!-- course, interval -->
 </entry>

entryエレメントが各巡回のコースとインターバルを指定します。enabled属性にfalseを指定すると無効になります。この下に、courseエレメントとintervalエレメントがひとつずつ置かれます。


===courseエレメント

 <course>
  コース
 </course>

巡回コースを指定します。


===intervalエレメント

 <interval>
  巡回間隔（分単位）
 </interval>

指定したコースを巡回する間隔を分単位で指定します。


==サンプル

 <?xml version="1.0" encoding="utf-8"?>
 <autoPilot>
  <entry>
   <course>Inbox</course>
   <interval>5</interval>
  </entry>
  <entry>
   <course>RSS</course>
   <interval>15</interval>
  </entry>
 </autoPilot>


==スキーマ

 element autoPilot {
   element entry {
     ## 有効かどうか
     ## 指定されない場合には有効
     attribute enabled {
       xsd:boolean
     },
     ## コース
     element course {
       xsd:string
     },
     ## 間隔（分単位）
     element interval {
       xsd:int
     }
   }*
 }

=end

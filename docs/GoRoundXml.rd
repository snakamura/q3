=begin
=goround.xml

((<巡回|URL:GoRound.html>))の設定をするXMLファイルです。このファイルには、((<巡回の設定|URL:OptionGoRound.html>))で設定した情報が保存されます。


==書式

===goroundエレメント

 <goround>
  <!-- course -->
 </goround>

goroundエレメントがトップレベルエレメントになります。goroundエレメント以下には、0個以上のcourseエレメントを置くことが出来ます。


===courseエレメント

 <course
  name="コース名"
  confirm="確認するかどうか">
  <!-- dialup, sequential, parallel -->
 </course>

courseエレメントは一つの巡回コースを表します。name属性でコース名を指定します。confirm属性がtrueだとそのコースで巡回する前に確認のダイアログが表示されます。

courseエレメントの子エレメントとして、dialupエレメントおよび、sequential, parallelエレメントのどちらかを置くことが出来ます。


===dialupエレメント

 <dialup
  name="ダイアルアップエントリ名"
  dialFrom="発信元"
  showDialog="ダイアルアップダイアログを表示するかどうか"
  disconnectWait="切断前に待機する秒数"
  wheneverNotConnected="ネットワーク接続がないときにのみ接続するか"/>

dialupエレメントはcourseエレメントの最初の子エレメントとしておくことが出来ます。このエレメントがない場合にはダイアルアップは行われません。

name属性でダイアルアップ接続のエントリ名を指定します。指定しない場合には実行時にエントリを選択するダイアログが表示されます。dialFromで発信元を指定することが出来ます。指定しない場合には、現在の発信元を使用します。

showDialog 属性にtrueを指定すると、ダイアル前にダイアログを表示します。disconnectWait属性に数字を指定すると、切断前に指定した秒数待機し、その間に切断をキャンセルすることが出来ます。wheneverNotConnected属性にtrueを指定するとネットワーク接続が存在しない場合にのみダイアルアップするようになります。例えば、LANに接続している場合にはダイアルアップしません。


===sequentialエレメント

 <sequential>
  <!-- entry -->
 </sequential>

sequentialエレメントを使用すると、子エレメントに指定したエントリを順番に実行します。


===parallelエレメント

 <parallel>
  <!-- entry -->
 </parallel>

parallelエレメントを使用すると、子エレメントに指定したエントリを同時に実行します。例えば、高速な接続を使用している場合に、parallelエレメントを使用して複数のアカウントを同時に同期することが出来ます。


===entryエレメント

 <entry
  account="アカウント名"
  subaccount="サブアカウント名"
  send="送信するかどうか"
  receive="受信するかどうか"
  applyRules="振り分けるかどうか"
  folder="フォルダ名"
  selectFolder="フォルダを選択するかどうか"
  filter="受信フィルタ"/>

entryエレメントでエントリを指定します。account属性でアカウント名を指定します。subaccount属性を指定するとサブアカウントを指定できます。指定されていない場合には、現在のサブアカウントが使用されます。

send属性とreceive属性にtrueを指定すると、送信・受信のどちらかのみを行うことが出来ます。applyRules属性にtrueを指定すると、振り分けを行います。いずれも指定しない場合には、送受信を行います。

folder属性でフォルダ名を指定すると、そのフォルダのみを同期します。//で囲むことにより正規表現が使用できます。また、selectFolder属性をtrueにすると、同期する前にどのフォルダを同期するのかを尋ねるダイアログが表示されます。指定しないと同期可能なすべてのフォルダを同期します。

filter属性で使用する((<同期フィルタ|URL:SyncFilter.html>))を指定することが出来ます。指定しない場合には、同期フィルタは使用されません。


==サンプル

 <?xml version="1.0" encoding="utf-8"?>
 <goround>
  <course name="all">
   <parallel>
    <entry account="test1"/>
    <entry account="test2" folder="Inbox" receive="true" filter="main"/>
   </parallel>
  </course>
  <course name="ml">
   <parallel>
    <entry account="test3" folder="/ml/.*/" receive="true"/>
   </parallel>
  </course>
 </goround>


==スキーマ

 start = element goround {
   element course {
     element dialup {
       ## ダイアルアップ名
       attribute name {
         xsd:string
       }?,
       ## 発信元
       attribute dialFrom {
         xsd:string
       }?,
       ## ダイアログを表示するかどうか
       attribute showDialog {
         xsd:boolean
       }?,
       ## 切断するまでの待ち時間
       attribute disconnectWait {
         xsd:int
       }?,
       ## ネットワークが接続されていないときのみダイアルアップするかどうか
       attribute wheneverNotConnected {
         xsd:boolean
       }
     }?,
     method,
     ## コース名
     attribute name {
       xsd:string
     },
     ## 巡回する前に確認するかどうか
     attribute confirm {
       xsd:boolean
     }?
   }*
 }
 
 method = element sequential {
   entry*
 } |
 element parallel {
   entry*
 }
 
 entry = element entry {
   empty,
   ## アカウント名
   attribute account {
     xsd:string
   },
   ## サブアカウント名
   attribute subaccount {
     xsd:string
   }?,
   ## 送信するかどうか
   attribute send {
     xsd:boolean
   }?,
   ## 受信するかどうか
   attribute receive {
     xsd:boolean
   }?,
   ## 振り分けるかどうか
   attribute applyRules {
     xsd:boolean
   }?,
   (
     ## 受信するフォルダ
     attribute folder {
       xsd:string
     } |
     ## 受信するフォルダを選択するかどうか
     attribute selectFolder {
       xsd:boolean
     }
   )?,
   ## 同期フィルタ
   attribute filter {
     xsd:string
   }?
 }

=end

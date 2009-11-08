=begin
=rules.xml

((<振り分け|URL:ApplyRules.html>))の設定をするXMLファイルです。このファイルには、((<振り分けの設定|URL:OptionRules.html>))で設定した情報が保存されます。


==書式

===rulesエレメント

 <rules>
   <!-- ruleSet -->
 </rules>

rulesエレメントはトップレベルエレメントになります。rulesエレメント以下には、0個以上のruleSetエレメントを置くことが出来ます。


===ruleSetエレメント

 <ruleSet
  account="アカウント名"
  folder="フォルダ名">
  <!-- rule -->
 </ruleSet>

ruleSetエレメントは振り分け時に使用されるルールセットを指定します。account属性にアカウント名を、folder属性にフォルダ名を指定することが出来ます。両方とも//で囲むことにより正規表現が使用できます。これらの属性は省略可能で、省略された場合にはそれぞれ全てのアカウント・フォルダにマッチします。たとえば、account属性のみを指定してfolder属性を指定しないと、指定したアカウントの全てのフォルダにマッチします。

振り分け時には、振り分け対象のフォルダ名とアカウント名を使用して、上から順番にルールセットを検索し、最初にマッチしたルールセットを使用します。


===ruleエレメント

 <rule
  match="マクロ"
  use="manual|auto|active"
  continue="true|false"
  description="説明",
  enabled="true|false">
  <!-- move/copy/delete/label/deleteCache/apply -->
 </rule>

ruleエレメントは、ルール自体を指定します。match属性にはマクロを指定します。ルールは上から順番にに評価されされます。結果が最初に真になったルールのアクションを実行します。実行されるアクションは子エレメントとして指定します。

use属性にはmanual, auto, activeの組み合わせを指定します。manualを指定すると手動振り分け時に、autoを指定すると自動振り分け時に、activeを指定するとアクティブ振り分け字に使用されます。複数指定する場合には空白文字で区切ります。指定しない場合には"manual auto"を指定したのと同じになります。

continue属性にはこのルールにマッチして処理した後で後続のルールを処理するかどうかを指定します。

description属性には説明を指定します。

enabled属性にfalseを指定するとルールが無効になります。


===moveエレメント

 <move
  account="アカウント名"
  folder="フォルダ名">
  <!-- template -->
 </move>

moveエレメントは、移動コマンドを指定します。このアクションはaccount属性とfolder属性で指定されたフォルダにメッセージを移動します。accountを属性は省略可能で、その場合振り分け対象のフォルダと同じアカウントのフォルダが移動先になります。

子エレメントとしてtemplateエレメントを置くと、テンプレートで処理してから移動することになります。


===copyエレメント

 <copy
  account="アカウント名"
  folder="フォルダ名">
  <!-- template -->
 </copy>

copyエレメントは、メッセージを移動せずにコピーする点を除いてmoveエレメントと同じです。


===templateエレメント

 <template
  name="テンプレート名">
  <!-- argument -->
 </template>

templateエレメントをcopyやmoveの子エレメントとして置くと、移動・コピー時にメッセージをテンプレートで処理します。name属性でテンプレート名を指定します。テンプレートに引数を渡すときには子エレメントとして任意の数のargumentエレメントをおきます。


===argumentエレメント

 <argument
  name="引数名">
  値
 </argument>

argumentエレメントはtemplateエレメントで指定したテンプレートに引数を渡すために指定します。name属性で名前を、テキストとして値を指定します。テンプレートに渡した引数は、テンプレート中からマクロの変数としてアクセスできます。


===deleteエレメント

 <delete
  direct="ゴミ箱を使用するかどうか"/>

deleteエレメントは、削除コマンドを指定します。このアクションは対象のメッセージを削除します。direct属性にtrueを指定するとゴミ箱を使用せずに直接メッセージを削除します。それ以外の場合にはゴミ箱にメッセージを移動します。


===labelエレメント

 <label
  type="add|remove">
  ラベル
 </label>

labelエレメントは、ラベル設定コマンドを指定します。type属性が指定されなかった場合には指定されたラベルを設定します。type属性にaddを指定すると指定されたラベルを追加し、removeを指定すると削除します。


===deleteCacheエレメント

 <deleteCache/>

deleteCacheエレメントは、キャッシュ削除コマンドを指定します。このアクションはメッセージのキャッシュ（IMAP4やNNTPアカウントでサーバから取得したメッセージ）を削除します。


===applyエレメント

 <apply>
  実行するマクロ
 </apply>

applyエレメントはマクロの実行を指定します。対象のメッセージに対して指定されたマクロを実行します。


==サンプル

 <?xml version="1.0" encoding="utf-8"?>
 <rules>
  <ruleSet account="test" folder="Inbox">
   <rule match="@BeginWith(%Subject, '[Qs:')">
    <move folder="ml/qs"/>
   </rule>
   <rule match="@Contain(From, '@example.com')">
    <move folder="test"/>
   </rule>
  </ruleSet>
  <ruleSet account="test">
   <rule match="@True()">
    <copy folder="archives">
     <template name="strip_header">
      <argument name="foo">bar</argument>
     </template>
    </copy>
   </rule>
  </ruleSet>
 </rules>


==スキーマ

 start = element rules {
   element ruleSet {
     element rule {
       ## アクション
       action?,
       ## ルールが適用される条件（マクロ）
       attribute match {
         xsd:string
       },
       # ルールが適用される場面
       # manualは手動振り分け
       # autoは自動振り分け
       # activeはアクティブ振り分け
       # 指定されない場合には手動と自動
       attribute use {
         xsd:string
       }?
       ## 次のルールに進むかどうか
       ## 指定されない場合にはfalse
       attribute continue {
         xsd:boolean
       }?,
       ## 説明
       attribute description {
         xsd:string
       }?,
       ## 有効かどうか
       ## 指定されない場合にはtrue
       attribute enabled {
         xsd:boolean
       }?
     }*,
     ## ルールが適用されるアカウント
     ## 指定されない場合、全てのアカウント
     attribute account {
       xsd:string
     }?,
     ## ルールが適用されるフォルダ
     ## 指定されない場合、全てのフォルダ
     attribute folder {
       xsd:string
     }?
   }*
 }
 
 action = element move {
   template?,
   ## 移動先アカウント
   attribute account {
     xsd:string
   }?,
   ## 移動先フォルダ
   attribute folder {
     xsd:string
   }
 } |
 element copy {
   template?,
   ## コピー先アカウント
   attribute account {
     xsd:string
   }?,
   ## コピー先フォルダ
   attribute folder {
     xsd:string
   }
 } |
 element delete {
   attribute direct {
     xsd:boolean
   }?
 } |
 element label {
   ## タイプ
   ## 指定しない場合には設定
   ## addは追加
   ## removeは削除
   attribute type {
     "add|remove"
   }?,
   ## ラベル
   xsd:string
 } |
 element deleteCache {
   empty
 } |
 element apply {
   ## 実行するマクロ
   xsd:string
 }
 
 template = element template {
   ## 引数
   element argument {
     ## 引数の値
     text,
     ## 引数の名前
     attribute name {
       xsd:string
     }
   }*,
   ## テンプレートの名前
   attribute name {
     xsd:string
   }
 }

=end

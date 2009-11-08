=begin
=syncfilters.xml

((<同期フィルタ|URL:SyncFilter.html>))の設定をするXMLファイルです。このファイルには、((<同期フィルタの設定|URL:OptionSyncFilters.html>))で設定した情報が保存されます。


==書式

===filtersエレメント

 <filters>
  <!-- filterSet -->
 </filters>

filtersエレメントがトップレベルエレメントになります。この下に0個以上のfilterSetを置くことが出来ます。


===filterSetエレメント

 <filterSet
  name="名前">
  <!-- filter -->
 </filterSet>

filterSetエレメントはフィルタのセットを指定します。name属性でフィルタセットの名前を指定します。


===filterエレメント

 <filter
  folder="フォルダ名"
  match="マクロ">
  <!-- action -->
 </filter>

filterエレメントでフィルタを指定します。match属性にマクロを指定します。このマクロを評価した結果が真になるフィルタのアクションが実行されます。folder属性を指定すると、指定したフォルダを同期するときにのみ使用されます。//で囲むことにより正規表現が使用できます。

filterエレメントは上から順番に評価され、最初にマクロが真になったフィルタが使用されます。

一つのfilterエレメントの下には一つ以上のactionエレメントを置くことが出来ます。アクションエレメントは上から順番に評価され、実行されます。


===actionエレメント

 <action
  name="名前">
  <!-- param -->
 </action>

actionエレメントはフィルタがどのような動作をするのかを指定します。name属性でアクションの名前を指定します。どのようなアクションがあるのかは備考を参照してください。


===paramエレメント

 <param
  name="名前">
  値
 </param>

paramエレメントはアクションのパラメータを指定します。name属性でパラメータの名前を指定し、子ノードとして値を指定します。アクションにどのようなパラメータがあるのかは備考を参照してください。


==サンプル

 <?xml version="1.0" encoding="utf-8"?>
 <filters>
  <filterSet name="test_main">
   <filter folder="Inbox" match="@Greater(@Size(),10240)">
    <action name="download">
     <param name="type">header</param>
    </action>
   </filter>
   <filter folder="Inbox" match="@And(@Or(
    @Contain(@Address(To),'foo.com'),
    @Contain(@Address(From),'foo.com')))">
    <action name="download">
     <param name="type">text</param>
    </action>
   </filter>
   <filter folder="Inbox" match="@And(@Or(
    @Contain(@Address(To),'foo.com'),
    @Contain(@Address(From),'foo.com')),
    @Less(@Size(@True()),10240))">
    <action name="download">
     <param name="type">text</param>
    </action>
   </filter>
   <filter folder="Inbox" match="@True()">
    <action name="download">
     <param name="type">header</param>
    </action>
   </filter>
  </filterSet>
  <filterSet name="test2_main">
   <filter match="Greater(@Size(),10240)">
    <action name="download">
     <param name="line">1000</param>
    </action>
   </filter>
  </filterSet>
 </filters>


==スキーマ

 element filters {
   element filterSet {
     element filter {
       element action {
         element param {
           ## パラメータの値
           xsd:string,
           ## パラメータの名前
           attribute name {
             xsd:string
           }
         }*,
         ## アクションの名前
         attribute name {
           xsd:string
         }
       }+,
       ## フィルタが適用されるフォルダ
       ## 指定されない場合、全てのフォルダ
       attribute folder {
         xsd:string
       }?,
       ## フィルタがマッチする条件（マクロ）
       attribute match {
         xsd:string
       }
     }*,
     ## フィルタセットの名前
     attribute name {
       xsd:string
     }
   }*
 }


==備考
現在指定できるアクションは以下のとおりです。アクションはプロトコルごとに異なります。

===POP3

====downloadアクション
メッセージをダウンロードします。lineパラメータに最大行数を指定します。


====deleteアクション
メッセージをサーバ上から削除します。


====ignoreアクション
メッセージをダウンロードしません。ignoreアクションを指定するとリストにも表示されなくなります。


===IMAP4

====downloadアクション
メッセージをダウンロードします。typeパラメータでタイプを指定します。

指定できるタイプは以下のとおりです。

:all
  全て
:text
  テキストとして表示するのに必要な部分のみ
:html
  HTMLメールとして表示するのに必要な部分のみ
:header
  ヘッダのみ


====deleteアクション
メッセージをサーバ上から削除します（削除フラグを立てます）。


===NNTP

====downloadアクション
メッセージをダウンロードします。


====ignoreアクション
メッセージをダウンロードしません。ignoreアクションを指定するとリストにも表示されなくなります。

=end

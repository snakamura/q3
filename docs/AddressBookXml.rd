=begin
=addressbook.xml

((<アドレス帳|URL:AddressBook.html>))の情報を保存する設定をするXMLファイルです。


==書式

===addressBookエレメント

 <addressBook>
  <!-- entry -->
 </addressBook>

addressBookエレメントがトップレベルエレメントになります。このエレメント以下には、0個以上のentryエレメントを置くことが出来ます。


===entryエレメント

 <entry>
  <!-- name, sortKey, addresses -->
 </entry>

entryエレメントは一つのエントリを表します。通常エントリはひとりの人に対応します。エントリには複数のアドレスを含めることが出来ます。


===nameエレメント

 <name>
  <!-- 名前 -->
 </name>

nameエレメントにはエントリの名前を指定します。これは通常そのエントリの人の名前になります。


===sortKeyエレメント

 <sortKey>
  <!-- ソートキー -->
 </sortKey>

sortKeyエレメントにはエントリのソートキーを指定します。これは通常そのエントリのふりがなを指定します。sortKeyを指定しないとnameエレメントの値がソートキーになります。


===addressesエレメント

 <addresses>
  <!-- address ->
 </addresses>

addressesエレメントはメールアドレスのリストを表します。addressesエレメントは、1個以上のaddressエレメントを含みます。


===addressエレメント

 <address
  alias="エイリアス名"
  category="カテゴリ名"
  comment="コメント"
  rfc2822="boolean"
  certificate="証明書の名前">
  <!-- アドレス -->
 </address>

addressエレメントで一つのメールアドレスを指定します。

alias属性でエイリアス名を指定します。ここで指定したエイリアス名をエディットビューのToフィールドなどに指定すると自動的に展開されます。

category属性でカテゴリを指定します。カテゴリは、「/」で区切ることで階層化できます。一つのアドレスに複数のカテゴリを指定する場合には、「,」で区切ります。

comment属性でアドレスにコメントをつけることが出来ます。

rfc2822属性にtrueを指定すると、指定されたメールアドレスは既にRFC2822形式であるとして処理されます。それ以外の場合には、エントリの名前とメールアドレスからRFC2822形式を生成します。デフォルトはfalseです。

certificate属性でS/MIMEで使用する証明書の名前を指定します。証明書は、security/<指定した名前>.pemというファイルからロードされます。


==サンプル

 <?xml version="1.0" encoding="utf-8"?>
 <addressBook>
  <entry>
   <name>Hogehoge Fuga</name>
   <addresses>
    <address alias="hoge" comment="自宅">hoge@foo.com</address>
    <address category="取引先/どこそこ" rfc2822="true"
     >ほげほげ様 &lt;hogefuga@dokosoko.co.jp></address>
   </addresses>
 </addressBook>


==スキーマ

 element addressBook {
   element entry {
     ## 名前
     element name {
       xsd:string
     },
     element sortKey {
       xsd:string
     }?,
     element addresses {
       element address {
         ## アドレス
         xsd:string,
         ## エイリアス
         attribute alias {
           xsd:string
         }?,
         ## カテゴリ（'/'で区切って階層化）
         ## 複数指定する場合には','で区切る
         attribute category {
           xsd:string {
             pattern = "([^/,]+(/[^/,]+)*)+(,([^/,]+(/[^/,]+)*))*"
           }
         }?,
         ## コメント
         attribute comment {
           xsd:string
         }?,
         ## アドレスがRFC2822形式になっているかどうか
         ## trueの場合、アドレスがそのまま使用される
         ## falseの場合、名前とアドレスからRFC2822形式が生成される
         ## 指定されない場合、false
         attribute rfc2822 {
           xsd:boolean
         }?,
         ## 証明書の名前
         attribute certificate {
           xsd:string
         }?
       }+
     }
   }*
 }

=end

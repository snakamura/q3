=begin
=headeredit.xml

ヘッダエディットビューの設定をするXMLファイルです。


==書式

===headerEditエレメント

 <headerEdit>
  <!-- line -->
 </headerEdit>

headerEditエレメントがトップレベルエレメントになります。headerEditエレメント以下には0個以上のlineエレメントを置くことができます。


===lineエレメント

 <line
  hideIfNoFocus="true|false"
  class="クラス名">
  <!-- static, edit, address, attachment, account, signature -->
 </line>

lineエレメントはヘッダエディットビューの一行を表します。

hideIfNoFocus属性は、ヘッダエディットビューからフォーカスがなくなったとき（例えばエディットビューにフォーカスが移動したとき）にその行を隠すかどうかを指定します。指定しない場合にはfalseを指定したのと同じになります。

class属性には正規表現を指定します。指定した正規表現にアカウントクラスがマッチする場合のみ行が表示されます。例えば、"mail|news"と指定するとmailアカウントとnewsアカウントでのみ表示されるようになります。指定しない場合には、アカウントクラスにかかわらず常に表示されます。


===staticエレメント

 <static
  width="幅"
  number="番号"
  initialFocus="true|false"
  style="フォントスタイル"
  align="left|center|right">
  文字列
 </static>

staticエレメントはスタティックコントロールを表します。コンテンツに表示する文字列を指定します。

width属性には幅を指定します。幅の指定については、((<header.xml|URL:HeaderXml.html>))の備考を参照してください。

number属性にはコントロールの番号を指定します。この番号を((<ViewFocusEditItemアクション|URL:ViewFocusEditItemAction.html>))の引数に指定することで、フォーカスを移動することができます。

initialFocus属性にはエディットウィンドウを開いたときにフォーカスを受け取るかどうかを指定します。指定しない場合には、trueを指定したのと同じになります。

style属性にはフォントのスタイルを指定します。指定できるのはboldとitalicの組み合わせです。複数指定する場合には,で区切ります。指定しない場合には通常のスタイルになります。

align属性にはleft, center, rightのいずれかを指定します。それぞれ、左寄せ、中央寄せ、右寄せになります。指定しない場合には左寄せになります。


===editエレメント

 <edit
  width="幅"
  number="番号"
  initialFocus="true|false"
  style="フォントスタイル"
  align="left|center|right"
  field="ヘッダ名"
  type="addressList|references|unstructured"/>

editエレメントはエディットコントロールを表します。

width, number, initialFocus, style, align属性についてはstaticエレメントを参照してください。

field属性にはこのエディットコントロールで編集するヘッダ名を指定します。また、type属性にはそのヘッダの型を指定します。指定できるのは、addressList, references, unstructuredのいずれかです。


===addressエレメント

 <address
  width="幅"
  number="番号"
  initialFocus="true|false"
  style="フォントスタイル"
  align="left|center|right"
  field="ヘッダ名"
  expandAlias="true|false"
  autoComplete="true|false"/>

addressエレメントはアドレス用のエディットコントロールを表します。

width, number, initialFocus, style, align属性についてはstaticエレメントを参照してください。field属性についてはeditエレメントを参照してください。

expandAlias属性には、((<アドレス帳|URL:AddressBook.html>))の別名を展開するかどうかを指定します。trueを指定するとフォーカスを失ったときに別名を展開します。指定しない場合にはtrueを指定したのと同じになります。

autoComplete属性には、((<アドレスの自動補完|URL:AddressAutoComplete.html>))を有効にするかどうかを指定します。指定しない場合にはtrueを指定したのと同じになります。


===attachmentエレメント

 <attachment
  width="幅"
  number="番号"
  initialFocus="true|false"/>

attachmentエレメントは添付ファイル編集コントロールを表します。

width, number, initialFocus属性についてはstaticエレメントを参照してください。


===accountエレメント

 <account
  width="幅"
  number="番号"
  initialFocus="true|false"
  showFrom="true|false"/>

accountエレメントは((<アカウント|URL:Account.html>))選択コントロールを表します。選択可能なアカウント、サブアカウントがコンボボックスで表示されます。

width, number, initialFocus属性についてはstaticエレメントを参照してください。

showFrom属性にはアカウント名の隣に使用されるFromアドレスを表示するかどうかを指定します。指定しない場合にはtrueを指定したのと同じになります。


===signatureエレメント

 <signature
  width="幅"
  number="番号"
  initialFocus="true|false"/>

signatureエレメントは((<署名|URL:Signature.html>))選択コントロールを表します。選択可能な署名がコンボボックスで表示されます。

width, number, initialFocus属性についてはstaticエレメントを参照してください。


==サンプル

 <?xml version="1.0" encoding="utf-8"?>
 <headerEdit>
  <line class="mail">
   <static width="1max" style="bold" align="right">T&amp;o:</static>
   <address field="To" number="0"/>
  </line>
  <line class="mail" hideIfNoFocus="true">
   <static width="1max" style="bold" align="right">&amp;Cc:</static>
   <address field="Cc" number="1" initialFocus="false"/>
  </line>
  <line class="mail" hideIfNoFocus="true">
   <static width="1max" style="bold" align="right">&amp;Bcc:</static>
   <address field="Bcc" number="2" initialFocus="false"/>
  </line>
  <line class="news">
   <static width="1max" style="bold" align="right">&amp;Newsgroups:</static>
   <edit type="unstructured" field="Newsgroups" number="3"/>
  </line>
  <line class="news" hideIfNoFocus="true">
   <static width="1max" style="bold" align="right">Fo&amp;llowup-To:</static>
   <edit type="unstructured" field="Followup-To" number="4" initialFocus="false"/>
  </line>
  <line hideIfNoFocus="true">
   <static width="1max" style="bold" align="right">&amp;Subject:</static>
   <edit type="unstructured" field="Subject" number="5"/>
  </line>
  <line hideIfNoFocus="true">
   <static width="1max" style="bold" align="right">Attac&amp;hment:</static>
   <attachment number="6"/>
  </line>
  <line hideIfNoFocus="true">
   <static width="1max" style="bold" align="right">Acco&amp;unt:</static>
   <account number="7"/>
   <static width="auto" style="bold" align="right">S&amp;ignature:</static>
   <signature width="5em" number="8"/>
  </line>
 </headerEdit>


==スキーマ

 start = element headerEdit {
   element line {
     (
       element static {
         textitem
       } |
       element edit {
         textitem
       } |
       element address {
         textitem,
         ## エイリアスを展開するかどうか
         ## 指定されない場合、true
         attribute expandAlias {
           xsd:boolean
         }?,
         ## オートコンプリートを使用するかどうか
         ## 指定されない場合、true
         attribute autoComplete {
           xsd:boolean
         }?
       } |
       element attachment {
         item
       } |
       element account {
         item,
         ## Fromのアドレスを表示するかどうか
         ## 指定されない場合、true
         attribute showFrom {
           xsd:boolean
         }?
       } |
       element signature {
         item
       }
     )*,
     ## フォーカスがない場合に隠すかどうか
     ## 指定されない場合、false
     attribute hideIfNoFocus {
       xsd:boolean
     }?,
     ## どのアカウントクラスで表示するか
     ## 指定されない場合にはすべてのクラスで表示
     attribute class {
       xsd:string
     }?
   }*
 }
 
 item = attribute width {
   xsd:string {
     pattern = "auto|[0-9]max|[0-9]min|[0-9]+(px)?|[0-9]+%|[0-9]+(\.[0-9]+)?em"
   }
 }?,
 attribute number {
   xsd:int
 }?,
 attribute initialFocus {
   xsd:boolean
 }?
 
 textitem = item,
 ## スタイル
 ## boldとitalicが指定可能
 ## 複数指定する場合には,で区切る
 attribute style {
   xsd:string
 }?,
 attribute align {
   "left" | "center" | "right"
 }?,
 (
   xsd:string |
   (
     ## ヘッダ名
     attribute field {
       xsd:string
     },
     ## ヘッダのタイプ
     attribute type {
       "addressList" | "references" | "unstructured"
     }?
   )
 )

=end

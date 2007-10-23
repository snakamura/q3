=begin
=header.xml

ヘッダビューの設定をするXMLファイルです。


==書式

===headerエレメント

 <header>
  <!-- line -->
 </header>

headerエレメントがトップレベルエレメントになります。headerエレメント以下には0個以上のlineエレメントを置くことができます。


===lineエレメント

 <line
  hideIfEmpty="アイテム名"
  class="クラス名">
  <!-- static, edit, attachment -->
 </line>

lineエレメントはヘッダビューの一行を表します。

hideIfEmpty属性にはアイテム名を指定します。指定したアイテムが空の場合には行ごと隠されます。指定しない場合には、その行は常に表示されます。

class属性には正規表現を指定します。指定した正規表現にアカウントクラスがマッチする場合のみ行が表示されます。例えば、"mail|news"と指定するとmailアカウントとnewsアカウントでのみ表示されるようになります。指定しない場合には、アカウントクラスにかかわらず常に表示されます。


===staticエレメント

 <static
  name="アイテム名"
  width="幅"
  number="番号"
  showAlways="true|false"
  background="背景色"
  style="フォントスタイル"
  align="left|center|right">
  テンプレート
 </static>

staticエレメントはスタティックコントロールを表します。コンテンツにテンプレート書式で表示する文字列を指定します。

name属性にはアイテム名を指定します。lineエレメントのhideIfEmpty属性に指定する場合にはここで名前を指定しておきます。

width属性には幅を指定します。幅の指定については備考を参照してください。

number属性にはコントロールの番号を指定します。この番号を((<ViewFocusItemアクション|URL:ViewFocusItemAction.html>))の引数に指定することで、フォーカスを移動することができます。

showAlways属性にはtrueまたはfalseを指定します。trueを指定するとコンテキストアカウントがない場合でも常にアイテムに文字列を表示します。この場合、テンプレート中にマクロを書くことはできません。指定しない場合にはfalseを指定した場合と同じになります。

background属性には背景色をテンプレート書式で指定します。テンプレートを評価した結果は、rrggbb形式の文字列になる必要があります。空文字列を返すとヘッダビューの背景色と同じになります。指定しない場合にはヘッダビューの背景色と同じになります。

style属性にはフォントのスタイルを指定します。指定できるのはboldとitalicの組み合わせです。複数指定する場合には,で区切ります。指定しない場合には通常のスタイルになります。

align属性にはleft, center, rightのいずれかを指定します。それぞれ、左寄せ、中央寄せ、右寄せになります。指定しない場合には左寄せになります。


===editエレメント

 <edit
  name="アイテム名"
  width="幅"
  number="番号"
  showAlways="true|false"
  background="背景色"
  style="スタイル"
  align="left|center|right"
  multiline="行数"
  wrap="true|false">
  テンプレート
 </edit>

editエレメントはエディットコントロールを表します。ただし編集できるわけではなく、スタティックコントロールとの主な違いは、フォーカスを持てるかどうかと自動スクロールするかどうかです。コンテンツにテンプレート書式で表示する文字列を指定します。

name, width, number, showAlways, background, style, align属性についてはstaticエレメントを参照してください。

multiline属性には複数行になったときに最大何行まで大きくするかを指定します。-1を指定すると複数行にはなりません。また、0を指定すると必要なだけ行数が増えます。指定しない場合には複数行になりません。

wrap属性にはコントロールの幅で自動で折り返すかどうかを指定します。指定しない場合には折り返しません。


===attachmentエレメント

 <attachment
  name="アイテム名"
  width="幅"
  number="番号"
  showAlways="true|false"
  background="背景色"/>

attachmentエレメントは添付ファイルコントロールを表します。メッセージの添付ファイルを表示します。

name, width, number, showAlways, background属性についてはstaticエレメントを参照してください。


==サンプル

 <?xml version="1.0" encoding="utf-8"?>
 <header>
  <line class="mail">
   <static width="auto" style="bold" showAlways="true">To:</static>
   <edit>{@FormatAddress(To, 3)}</edit>
  </line>
  <line class="mail" hideIfEmpty="cc">
   <static width="auto" style="bold" showAlways="true">Cc:</static>
   <edit name="cc">{@FormatAddress(Cc, 3)}</edit>
  </line>
  <line class="news">
   <static width="auto" style="bold" showAlways="true">Newsgroups:</static>
   <edit>{Newsgroups}</edit>
  </line>
  <line class="news" hideIfEmpty="followup-to">
   <static width="auto" style="bold" showAlways="true">Followup-To:</static>
   <edit name="followup-to">{FollowUp-To}</edit>
  </line>
  <line class="mail|news">
   <static width="auto" style="bold" showAlways="true">From:</static>
   <edit background="{@If(@Not(@Param('Verify')), '', @Contain(@Param('Verify'), 'AddressMatch'), 'f5f6be', @Contain(@Param('Verify'), 'AddressMismatch'), 'ec7b95', '')}">{@FormatAddress(From, 3)}</edit>
   <static width="auto" style="bold" align="right" showAlways="true">Date:</static>
   <edit width="10em">{@FormatDate(@Date(Date), @Profile('', 'Global', 'DefaultTimeFormat'))}</edit>
  </line>
  <line class="mail|news">
   <static width="auto" style="bold" showAlways="true">Subject:</static>
   <edit>{@Subject()}</edit>
  </line>
  <line hideIfEmpty="attachment" class="mail|news">
   <static width="auto" style="bold" showAlways="true">Attachment:</static>
   <attachment name="attachment" background="{@If(@Equal(X-QMAIL-AttachmentDeleted, 1), 'ccc7ba', '')}"/>
  </line>
  <line class="rss">
   <static width="auto" style="bold" showAlways="true">Title:</static>
   <edit>{@Subject()}</edit>
  </line>
  <line class="rss">
   <static width="auto" style="bold" showAlways="true">Date:</static>
   <edit>{@FormatDate(@Date(Date), @Profile('', 'Global', 'DefaultTimeFormat'))}</edit>
  </line>
  <line class="rss" hideIfEmpty="creator">
   <static width="auto" style="bold" showAlways="true">Creator:</static>
   <edit name="creator">{X-RSS-Creator}</edit>
  </line>
  <line class="rss" hideIfEmpty="category">
   <static width="auto" style="bold" showAlways="true">Category:</static>
   <edit name="category">{X-RSS-Category}</edit>
  </line>
  <line class="rss" hideIfEmpty="subject">
   <static width="auto" style="bold" showAlways="true">Subject:</static>
   <edit name="subject">{X-RSS-Subject}</edit>
  </line>
  <line class="rss">
   <static width="auto" style="bold" showAlways="true">URL:</static>
   <edit>{X-RSS-Link}</edit>
  </line>
  <line hideIfEmpty="label">
   <static width="auto" style="bold" showAlways="true">Label:</static>
   <edit name="label">{@Label()}</edit>
  </line>
  <line class="mail|news" hideIfEmpty="sign">
   <static width="auto" style="bold" showAlways="true">Signed by:</static>
   <edit name="sign" background="{@If(@Not(@Param('Verify')), '', @Contain(@Param('Verify'), 'Verified'), 'f5f6be', @Contain(@Param('Verify'), 'VerifyFailed'), 'ec7b95', '')}">{@Param('SignedBy')}</edit>
  </line>
 </header>


==スキーマ

 start = element header {
   element line {
     (
       element static {
         textitem
       } |
       element edit {
         textitem,
         attribute multiline {
           xsd:int
         }?,
         attribute wrap {
           xsd:boolean
         }?
       } |
       element attachment {
         item
       }
     )*,
     attribute hideIfEmpty {
       xsd:string
     }?,
     attribute class {
       xsd:string
     }?
   }*
 }
 
 item = attribute name {
   xsd:string
 }?,
 attribute width {
   xsd:string {
     pattern = "auto|[0-9]max|[0-9]min|[0-9]+(px)?|[0-9]+%|[0-9]+(\.[0-9]+)?em"
   }
 }?,
 attribute showAlways {
   xsd:boolean
 }?,
 attribute background {
   xsd:string
 }?
 
 textitem = xsd:string,
 item,
 attribute style {
   xsd:string
 }?,
 attribute align {
   "left" | "center" | "right"
 }?


==備考

width属性での幅の指定では以下のような指定ができます。

:指定なし
  幅を指定しません。
:auto
  必要に応じて幅を決めます。
:max
  1maxから9maxまで指定でき、同じ番号をつけられたコントロールで最大のコントロールの幅に合わされます。コントロールの幅はautoを指定したのと同様に決められます。
:min
  1minから9minまで指定でき、同じ番号をつけられたコントロールで最小のコントロールの幅に合わされます。コントロールの幅はautoを指定したのと同様に決められます。
:px
  ピクセル単位で指定します。整数のみ指定できます。
:%
  パーセントで指定します。整数のみ指定できます。
:em
  フォントの高さを基準に指定します。小数を指定できます。

maxとminは別の行にあるアイテムの幅を揃えたい場合に使用します。例えば、各行の左側にラベルを置いてラベルの幅を合わせたい場合に、それらすべてのラベルのwidth属性に1maxと指定しておくと、最も幅の広いラベルの幅にすべてのラベルの幅が合わされます。

行内の配置を決めるときには以下のように決めます。

(1)auto, max, minが指定されたコントロールの幅を計算し、各コントロールが必要とする幅を決める
(2)px, emが指定されたコントロールの幅を決める
(3)(1), (2)で決まったコントロールの幅をすべて足す
(4)(3)で計算した幅がヘッダビューの幅よりも狭い場合には、残りの幅を%で指定されたコントロールに割り振る
(5)%で指定されたコントロールの幅の合計が100%を超えていない場合には、残りの幅を幅を指定しなかったコントロールで均等に割る
(6)%で指定されたコントロールの幅の合計が100%を超えていた場合には、幅を指定しなかったコントロールの幅を0にする
(7)(3)で計算した幅がヘッダビューの幅よりも広い場合には、%で指定されたコントロールと幅を指定しなかったコントロールの幅を0にする

=end

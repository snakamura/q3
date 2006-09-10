=begin
=qmail.xml

QMAIL3全体に関する設定を保存するXMLファイルです。このファイルで設定できる多くの項目は((<オプションの設定|URL:Options.html>))などで設定できますが、一部の項目は直接このファイルを編集しないと設定できません。設定できる項目の一覧は備考を参照してください。


==書式

===profileエレメント

 <profile>
  <!-- section -->
 </profile>

profileエレメントがトップレベルエレメントになります。profileエレメント以下には0個以上のsectionエレメントを置くことができます。


===sectionエレメント

 <section
  name="名前">
  <!-- key -->
 </filter>

sectionエレメントは一つのセクションを表します。name属性にはセクションの名前を指定します。


===keyエレメント

 <key
  name="名前">
  値
 </key>

keyエレメントは一つのキーを表します。name属性にはキーの名前を指定します。子ノードとしてそのキーの値を指定します。


==サンプル

 <?xml version="1.0" encoding="utf-8"?>
 <profile>
  <section name="AddressBookFrameWindow">
   <key name="Height">552</key>
   <key name="Left">856</key>
   <key name="Top">97</key>
   <key name="Width">611</key>
  </section>
  <section name="EditFrameWindow">
   <key name="Height">565</key>
   <key name="Left">175</key>
   <key name="Top">558</key>
   <key name="Width">627</key>
  </section>
  <section name="Find">
   <key name="History0">Test</key>
  </section>
  <section name="FolderWindow">
   <key name="ExpandedFolders">//Main //News //RSS //Sub //Sub/Inbox</key>
  </section>
  <section name="Global">
   <key name="CurrentFolder">//Main/テスト</key>
   <key name="DetachFolder">C:\Temp</key>
   <key name="LastUpdateCheck">2006-08-13T20:40:26+09:00</key>
   <key name="Offline">0</key>
  </section>
  <section name="HeaderEditWindow">
   <key name="ImeFollowup-To">0</key>
   <key name="ImeNewsgroups">0</key>
  </section>
  <section name="MainWindow">
   <key name="Height">849</key>
   <key name="Left">159</key>
   <key name="PrimaryLocation">187</key>
   <key name="Top">79</key>
   <key name="Width">764</key>
  </section>
  <section name="MessageFrameWindow">
   <key name="Height">711</key>
   <key name="Left">682</key>
   <key name="Top">426</key>
   <key name="Width">811</key>
  </section>
  <section name="OptionDialog">
   <key name="Panel">20</key>
  </section>
  <section name="RecentAddress">
   <key name="Address0">Taro Yamada &lt;taro@example.org></key>
  </section>
  <section name="SyncDialog">
   <key name="Left">609</key>
   <key name="Top">845</key>
  </section>
 </profile>


==スキーマ

 element profile {
   element section {
     element key {
       ## 値
       xsd:string,
       ## キーの名前
       attribute name {
         xsd:string
       }
     }*,
     ## セクションの名前
     attribute name {
       xsd:string
     }
   }*
 }


==備考
このファイルではセクションとキーで値を指定します。例えば、上の例ではGlobalセクションのDetachFolderキーにC:\Tempという値が指定されています。このドキュメント中ではこれをGlobal/DetachFolderのように記述してあることがあります。例えば、「記憶するアドレスの個数は((<qmail.xml|URL:QmailXml.html>))のRecentAddress/Maxで指定できます。」のように書かれていたら、RecentAddressセクションのMaxキーで指定するということになります。

それぞれのキーはデフォルトの値を持っていて、指定されていない場合にはその値が使用されます。また、値がデフォルトの値と同じ場合にはファイルには書き出されません。存在しないキーの値を指定する場合には、新しくセクションやキーを追加してください。

指定できるセクションとキーは以下の通りです。


===AddressBookセクション

+AddressOnly (0 @ 0|1)
外部アドレス帳からアドレスを取り込むときに名前を取り込まずメールアドレスだけを取り込むかどうか。


+Category
アドレス選択ダイアログで現在選択されているカテゴリ。


+Externals ( @ WAB, Outlook, PocketOutlook)
取り込む外部アドレス帳。


+AddressWidth (130), NameWidth (120), CommentWidth (60), SelectedAddressWidth (150)
アドレス選択ダイアログのアドレス欄、名前欄、コメント欄、選択されたアドレス欄の幅。


+Width (620), Height (450)
アドレス選択ダイアログの大きさ。


===AddressBookFrameWindowセクション

+Left (0), Top (0), Width (0), Height (0), Show (1), Alpha (0 @ 0-255)
アドレス帳ウィンドウの位置と大きさ、表示方法と透過度。


+ShowToolbar (1 @ 0|1), ShowStatusBar (1 @ 0|1)
アドレス帳ウィンドウのツールバーとステータスバーを表示するかどうか。


===AddressBookListWindowセクション

+AddressWidth (150), NameWidth (150), CommentWidth (150)
アドレスビューのアドレス欄、名前欄、コメント欄の幅。


+FontFace, FontSize (9), FontStyle (0), FontCharset (0)
アドレスビューのフォント。フォントサイズはポイント。


===AutoPilotセクション

+Enabled (0 @ 0|1)
自動巡回が有効かどうか


+OnlyWhenConnected
ネットワーク接続されているときのみ自動巡回するかどうか。


===ColorsDialogセクション

+Width (620), Height (450)
色の設定ダイアログの大きさ。


===Dialupセクション

+Entry
最後に指定したダイアルアップのエントリ名。


===EditFrameWindow

+Left (0), Top (0), Width (0), Height (0), Show (1), Alpha (0 @ 0-255)
エディットウィンドウの位置と大きさ、表示方法と透過度。


+ShowToolbar (1 @ 0|1), ShowStatusBar (1 @ 0|1)
エディットウィンドウのツールバーとステータスバーを表示するかどうか。


===EditWindow

+FontFace, FontSize (9), FontStyle (0), FontCharset (0)
エディットビューのフォント。フォントサイズはポイント。


+AdjustExtent (0 @ 0|1)
文字幅を調節するかどうか。


+UseSystemColor (1 @ 0|1)
システムの配色を使うかどうか。


+ForegroundColor (000000), BackgroundColor (ffffff), LinkColor (0000ff), QuoteColor1 (008000), QuoteColor2 (000080)
文字色、背景色、リンクの色、引用の色1, 2。形式はRRGGBB。


+CharInLine (0)
一行の文字数。0の場合にはウィンドウの折り返し位置。指定した値×xの文字幅の位置で折り返される。


+ClickableURL (1 @ 0|1)
クリッカブルURLが有効かどうか。


+DragScrollDelay (300), DragScrollInterval (300)
ドラッグで選択中にスクロールするときの遅延と間隔。単位はミリ秒。


+LineQuote (0 @ 0|1)
引用を線で表示するかどうか。


+LineSpacing (2)
行間の高さ。単位はピクセル。


+MarginLeft (10), MarginTop (10), MarginRight (10), MarginBottom (10)
マージン。単位はピクセル。


+Quote1 (>), Quote2 (#)
引用文字1, 2。候補を文字列で指定。

Quote1で指定した文字のいずれかから始まる行がQuoteColor1で指定した色に、Quote2で指定した文字のいずれかから始まる行がQuoteColor2で指定した色になる。LineQuoteが1の場合には、Quote1で指定した文字のいずれかから始まる行は線で表示される。


+ReformLineLength (74)
整形するときの一行の文字数。


+ReformQuote (>|#)
整形するときに引用として扱われる文字。


+ShowCaret (1 @ 0|1), ShowNewLine (1 @ 0|1), ShowTab (1 @ 0|1), ShowRuler (1 @ 0|1), ShowHorizontalScrollBar (0 @ 0|1), ShowVerticalScrollBar (1 @ 0|1)
キャレット、改行文字、タブ、ルーラ、水平スクロールバー、垂直スクロールバーを表示するかどうか。


+URLSchemas (http https ftp file mailto)
リンクにするスキーマ。スペースで区切って指定。


+WordWrap
ワードラップと禁則が有効かどうか。


+TabWidth
タブの幅。指定した値×xの文字幅の位置がタブ位置になる。


+Ime (0)
Imeの状態。


+ArchiveAttachments (0 @ 0|1)
デフォルトで添付ファイルの圧縮が有効かどうか。


+AutoReform (1 @ 0|1)
デフォルトで自動整形が有効かどうか。


+HideHeaderIfNoFocus (0 @ 0|1)
ヘッダエディットビューがフォーカスを失ったときにヘッダエディットビューを隠すかどうか。


// TODO

=end

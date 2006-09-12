=begin
=qmail.xml

QMAIL3全体に関する設定を保存するXMLファイルです。このファイルで設定できる多くの項目は((<オプションの設定|URL:Options.html>))などで設定できますが、一部の項目は直接このファイルを編集しないと設定できません。設定できる項目の一覧は備考を参照してください。

このファイルを編集するときにはQMAIL3を終了させてから編集してください。


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


===EditFrameWindowセクション

+Left (0), Top (0), Width (0), Height (0), Show (1), Alpha (0 @ 0-255)
エディットウィンドウの位置と大きさ、表示方法と透過度。


+ShowToolbar (1 @ 0|1), ShowStatusBar (1 @ 0|1)
エディットウィンドウのツールバーとステータスバーを表示するかどうか。


===EditWindowセクション

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


===Findセクション

+Histroy?
検索した履歴。?は0から始まる数字。


+HistorySize (10)
保存する履歴の最大数。


+Ime (0)
Imeの状態。


+MatchCase (0 @ 0|1)
大文字と小文字を区別するかどうか。


+Regex (0 @ 0|1)
正規表現を使うかどうか。


===FixedFormTextDialogセクション

+Width (620), Height (450)
定型文ダイアログのサイズ。


===FolderComboBoxセクション

+FontFace, FontSize (9), FontStyle (0), FontCharset (0)
フォルダコンボボックスのフォント。フォントサイズはポイント。


+ShowAllCount (1 @ 0|1)
メッセージ数を表示するかどうか。


+ShowUnseenCount (1 @ 0|1)
未読メッセージ数を表示するかどうか。


===FolderListWindowセクション

+FontFace, FontSize (9), FontStyle (0), FontCharset (0)
フォルダリストビューのフォント。フォントサイズはポイント。


+UseSystemColor (1 @ 0|1)
システムの配色を使うかどうか。


+ForegroundColor (000000), BackgroundColor (ffffff)
文字色、背景色。形式はRRGGBB。


+NameWidth (150), IdWidth (50), CountWidth (50), UnseenCountWidth (50), SizeWidth (150)
名前欄、ID欄、メッセージ数欄、未読メッセージ数欄、サイズ欄の幅。



===FolderWindowセクション

+FontFace, FontSize (9), FontStyle (0), FontCharset (0)
フォルダビューのフォント。フォントサイズはポイント。


+UseSystemColor (1 @ 0|1)
システムの配色を使うかどうか。


+ForegroundColor (000000), BackgroundColor (ffffff)
文字色、背景色。形式はRRGGBB。


+AccountShowAllCount (1 @ 0|1)
アカウントにメッセージ数を表示するかどうか。


+AccountShowUnseenCount (1 @ 0|1)
アカウントに未読メッセージ数を表示するかどうか。


+FolderShowAllCount (1 @ 0|1)
フォルダにメッセージ数を表示するかどうか。


+FolderShowUnseenCount (1 @ 0|1)
フォルダに未読メッセージ数を表示するかどうか。


+DragOpenWait (500)
ドラッグアンドドロップでフォルダの上にドラッグしたときにフォルダが開くまでの待ち時間。単位はミリ秒。


+ExpandedFolders
展開されているアカウントとフォルダ。


===FullTextSearchセクション

+Command (namazu -l -a "$condition" "$index")
全文検索で使用するコマンド。


+IndexCommand (mknmz.bat -a -h -O \"$index\" \"$msg\")
全文検索のインデックス更新で使用するコマンド。


===Globalセクション

+Action
ToolInvokeActionアクションで最後に起動したアクション。


+AddZoneId (1 @ 0|1)
添付ファイルを保存するときにZoneIdを付加するかどうか。


+AutoUpdateCheck (1 @ 0|1)
自動バージョンチェックが有効かどうか。


+Bcc (1 @ 0|1)
デフォルトで自分のアドレスをBccに入れるかどうか。


+CharsetAliases (windows-31j=shift_jis)
文字コード名のエイリアス。

エイリアス名=エンコーディング名の形で小文字で指定。複数指定する場合には空白で区切る。


+ConfirmDeleteMessage (0 @ 0|1)
メッセージを削除するときに確認するかどうか。


+ConfirmEmptyFolder (1 @ 0|1)
フォルダを空にするときに確認するかどうか。


+ConfirmEmptyTrash (1 @ 0|1)
ゴミ箱を空にするときに確認するかどうか。


+CurrentFolder
選択されているフォルダ。


+DefaultCharset
デフォルトの文字コード。指定されていない場合にはプラットフォームから自動取得。


+DefaultMailAccount
コマンドラインから-sを使ってmailto URLを指定して起動されたときに使用されるアカウント。


+DefaultRssAccount
コマンドラインから-sを使ってfeed URLを指定して起動されたときに使用されるアカウント。


+DefaultTimeFormat (%Y4/%M0/%D %h:%m:%s)
デフォルトの時間のフォーマット。指定方法は、((<@FormatDate|URL:FormatDateFunction.html>))を参照。


+DetachFolder
デフォルトの添付ファイルを保存するフォルダ。


+DetachOpenFolder (0 @ 0|1)
添付ファイルを保存した後で、保存先のフォルダを開くかどうか。


+Editor (notepad.exe)
外部エディタ。


+EmptyTrashOnExit (0 @ 0|1)
終了時にゴミ箱を空にするかどうか。


+Encodings (iso-8859-1 iso-2022-jp shift_jis euc-jp utf-8)
選択可能な文字コード。複数指定するときには空白区切り。


+ExcludeArchive (\.(?:zip|lzh|tgz|gz)$)
添付ファイルを圧縮するときに除外するファイルを指定する正規表現。


+ExternalEditor
外部エディタ。Editorで指定したものよりも優先される。

Editorで指定したエディタは、添付ファイルをエディタで開いたりするときなどにも使用されます。メール編集用の外部エディタとして他のエディタを使用したい場合にはこちらに指定します。


+ExternalEditorAutoCreate (1 @ 0|1)
外部エディタでメッセージを作成するときに、外部エディタが終了したら自動でメッセージを作成するかどうか。


+Filer
添付ファイルを保存した後でフォルダを開くときに使用するエディタ。指定しない場合には関連付けで開く。


+ForwardRfc822 (0 @ 0|1)
転送するときにmessage/rfc822形式で転送するかどうか。


+HideWhenMinimized (0 @ 0|1)
最小化されたときに隠すかどうか。


+ImeControl (1 @ 0|1)
IMEを自動で制御するかどうか。


+IncrementalSearch (0 @ 0|1)
メッセージ内検索やエディットビューの検索でインクリメンタルサーチを使うかどうか。


+LastUpdateCheck
最後にバージョンチェックをした日時。


+Libraries
ロードする外部ライブラリ。複数指定するときには空白区切り。


+Log (-1 @ -1|0|1|2|3|4)
システムログのログレベル。

:-1
  None
:0
  Fatal
:1
  Error
:2
  Warn
:3
  Info
:4
  Debug


+LogFilter
システムログをモジュール名でフィルタするための正規表現。


+LogTimeFormat (%Y4/%M0/%D-%h:%m:%s%z)
システムログの日付フォーマット。指定方法は、((<@FormatDate|URL:FormatDateFunction.html>))を参照。


+Macro
MessageMacroアクションで最後に指定したマクロ。


+NextUnseenInOtherAccounts (0 @ 0|1)
ViewNextUnseenMessageアクションで他のアカウントの未読メッセージにジャンプするかどうか。


+NextUnseenWhenScrollEnd (0 @ 0|1)
ViewNextMessagePageアクションで最後までスクロールしたときに次の未読メッセージにジャンプするかどうか。


+NoBccForML (0 @ 0|1)
自分をBccに含める場合にMLからのメッセージらしいときにはBccを付加しないかどうか。


+Offline (1 @ 0|1)
オフラインかどうか。


+OpenAddressBook (0 @ 0|1)
メッセージ作成時に自動でアドレス選択ダイアログを開くかどうか。


+OpenRecentInPreview (0 @ 0|1)
新着メッセージリストからメッセージを開くときにプレビューで開くかどうか。


+PrintExtension (html)
印刷するときに書き出すファイルの拡張子。


+Quote (> )
EditPasteWithQuoteアクションなどで使われる引用符。


+RFC2231 (0 @ 0|1)
添付ファイルのファイル名などをRFC2231形式でエンコードするかどうか。


+SaveMessageViewModePerFolder (1 @ 0|1)
メッセージモードをフォルダごとに保存するかどうか。


+SaveOnDeactivate (1 @ 0|1)
非アクティブになったときに保存するかどうか。


+ShowUnseenCountOnWelcome (0 @ 0|1)
Windows XPのようこそ画面に未読メッセージ数を表示するかどうか。


+TemporaryFolder
一時ファイルを置くフォルダ。


+UseExternalEditor (0 @ 0|1)
外部エディタを使用するかどうか。


+WarnExtensions (exe com pif bat scr htm html hta vbs js)
添付ファイルを開くときに警告する拡張子。


+XMailerWithOSVersion (1 @ 0|1)
X-MailerにOSのバージョンを含めるかどうか。


===GoRoundCourseDialogセクション

+Width (620), Height(450)
巡回コースダイアログのサイズ。



===GPGセクション

+Command (gpg.exe)
GPGを起動するときのコマンド。


===HeaderEditWindowセクション

+FontFace, FontSize (9), FontStyle (0), FontCharset (0)
ヘッダエディットビューのフォント。フォントサイズはポイント。


+ImeTo, ImeCc, ImeBcc, ImeSubject
To, Cc, Bcc, Subject欄のImeの状態。


===HeaderWindowセクション

+FontFace, FontSize (9), FontStyle (0), FontCharset (0)
ヘッダビューのフォント。フォントサイズはポイント。


===Imap4Searchセクション

+Command (0 @ 0|1)
IMAP4検索でコマンドを指定するかどうか。


+SearchBody (0 @ 0|1)
IMAP4検索で本文を検索するかどうか。


===InputBoxDialogセクション

+Width (400), Height (300)
((<@InputBox|URL:InputBoxFunction.html>))の複数行ダイアログのサイズ。


===JunkFilterセクション

+BlackList, WhiteList
ブラックリストとホワイトリスト。


+Flags (3)
フラグ。以下の組み合わせを10進で指定。

:0x01
  自動で学習
:0x02
  手動で学習


+MaxTextLen (32768)
判定対象にするテキストの最大サイズ。単位はバイト。


+ThresholdScore (0.95)
スパムと判定する閾値。


===Labelセクション

+Histroy?
ラベルの履歴。?は0から始まる数字。


+HistorySize (10)
保存するラベルの最大数。


===ListWindowセクション

+FontFace, FontSize (9), FontStyle (0), FontCharset (0)
リストビューのフォント。フォントサイズはポイント。


+UseSystemColor (1 @ 0|1)
システムの配色を使うかどうか。


+ForegroundColor (000000), BackgroundColor (ffffff)
文字色、背景色。形式はRRGGBB。


+Ellipsis (1 @ 0|1)
カラムの幅に収まらない文字列の終端を...にするかどうか。


+ShowHeaderColumn (1 @ 0|1)
ヘッダカラムを表示するかどうか。


+SingleClickOpen (0 @ 0|1)
シングルクリックでメッセージウィンドウを開くかどうか。


+TimeFormat (%Y2/%M0/%D %h:%m)
日付フォーマット。指定方法は、((<@FormatDate|URL:FormatDateFunction.html>))を参照。


===MacroDialogセクション

+Width (620), Height(450)
マクロダイアログのサイズ。


===MacroSearchセクション

+Macro (0 @ 0|1)
検索条件がマクロかどうか。


+MatchCase (0 @ 0|1)
大文字と小文字を区別するかどうか。


+SearchHeader (0 @ 0|1)
ヘッダを検索するかどうか。


+SearchBody (0 @ 0|1)
本文を検索するかどうか。


+SearchMacro (@Or(@Contain(%Subject, $Search, $Case), @Contain(%From, $Search, $Case), @Contain(%To, $Search, $Case), @Contain(@Label(), $Search, $Case)))
検索に使用するマクロ。詳細は、((<基本検索|URL:MacroSearch.html>))を参照。

// TODO

=end

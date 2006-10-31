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
   <key name="NextUpdateCheck">2006-08-13T20:40:26+09:00</key>
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
アドレス帳関係の設定をします。

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
アドレス帳ウィンドウの設定をします。

+Left (0), Top (0), Width (0), Height (0), Show (1), Alpha (0 @ 0-255)
ウィンドウの位置と大きさ、表示方法と透過度。


+ShowToolbar (1 @ 0|1), ShowStatusBar (1 @ 0|1)
ウィンドウのツールバーとステータスバーを表示するかどうか。


===AddressBookListWindowセクション
アドレスビューの設定をします。

+AddressWidth (150), NameWidth (150), CommentWidth (150)
ビューのアドレス欄、名前欄、コメント欄の幅。


+FontFace, FontSize (9), FontStyle (0), FontCharset (0)
ビューのフォント。フォントサイズはポイント。


===AutoPilotセクション
((<自動巡回|URL:AutoPilot.html>))の設定をします。

+Enabled (0 @ 0|1)
自動巡回が有効かどうか


+OnlyWhenConnected
ネットワーク接続されているときのみ自動巡回するかどうか。


===ColorsDialogセクション
色の設定ダイアログの設定をします。

+Width (620), Height (450)
ダイアログの大きさ。


===Dialupセクション
ダイアルアップの設定をします。

+Entry
最後に指定したダイアルアップのエントリ名。


===EditFrameWindowセクション
エディットウィンドウの設定をします。

+Left (0), Top (0), Width (0), Height (0), Show (1), Alpha (0 @ 0-255)
ウィンドウの位置と大きさ、表示方法と透過度。


+ShowToolbar (1 @ 0|1), ShowStatusBar (1 @ 0|1)
ウィンドウのツールバーとステータスバーを表示するかどうか。


===EditWindowセクション
エディットビューの設定をします。

+FontFace, FontSize (9), FontStyle (0), FontCharset (0)
ビューのフォント。フォントサイズはポイント。


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
検索の設定をします。

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
定型文ダイアログの設定をします。

+Width (620), Height (450)
ダイアログのサイズ。


===FolderComboBoxセクション
フォルダコンボボックスの設定をします。

+FontFace, FontSize (9), FontStyle (0), FontCharset (0)
ビューのフォント。フォントサイズはポイント。


+ShowAllCount (1 @ 0|1)
メッセージ数を表示するかどうか。


+ShowUnseenCount (1 @ 0|1)
未読メッセージ数を表示するかどうか。


===FolderListWindowセクション
フォルダリストビューの設定をします。

+FontFace, FontSize (9), FontStyle (0), FontCharset (0)
ビューのフォント。フォントサイズはポイント。


+UseSystemColor (1 @ 0|1)
システムの配色を使うかどうか。


+ForegroundColor (000000), BackgroundColor (ffffff)
文字色、背景色。形式はRRGGBB。


+NameWidth (150), IdWidth (50), CountWidth (50), UnseenCountWidth (50), SizeWidth (150)
名前欄、ID欄、メッセージ数欄、未読メッセージ数欄、サイズ欄の幅。



===FolderWindowセクション
フォルダビューの設定をします。

+FontFace, FontSize (9), FontStyle (0), FontCharset (0)
ビューのフォント。フォントサイズはポイント。


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
((<全文検索|URL:FullTextSearch.html>))の設定をします。

+Command (namazu -l -a "$condition" "$index")
全文検索で使用するコマンド。


+IndexCommand (mknmz.bat -a -h -O \"$index\" \"$msg\")
全文検索のインデックス更新で使用するコマンド。


===Globalセクション
全般的な設定をします。

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


+NextUpdateCheck
次にバージョンチェックをする日時。


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
巡回コースダイアログの設定をします。

+Width (620), Height(450)
ダイアログのサイズ。


===GPGセクション
GnuPGの設定をします。

+Command (gpg.exe)
GnuPGを起動するときのコマンド。


===HeaderEditWindowセクション
ヘッダエディットビューの設定をします。

+FontFace, FontSize (9), FontStyle (0), FontCharset (0)
ビューのフォント。フォントサイズはポイント。


+ImeTo, ImeCc, ImeBcc, ImeSubject
To, Cc, Bcc, Subject欄のImeの状態。


===HeaderWindowセクション
ヘッダビューの設定をします。

+FontFace, FontSize (9), FontStyle (0), FontCharset (0)
ビューのフォント。フォントサイズはポイント。


===Imap4Searchセクション
((<IMAP4検索|URL:Imap4Search.html>))の設定をします。

+Command (0 @ 0|1)
IMAP4検索でコマンドを指定するかどうか。


+SearchBody (0 @ 0|1)
IMAP4検索で本文を検索するかどうか。


===InputBoxDialogセクション
入力ダイアログの設定をします。

+Width (400), Height (300)
((<@InputBox|URL:InputBoxFunction.html>))の複数行ダイアログのサイズ。


===JunkFilterセクション
((<スパムフィルタ|URL:JunkFilter.html>))の設定をします。

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
((<ラベル|URL:Label.html>))の設定をします。

+Histroy?
ラベルの履歴。?は0から始まる数字。


+HistorySize (10)
保存するラベルの最大数。


===ListWindowセクション
リストビューの設定をします。

+FontFace, FontSize (9), FontStyle (0), FontCharset (0)
ビューのフォント。フォントサイズはポイント。


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
マクロダイアログの設定をします。

+Width (620), Height(450)
マクロダイアログのサイズ。


===MacroSearchセクション
((<基本検索|URL:MacroSearch.html>))の設定をします。

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


===MainWindowセクション
メインウィンドウの設定をします。

+Left (0), Top (0), Width (0), Height (0), Show (1), Alpha (0 @ 0-255)
ウィンドウの位置と大きさ、表示方法と透過度。


+Placement (F|(L-P))
ウィンドウの配置方法。指定できる値は、((<その他の設定|URL:OptionMisc.html>))の[ビューの配置]を参照。


+PrimaryLocation (100)
外側の分割ウィンドウの分割位置。


+SecondaryLocation (200)
内側の分割ウィンドウの分割位置。


+SecurityMode (0)
((<セキュリティモード|URL:SecurityMode.html>))。以下の組み合わせ。

:0x01
  S/MIMEモード
:0x02
  PGPモード


+ShowToolbar (1 @ 0|1), ShowStatusBar (1 @ 0|1), ShowFolderComboBox (0 @ 0|1), ShowFolderWindow (1 @ 0|1), ShowPreviewWindow (1 @ 0|1)
ツールバー、ステータスバー、フォルダコンボボックス、フォルダウィンドウ、プレビューをそれぞれ表示するかどうか。


===MessageFrameWindowセクション
メッセージウィンドウの設定をします。

+Left (0), Top (0), Width (0), Height (0), Show (1), Alpha (0 @ 0-255)
ウィンドウの位置と大きさ、表示方法と透過度。


+SecurityMode (0)
((<セキュリティモード|URL:SecurityMode.html>))。以下の組み合わせ。

:0x01
  S/MIMEモード
:0x02
  PGPモード


+ShowToolbar (1 @ 0|1), ShowStatusBar (1 @ 0|1)
ツールバー、ステータスバーを表示するかどうか。


===MessageWindow
メッセージビューの設定をします。

+FontFace, FontSize (9), FontStyle (0), FontCharset (0)
ビューのフォント。フォントサイズはポイント。


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


+FontGroup
フォントグループ名。詳細は、((<フォント|URL:Font.html>))を参照。


+SeenWait (0)
メッセージを表示してから既読にするまでの待ち時間。単位は秒。-1にすると既読にしない。


+ShowHeader (1 @ 0|1)
ヘッダビューを隠したときにメッセージビュー内にヘッダを表示するかどうか。


+ShowHeaderWindow (1 @ 0|1)
ヘッダビューを表示するかどうか。


+Template
((<表示用テンプレート|URL:ViewTemplate.html>))。


+ViewFit (0 @ 0|1|2)
HTML表示の配置方法。((<ViewFitアクション|URL:ViewFitAction.html>))を参照。


+ViewMode (32)
((<メッセージ表示モード|URL:MessageViewMode.html>))。


+ViewZoom (-1 @ -1|0|1|2|3|4)
HTML表示の文字サイズ。((<ViewZoomアクション|URL:ViewZoomAction.html>))を参照。


===MoveMessageDialogセクション
メッセージの移動ダイアログの設定をします。

+ShowHidden (0 @ 0|1)
隠されているフォルダを表示するかどうか。


===OptionDialogセクション
オプションダイアログの設定をします。

+Width (620), Height(450)
マクロダイアログのサイズ。


+Panel (0)
選択されているパネル。


===PGPセクション
PGPの設定をします。

+Command (pgp.exe)
PGPを起動するときのコマンド。


+UseGPG (1 @ 0|1)
GnuPGを使うか、PGPを使うか。


===PreviewWindowセクション
プレビューの設定をします。

+FontFace, FontSize (9), FontStyle (0), FontCharset (0)
ビューのフォント。フォントサイズはポイント。


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


+Delay (300)
リストビューでメッセージを選択してからプレビューに反映させるまでの待ち時間。単位はミリ秒。


+FontGroup
フォントグループ名。詳細は、((<フォント|URL:Font.html>))を参照。


+SeenWait (0)
メッセージを表示してから既読にするまでの待ち時間。単位は秒。-1にすると既読にしない。


+ShowHeader (1 @ 0|1)
ヘッダビューを隠したときにメッセージビュー内にヘッダを表示するかどうか。


+ShowHeaderWindow (1 @ 0|1)
ヘッダビューを表示するかどうか。


+Template
((<表示用テンプレート|URL:ViewTemplate.html>))。


+UpdateAlways (0 @ 0|1)
バックグラウンドでメッセージが削除されたなどの状況で選択されたメッセージが変わった場合にも、プレビューに選択されたメッセージを表示するかどうか。


+ViewFit (0 @ 0|1|2)
HTML表示の配置方法。((<ViewFitアクション|URL:ViewFitAction.html>))を参照。


+ViewMode (32)
((<メッセージ表示モード|URL:MessageViewMode.html>))。


+ViewZoom (-1 @ -1|0|1|2|3|4)
HTML表示の文字サイズ。((<ViewZoomアクション|URL:ViewZoomAction.html>))を参照。


===RecentAddressセクション
((<アドレスの自動補完|URL:AddressAutoComplete.html>))で使われる最近使用したメールアドレスの設定をします。

+Address?
アドレスの履歴。?は0から始まる数字。

+Max (10)
覚えておくアドレスの最大数。


===Recentsセクション
((<新着メッセージ通知|URL:Recents.html>))の設定をします。

+Filter
メッセージを新着メッセージ通知の対象にするかどうかを決める正規表現。新着メッセージのURIが指定した正規表現にマッチすると通知される。


+HotKey (65)
新着メッセージ一覧を表示するためのホットキー。仮想キーコードで指定。デフォルトは'A'。


+HotKeyModifiers (5)
新着メッセージ一覧を表示するためのホットキーの装飾キー。仮想キーコードで指定。デフォルトは、Alt+Shift。


+Max (20)
最大の新着メッセージ数。


===RecentsWindowセクション
新着メッセージ通知ウィンドウの設定をします。

+FontFace, FontSize (9), FontStyle (0), FontCharset (0)
通知ウィンドウのフォント。フォントサイズはポイント。

+Alpha (224 @ 0-255)
ウィンドウの透過度。

+AutoPopup (1 @ 0|1)
自動でポップアップするかどうか。

+HideTimeout (20)
自動でポップアップしたときに消えるまでの時間。単位は秒。

+Width (400)
ウィンドウの幅。

+Use (1 @ 0|1)
新着メッセージ通知ウィンドウを使うかどうか。新着メッセージ通知ウィンドウを使わない場合にはメニューで表示されます。


===Replaceセクション
置換の設定をします。

+Histroy?
置換した履歴。?は0から始まる数字。


+HistorySize (10)
保存する履歴の最大数。


+Ime (0)
Imeの状態。


===RulesDialogセクション
振り分けの設定ダイアログの設定をします。

+Width (620), Height (450)
ダイアログの大きさ。


===Searchセクション
検索の設定をします。

+All (0 @ 0|1)
すべてのフォルダを対象に検索するかどうか。


+Condition
検索条件


+Histroy?
検索した履歴。?は0から始まる数字。


+HistorySize (10)
保存する履歴の最大数。


+Ime (0)
Imeの状態。


+NewFolder (0)
検索したときに新しい検索フォルダを作るかどうか。


+Page
検索ダイアログのページ。


+Recursive (0 @ 0|1)
フォルダを再帰的に検索するかどうか。


===Securityセクション
セキュリティの設定をします。

+DefaultMessageSecurity (4112)
デフォルトのセキュリティ設定。以下の組み合わせ。

:0x0000
  なし
:0x0001
  S/MIMEで署名
:0x0002
  S/MIMEで暗号化
:0x0010
  S/MIMEでマルチパート署名を使用
:0x0020
  S/MIMEで自分のアドレスでも暗号化する
:0x0100
  PGPで署名
:0x0200
  PGPで暗号化
:0x1000
  PGP/MIMEを使用


+LoadSystemStore (1 @ 0|1)
システムの証明書ストアからCAの証明書を読み込むかどうか。


===SignaturesDialogセクション
署名の設定ダイアログの設定をします。

+Width (620), Height (450)
ダイアログの大きさ。


===Syncセクション
同期の設定をします。

+Notify (0 @ 0|1|2)
新着メッセージを通知するかどうか。

:0
  常に通知する
:1
  常に通知しない
:2
  自動で同期したときだけ通知する


+Sound (C:\Windows\Media\notify.wav)
新着メッセージがあったときに鳴らすサウンドファイル。


===SyncDialogセクション

+Left (0), Top (0), Width (0), Height (0), Alpha (0 @ 0-255)
ダイアログの位置と大きさと透過度。


+Show (2)
ダイアログを表示する条件。以下のいずれか。

:0
  常に表示する
:1
  常に表示しない
:2
  手動で同期したときだけ表示する


===SyncFiltersDialogセクション
同期フィルタの設定ダイアログの設定をします。

+Width (620), Height (450)
ダイアログの大きさ。


===TabWindowセクション
タブの設定をします。

+FontFace, FontSize (9), FontStyle (0), FontCharset (0)
ビューのフォント。フォントサイズはポイント。


+CurrentTab (0)
現在のタブ。


+Multiline (0 @ 0|1)
タブを複数行にするかどうか。


+Reuse (0 @ 0|1|2)
タブを再利用するかどうか。以下の組み合わせ。

:0x00
  再利用しない
:0x01
  新しいタブを開くときに再利用
:0x02
  タブで表示するフォルダを変更するときに再利用


+Show (0 @ 0|1)
タブを表示するかどうか。


+ShowAllCount (1 @ 0|1)
メッセージ数を表示するかどうか。


+ShowUnseenCount (1 @ 0|1)
未読メッセージ数を表示するかどうか。


=end

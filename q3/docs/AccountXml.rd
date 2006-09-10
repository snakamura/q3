=begin
=account.xml

アカウントごとの設定を保存するXMLファイルです。このファイルで設定できる多くの項目は((<アカウントのプロパティ|URL:AccountProperty.html>))などで設定できますが、一部の項目は直接このファイルを編集しないと設定できません。設定できる項目の一覧は備考を参照してください。


==書式
書式は((<qmail.xml|URL:QmailXml.html>))と同じですのでそちらを参照してください。


==サンプル

 <?xml version="1.0" encoding="utf-8"?>
 <profile>
  <section name="Global">
   <key name="Class">mail</key>
   <key name="SenderAddress">taro@example.org</key>
   <key name="SenderName">Taro Yamada</key>
  </section>
  <section name="Receive">
   <key name="Host">mail.example.org</key>
   <key name="Port">110</key>
   <key name="Type">pop3</key>
   <key name="UserName">taro</key>
  </section>
  <section name="Send">
   <key name="Host">mail.example.org</key>
   <key name="Port">25</key>
   <key name="Type">smtp</key>
  </section>
 </profile>


==スキーマ
スキーマは((<qmail.xml|URL:QmailXml.html>))と同じですのでそちらを参照してください。


==備考
このファイルではセクションとキーで値を指定します。例えば、上の例ではGlobalセクションのClassキーにmailという値が指定されています。このドキュメント中ではこれをGlobal/Classのように記述してあることがあります。

それぞれのキーはデフォルトの値を持っていて、指定されていない場合にはその値が使用されます。また、値がデフォルトの値と同じ場合にはファイルには書き出されません。存在しないキーの値を指定する場合には、新しくセクションやキーを追加してください。

指定できるセクションとキーは以下の通りです。


===Dialupセクション
ダイアルアップの設定をします。

+DisconnectWait (0)
ダイアルアップを切断するまでの待ち時間。単位は秒。


+Entry
ダイアルアップのエントリ名。


+ShowDialog (0 @ 0|1)
ダイアルアップ時にダイアログを表示するかどうか。


+Type (0 @ 0|1|2)
ダイアルアップのタイプ。

:0
  ダイアルアップしない
:1
  ネットワーク接続されていないときだけダイアルアップする
:2
  常にダイアルアップする


===FullTextSearchセクション
全文検索の設定をします。

+Index
インデックスのあるディレクトリ。空の場合にはアカウントディレクトリのindexディレクトリ。


===Globalセクション
全般的な設定をします。

+AddMessageId (1 @ 0|1)
Message-Idを付加するかどうか。


+AutoApplyRules (0 @ 0|1)
自動振り分けを有効にするかどうか。


+BlockSize (0)
メッセージボックスのブロックサイズ。

:0
  一メッセージ一ファイル
:-1
  分割なし
:それ以外
  指定された数値で分割（単位はMB）


+Class
アカウントクラス。


+Identity
サブアカウントの同一性。


+IndexBlockSize (-1)
インデックスのブロックサイズ。-1で分割しない。単位はMB。


+IndexMaxSize (-1)
メモリにキャッシュするインデックスの数。-1で無制限。


+LogTimeFormat (%Y4/%M0%D-%h:%m%s%z)
通信ログの日付フォーマット。指定方法は、((<@FormatDate|URL:FormatDateFunction.html>))を参照。


+MessageStorePath
メッセージを保存するディレクトリ。空の場合にはアカウントディレクトリ。


+ReplyTo
Reply-Toに設定するアドレス。


+SenderAddress
Fromに設定するアドレス。


+SenderName
Fromに設定する名前。


+ShowUnseenCountOnWelcome (1 @ 0|1)
Windows XPのようこそ画面に未読メッセージ数を表示するかどうか。


+SslOption (0)
SSLのオプション。指定できる値は、((<SSL|URL:SSL.html>))を参照。


+StoreDecodedMessage (0 @ 0|1)
S/MIMEやPGPで復号したメッセージを全文検索用に保存するかどうか。


+SubAccount
現在のサブアカウント。


+Timeout (60)
タイムアウト。単位は秒。


+TrasnferEncodingFor8Bit ( @ 8bit, base64, quoted-printable)
8bitの文字をエンコードする方法。指定しない場合には文字コードによって決まる。


+TreatAsSent (1 @ 0|1)
受信したメッセージのFromが自分のアドレスだった場合に送信済みとして扱うかどうか。


===Httpセクション
HTTPに関する設定をします。RSSアカウントで使用されます。

+UseProxy (0 @ 0|1)
プロキシを使うかどうか。


+UseInternetSetting
インターネットのプロパティで指定したプロキシの設定を使うかどうか。


+ProxyHost, ProxyPort (8080), ProxyUserName, ProxyPassword
プロキシのホスト名、ポート、ユーザ名、パスワード。


===Imap4セクション
IMAP4に関する設定をします。IMAP4アカウントで使用されます。

+AdditionalFields
サーバからメッセージのインデックスを取得するときに追加で取得するヘッダ名。空白で区切って指定。


+AuthMethods
使用を許可する認証方式。


+CloseFolder (0 @ 0|1)
フォルダを閉じるときに削除マークの付いたメッセージを削除するかどうか。


+FetchCount (100)
インデックスを取得するときに一回のリクエストで取得するメッセージの数。


+ForceDisconnect (0)
強制的にセッションを切断するまでの待ち時間。単位は秒。

セッションを使うときにここで指定した時間以上にアイドル状態が続いたセッションは強制的に切断されます。0を指定すると切断しません。例えば、NATを使っている場合、IMAP4サーバが接続を切る前にNATの変換テーブルがクリアされてしまうと、切断されたのを検出できないため普通に切断するのに時間がかかります。この場合、NATのテーブルがクリアされる時間よりも短い時間をここに指定すると、それ以上アイドル状態だったら接続を強制的に切断するので時間がかかることがなくなります。


+MaxSession (5)
オンラインモードで使用するセッションの最大数。


+Option (255)
オプション。以下の組み合わせを十進数で指定する。

:0x01
  ENVELOPEを使う
:0x02
  BODYSTRUCTUREを常に使う
:0x04
  BODYSTRUCTUREを信頼する


+OutboxFolder (Outbox), DraftFolder (Outbox), SentboxFolder (Sentbox), TrashFolder (Trash), JunkFolder (Junk)
送信箱、草稿箱、送信済み、ゴミ箱、スパムフォルダとして使うフォルダ名。


+Reselect (1 @ 0|1)
同期した時間よりも前にフォルダを選択していた場合に選択しなおすかどうか。

IMAP4サーバによっては（例えば、Courier-IMAPサーバ）、セッションAがフォルダ1を選択した後でセッションBがフォルダ1にメッセージを追加したりすると、セッションAからはフォルダを選択しなおすまで追加したメッセージが見えません。Reselectに1を指定すると、フォルダを選択した時間が同期した時間よりも前の場合にはフォルダを選択しなおします。


+RootFolder
ルートフォルダ。


+RootFolderSeparator (/)
ルートフォルダのセパレータ。


+SearchCharset
検索時に使用する文字コード。デフォルトでは自動判定。


+SearchUseCharset (1)
検索時に文字コードを指定するかどうか。


+UseNamespace (0)
ネームスペースを使うかどうか。


+UsePersonal (1), UseShared (1), UseOthers (1)
ネームスペースを使うときに、パーソナル、共有、その他のフォルダを見せるかどうか。


===JunkFilterセクション
スパムフィルタの設定をします。

+Enabled (0 @ 0|1)
スパムフィルタが有効かどうか。


===Miscセクション

+IgnoreError (0 @ 0|1)
エラーが起きても無視するかどうか。


===Nntpセクション
NNTPに関する設定をします。NNTPアカウントで使用されます。

+ForceDisconnect (0)
強制的にセッションを切断するまでの待ち時間。単位は秒。

詳細はImap4セクションの同じ項を参照。


+InitialFetchCount (300)
初めてメッセージを取得するときに取得するメッセージ数。


+UseXOVER (1 @ 0|1)
XOVERコマンドを使用するかどうか。


+XOVERStep (100)
XOVERコマンドでインデックスを取得するときに一回のリクエストで取得するメッセージの数。


===Pop3セクション
POP3に関する設定をします。POP3アカウントで使用されます。

+Apop (0 @ 0|1)
APOPを使用するかどうか。


+DeleteBefore (0)
受信してからサーバ上のメッセージを削除するまでの日数。0の場合には削除しない。


+DeleteLocal (0 @ 0|1)
サーバ上のメッセージを削除するときにローカルのメッセージも削除するかどうか。


+DeleteOnServer (0 @ 0|1)
受信したメッセージをサーバから削除するかどうか。


+GetAll (20)
UIDLやLISTのリクエストをまとめて出す閾値。

UIDLやLISTは、すべてのメッセージの分をまとめて取得する方法と、各メッセージで個別に取得する方法があります。サーバ上に既に受信済みのメッセージが多い場合には各メッセージで個別に取得するほうが速くなります。サーバ上にx通のメッセージがあり、サーバ上にある既に受信済みのメッセージがy通だった場合に、x/(x - y)がここで指定した数値を超えた場合にはまとめて取得します。


+HandleStatus (0 @ 0|1)
メッセージにStatus: ROのヘッダが付いていたときに既読にするかどうか。


+SkipDuplicatedUID (0 @ 0|1)
UIDLが同じメッセージを強制的に無視するかどうか。

POP3でサーバ上のメッセージを未読管理するときには、前回最後に受信したメッセージのUIDを持つメッセージを探し、それ以降のメッセージを取得します。このため、そのメッセージよりも後ろに既に受信したメッセージがあると二重に受信することがあります。SkipDuplicatedUIDを1にすると同じUIDを持つメッセージはどこに現れても無視します。偶然異なるメッセージのUIDがサーバ上で同じになってしまった場合にも受信しなくなりますので注意してください。


===Pop3Sendセクション
送信用のPOP3に関する設定をします。送信プロトコルとしてPOP3 (XTND XMIT)を選んだ場合に使用されます。

+Apop (0 @ 0|1)
APOPを使用するかどうか。


===Receiveセクション
受信に関する一般的な設定をします。

+Host
受信用サーバのホスト名。


+Port
受信用サーバのポート。


+Log (0 @ 0|1)
ログを取るかどうか。


+Secure (0 @ 0|1|2)
SSLの設定。

:0
  SSLを使用しない
:1
  SSLを使用する
:2
  STARTTLSを使用する


+SyncFilterName
同期フィルタの名前。


+Type
受信プロトコル。


+UserName
受信用サーバのユーザ名。


===Sendセクション
送信に関する一般的な設定をします。

+Host
送信用サーバのホスト名。


+Port
送信用サーバのポート。


+Log (0 @ 0|1)
ログを取るかどうか。


+Secure (0 @ 0|1|2)
SSLの設定。指定できる値は、Receive/Secureと同じ。


+Type
送信プロトコル。


+UserName
送信用サーバのユーザ名。


===Smtpセクション
SMTPに関する設定をします。送信プロトコルとしてSMTPを選んだ場合に使用されます。

+AuthMethods
SMTP認証で使用を許可する認証方式。


+EnvelopeFrom
EnvelopeFromとして使うメールアドレス。指定しない場合にはFromまたはSenderのアドレスが使用される。


+LocalHost
EHLOまたはHELOで送られるホスト名。指定しない場合には、現在のホストの名前。

+PopBeforeSmtp (0 @ 0|1)
POP before SMTPを使うかどうか。


+PopBeforeSmtpWait
POP before SMTPでPOP3で認証した後にSMTPで送信するまでの待ち時間。


+PopBeforeSmtpCustom (0 @ 0|1)
POP before SMTPでカスタム設定を使うかどうか。


+PopBeforeSmtpProtocol (pop3 @ pop3|imap4)
POP before SMTPでカスタム設定の時のプロトコル。

+PopBeforeSmtpHost
POP before SMTPでカスタム設定の時のホスト名またはIPアドレス。


+PopBeforeSmtpPort
POP before SMTPでカスタム設定の時のポート。


+PopBeforeSmtpSecure (0 @ 0|1|2)
POP before SMTPでカスタム設定の時にSSLを使うかどうか。指定できる値は、Receive/Secureと同じ。

+PopBeforeSmtpApop (0 @ 0|1)
POP before SMTPでカスタム設定の時にAPOPを使うかどうか。プロトコルがpop3の場合のみ。


===UIセクション

+FolderTo
メッセージの移動ダイアログでデフォルトで選択されるフォルダ。

=end

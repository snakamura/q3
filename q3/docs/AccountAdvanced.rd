=begin
=高度の設定

高度の設定を行います。

((<高度タブ|"IMG:images/AccountAdvancedPage.png">))


+[同期フィルタ]
使用する同期フィルタを指定します。同期フィルタについては、((<同期フィルタ|SyncFilter.html>))を参照してください。


+[編集]
同期フィルタを編集します。


+[送信前に受信サーバに接続する]
送信する前に受信サーバに接続するかどうかを指定します。POP3 before SMTPを使いたい場合には、ここにチェックを入れます。デフォルトでは接続しません。


+[Message-Idを付加]
Message-Idを自動で付加するかどうかを指定します。デフォルトでは、mailクラスのアカウントでは自動で付加し、newsクラスのアカウントでは付加しません。


+[Content-Transfer-Encoding: 8bitを使用]
8bitの文字コードを使用するときに、Content-Transfer-Encodingに8bitを使うかどうかを指定します。使わない場合には、適宜base64またはquoted-printableが使用されます。デフォルトでは使用しません。


+[自動で振り分ける]
受信したメッセージを自動で振り分けるかどうかを指定します。デフォルトでは自動で振り分けません。振り分けについては、((<振り分け|URL:ApplyRules.html>))を参照してください。


+[スパムフィルタを有効にする]
スパムフィルタを有効にするかどうかを指定します。デフォルトでは無効です。スパムフィルタについては、((<スパムフィルタ|URL:JunkFilter.html>))を参照してください。


+[自分のアドレスからのメールは送信済みとみなす]
差出人が自分のアドレスのメールを受け取った場合には、送信済みフラグを立てて送信済みメッセージとして処理するかどうかを指定します。Bccに自分のメールアドレスを設定してこの設定をチェックすると、複数のクライアントでPOP3を使うときに自分の送ったメッセージを管理できます。デフォルトでは送信済みとみなします。


+[全文検索用に復号済みメッセージをキャッシュする]
((<"S/MIME"|URL:SMIME.html>))や((<PGP|URL:PGP.html>))で暗号化・署名されたメッセージを復号したときに、復号されたメッセージを全文検索用にキャッシュするかどうかを指定します。チェックすると全文検索エンジンが復号済みのメッセージを処理できるようにファイルとして保存します。デフォルトではキャッシュしません。


+[同一性]
サブアカウントの同一性を指定します。詳細は((<サブアカウント|URL:SubAccount.html>))を参照してください。


+[タイムアウト]
サーバに接続するときのタイムアウトを指定します。単位は秒です。デフォルトは20秒です。

=end

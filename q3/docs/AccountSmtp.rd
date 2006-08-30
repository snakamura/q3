=begin
=SMTPの設定

SMTPでメッセージを送信するための設定を行います。

((<[SMTP]タブ|"IMG:images/AccountSmtpSendPage.png">))


+[ローカルホスト]
SMTPのHELOまたはEHLOコマンドに渡すローカルホスト名を指定します。通常、ローカルホスト名は自動的に検出されるため指定する必要はありません。別のホスト名を指定したいときにはここで指定することができます。デフォルトでは指定されません。


+[POP before SMTPを使う]
POP before SMTPを使うかどうかを指定します。デフォルトでは使用しません。POP before SMTPを有効にすると、((<ユーザの設定|URL:AccountUser.html>))の[送信]で[認証]にチェックを入れていてもSMTP認証は行われません。


+[待ち時間]
POP before SMTPでPOPサーバに接続してからSMTPサーバに接続するまでの待ち時間を指定します。デフォルトでは3秒です。


+[カスタム]
POP before SMTPの詳細な設定を行います。[POP before SMTP]ダイアログが開きます。


===[POP before SMTP]ダイアログ
POP before SMTPの詳細を設定します。デフォルトの設定では受信側で設定したサーバに接続しますが、ここで別のサーバを指定することができます。例えば、IMAP4アカウントなのにSMTPの認証だけはPOP before SMTPでPOP3サーバに接続しなくてはいけないというような場合に指定します。

このダイアログで、[カスタム設定を使用]を選んだ場合には、指定したサーバに接続するときの認証情報（ユーザ名とパスワード）は、((<ユーザの設定|URL:AccountUser.html>))の[送信]側で指定した情報が使われます。[認証]にチェックを入れ、ユーザ名を指定してください。

((<[POP before SMTP]ダイアログ|"IMG:images/PopBeforeSmtpDialog.png">))


+[デフォルトの設定を使用]
デフォルトの設定を使用します。


+[カスタム設定を使用]
接続するサーバなどを指定します。


+[プロトコル]
プロトコルを選択します。POP3とIMAP4が選択できます。


+[ホスト]
接続するサーバのホスト名またはIPアドレスを指定します。


+[ポート]
接続するサーバのポートを指定します。


====[安全]
SSLやSTARTTLSを使うかどうかを指定します。SSLやSTARTTLSについては、((<SSL|URL:SSL.html>))を参照してください。


+[なし]
SSLを使用しません。


+[SSL]
SSLを使用します。


+[STARTTLS]
STARTTLSを使用します。


+[APOPを使用]
プロトコルとしてPOP3を選んだ場合に、認証でAPOPを使用するかどうかを指定します。

=end

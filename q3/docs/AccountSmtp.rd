=begin
=SMTPの設定

SMTPでメッセージを送信するための設定を行います。

((<[SMTP]タブ|"IMG:images/AccountSmtpSendPage.png">))


+[ローカルホスト]
SMTPのHELOまたはEHLOコマンドに渡すローカルホスト名を指定します。通常、ローカルホスト名は自動的に検出されるため指定する必要はありません。別のホスト名を指定したいときにはここで指定することができます。デフォルトでは指定されません。


+[POP before SMTPを使う]
POP before SMTPを使うかどうかを指定します。デフォルトでは使用しません。POP before SMTPを有効にすると、((<ユーザの設定|URL:AccountUser.html>))の[送信]で[認証]にチェックを入れていてもSMTP認証は行われません。


+[待ち時間]
POP before SMTPでPOPサーバに接続してからSMTPサーバに接続するまでの待ち時間を秒単位で指定します。デフォルトでは3秒です。


+[カスタム]
POP before SMTPの詳細な設定を行います。((<[POP before SMTP]ダイアログ|URL:PopBeforeSmtpDialog.html>))が開きます。

=end

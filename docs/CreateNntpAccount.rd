=begin
=アカウントの作成

==アカウントの作成
アカウントの作成方法は、POP3アカウントの((<アカウントの作成|URL:CreatePop3Account.html>))とほぼ同じです。

[アカウントの作成]ダイアログでは、[クラス]に「news」を選択し、[受信プロトコル]と[送信プロトコル]に「NNTP」を選択します。

((<[アカウントの作成]ダイアログ|"IMG:images/TutorialNntpCreateAccountDialog.png">))


==アカウントの設定
まずは、[一般]タブです。

((<[一般]タブ|"IMG:images/TutorialNntpGeneralPage.png">))

[サーバ情報]の[受信]と[送信]にはNNTPサーバのホスト名を指定します。ほとんどの場合、両方とも同じホスト名になりますが、省略せずに両方指定します。

次に[ユーザ情報]を指定します。[名前]に自分の名前を、[メールアドレス]に使用するメールアドレスを指定します。ここで指定した名前とメールアドレスがFromヘッダで使用されます。[メールアドレス]とは別のアドレスに返信をもらいたい場合には、[返信先アドレス]にメールアドレスを指定します。ここで指定したアドレスがReply-Toヘッダで使用されます。


次に、[ユーザ]タブをクリックします。

((<[ユーザ]タブ|"IMG:images/TutorialNntpUserPage.png">))

認証が必要な場合には、[ユーザ名]と[パスワード]を指定します。パスワードを保存したくない場合、空のままにしておくと接続するときに尋ねられます。まったく同じであっても、[受信]と[送信]の両方を設定する必要があります。認証が必要ない場合には、両方とも空のままにします。


[詳細]タブでは、SSLを使用する場合には[SSL]を選択してください。

((<[詳細]タブ|"IMG:images/TutorialNntpDetailPage.png">))


次に、[NNTP]タブをクリックします。

((<[NNTP]タブ|"IMG:images/TutorialNntpNntpPage.png">))

[初期取得数]には初めてグループを購読したときに取得するメッセージの数を指定します。

[XOVERを使用]にチェックを入れるとXOVERコマンドを使用してメッセージを取得します。XOVERコマンドをサポートしていないサーバの場合にはチェックを外してください。


[NNTP (POST)]タブは設定項目がないので飛ばします。[ダイアルアップ]タブと[高度]タブはPOP3アカウントと同様に指定します。

=end

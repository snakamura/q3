=begin
=アカウントの作成

==アカウントの作成
POP3のアカウントを作成するには、メニューから((<[ツール]-[アカウント]|URL:ToolAccountAction.html>))を選択し、[アカウント]ダイアログを開きます。

((<[アカウント]ダイアログ|"IMG:images/TutorialAccountDialog.png">))

[アカウント]ダイアログの[アカウント追加]ボタンをクリックし、[アカウントの作成]ダイアログを開きます。

((<[アカウントの作成]ダイアログ|"IMG:images/TutorialPop3CreateAccountDialog.png">))

[名前]には任意のアカウント名を指定します。ただし、ファイル名として使えない文字は使えません。

[クラス]は「mail」を、[受信プロトコル]は「POP3」を選択します。[送信プロトコル]は通常「SMTP」を選択します。サーバ管理者からPOP3のXTND XMITを使うように指定されている場合には「POP3 (XTND XMIT)」を選択します。

その他の設定はデフォルトのままをお勧めします。詳細については、((<アカウントの作成|URL:CreateAccount.html>))を参照してください。また、ここで指定した項目はアカウント名以外は後で変更することはできません。

[OK]をクリックすると、アカウントが作成されアカウントの設定ダイアログが開きます。ここで、アカウントの設定を行います。


==アカウントの設定
まずは、[一般]タブです。

((<[一般]タブ|"IMG:images/TutorialPop3GeneralPage.png">))

はじめに[サーバ情報]を指定します。[受信]にPOP3サーバのホスト名を、[送信]にSMTPサーバのホスト名を指定します。両方が同じホストの場合でも省略せずに両方指定します。

次に[ユーザ情報]を指定します。[名前]に自分の名前を、[メールアドレス]に使用するメールアドレスを指定します。ここで指定した名前とメールアドレスがFromヘッダで使用されます。[メールアドレス]とは別のアドレスに返信をもらいたい場合には、[返信先アドレス]にメールアドレスを指定します。ここで指定したアドレスがReply-Toヘッダで使用されます。


次に、[ユーザ]タブをクリックします。

((<[ユーザ]タブ|"IMG:images/TutorialPop3UserPage.png">))

[受信]のところにPOP3サーバのログイン情報を設定します。[ユーザ名]にユーザ名を、[パスワード]にパスワードを指定します。パスワードを保存したくない場合には、空のままにしておくと接続するときにパスワードを尋ねられます。

SMTP認証を使う場合には[送信]の[認証]にチェックを入れ、[ユーザ名]と[パスワード]を同様に指定します。POP before SMTPを使う場合には、[送信]の[認証]にはチェックを入れません。


次に、[詳細]タブをクリックします。

((<[詳細]タブ|"IMG:images/TutorialPop3DetailPage.png">))

[受信], [送信]ともポート、SSLの使用、ログの設定を行います。通常はデフォルトのままで問題ありません。使用するポートがデフォルトと異なる場合には[ポート]にポート番号を指定します。また、SSLを使用する場合には、[SSL]または[STARTTLS]を選択します。

[ログ]にチェックを入れると通信のログを取ることができます。メールの送受信がうまくできない場合などには、ここにチェックを入れてログを取って調べることで問題の解消に役立てることができます。


次に、[POP3]タブをクリックします。

((<[POP3]タブ|"IMG:images/TutorialPop3Pop3Page.png">))

認証にAPOPを使う場合には[APOP]にチェックを入れます。

受信したメッセージをサーバから削除する場合には[サーバ上のメッセージ]の[削除する]を選択します。あるいは、受信してから一定日数以上経過したメッセージをサーバから削除する場合には、[古いメッセージを削除]を選択し、[?日後]で日数を指定します。

その他の設定については、((<POP3の設定|URL:AccountPop3.html>))を参照してください。


次に、[SMTP]タブをクリックします。

((<[SMTP]タブ|"IMG:images/TutorialPop3SmtpPage.png">))

POP before SMTPを使用する場合には、[POP before SMTPを使う]にチェックを入れます。

その他の設定については、((<SMTPの設定|URL:AccountSmtp.html>))を参照してください。


最後に、[ダイアルアップ]タブをクリックします。

((<[ダイアルアップ]タブ|"IMG:images/TutorialPop3DialupPage.png">))

送受信時に自動的にダイアルアップしたい場合には、[ネットワークに接続していないときにダイアルアップ接続する]または[常にダイアルアップ接続する]を選択し、[エントリ名]を選択します。


[OK]をクリックしてダイアログを閉じれば、POP3アカウントの設定は終了です。

=end

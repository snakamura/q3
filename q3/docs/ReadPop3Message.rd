=begin
=メールを読む

まず、フォルダビューで作成したアカウントの受信箱を選択します。フォルダビューのアカウント名をダブルクリックしてツリーを展開し、受信箱を選択します。

// TODO 画像更新

((<受信箱の選択|"IMG:images/TutorialPop3SelectInbox.png">))

この時点では受信済みのメッセージがありませんので、リストビューには何も表示されていません。


==メールを受信する
メールを受信するには、メニューから((<[ツール]-[受信]|URL:ToolReceiveAction.html>))を選択します。

// TODO 画像 同期ダイアログ

受信中は同期ダイアログが表示され、受信の進捗状況を確認できます。エラーがなければ受信が終了すれば同期ダイアログは自動的に閉じます。


==メールを読む
受信したメールを読むには、リストビューで読みたいメッセージをクリックします。すると、リストビューでメッセージが選択され、プレビューにメッセージの内容が表示されます。

// TODO 画像 プレビュー

別ウィンドウでメッセージを読みたい場合には、リストビューで開きたいメッセージをダブルクリックすると、メッセージウィンドウが開きます。

// TODO 画像 メッセージウィンドウ

次のメッセージや前のメッセージを読みたいときには、((<[表示]-[次のメッセージ]|URL:ViewNextMessageAction.html>))や((<[表示]-[前のメッセージ]|URL:ViewPrevMessageAction.html>))を使うことができます。((<[表示]-[次の未読メッセージ]|URL:ViewNextUnseenMessageAction.html>))を使うと次の未読メッセージにジャンプすることができます。

((<スペースキー|URL:ViewNextMessagePageAction.html>))を押すと一ページずつスクロールし、最後までスクロールし終わると次のメッセージを表示します。((<Shiftキーを押しながらスペースキー|URL:ViewPrevMessagePageAction.html>))を押すと一ページずつ前にスクロールし、最初までスクロールし終わると一つ前のメッセージを表示します。


===HTMLメールを読む
HTMLメールをHTML表示で読むには、((<[表示]-[HTML]-[HTMLを表示]|URL:ViewHtmlModeAction.html>))を選択します。

HTMLメールに外部サーバ上に置かれた画像が含まれている場合には、デフォルトではこれらの画像は表示されません。これらの画像を表示するには、((<[表示]-[HTML]-[オンラインで表示]|URL:ViewHtmlOnlineModeAction.html>))を選択します。このオプションを有効にすると、外部サーバに置かれた画像を取得しますので、使い方によっては送信者にメールを開いたことが通知される可能性があることに注意してください。

またデフォルトではHTMLメールは制限つきゾーンで表示されますので、スクリプトなどは実行されません。これらを実行するには、((<[表示]-[HTML]-[インターネットゾーンで表示]|URL:ViewHtmlInternetZoneModeAction.html>))を選択します。このオプションを有効にすると、インターネットゾーンで表示されます。

=end

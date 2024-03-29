=begin
=メールを書く

==新規にメールを書く
新規にメールを書くときには、((<[メッセージ]-[新規]|URL:MessageCreateAction.html>))を選択します。選択すると、エディットウィンドウが開きます。

((<エディットウィンドウ|"IMG:images/TutorialPop3CreateMessage.png">))

[宛先], [Cc], [Bcc]にそれぞれ送信先アドレスを入力します。複数のメールアドレスを指定するには、「,」で区切ります。((<[ツール]-[アドレス帳]|URL:ToolSelectAddressAction.html>))を選択し、アドレス帳からこれらのアドレスを選択することもできます。

((<[アドレス選択]ダイアログ|"IMG:images/TutorialPop3SelectAddressDialog.png">))

デフォルトではBccに自分のアドレスが設定されるようになっています。これにより、自分の送信したメールのコピーが自分自身にも送られますので、複数のPCで送信済みのメールを管理するのに便利です。デフォルトでBccに自分のアドレスを設定したくない場合には、((<[オプション]ダイアログの[その他2]パネル|URL:OptionMisc2.html>))で[自分をBccに含める]のチェックを外します。

[件名]と本文を入力したら、((<[ファイル]-[送信]|URL:FileSendAction.html>))を選択すると、送信箱に送信待ちメッセージとして保存されます。送信箱に保存されたメッセージは次回の送信時に送信されます。今すぐに送信したい場合には、((<[ファイル]-[すぐに送信]|URL:FileSendNowAction.html>))を選択すると、送信箱に保存し、すぐにメッセージを送信します。

草稿として保存したい場合には、((<[ファイル]-[草稿として保存]|URL:FileDraftAction.html>))を選択すると、草稿箱に草稿として保存されます。また、((<[ファイル]-[草稿として保存して閉じる]|URL:FileDraftCloseAction.html>))を選択すると、草稿として保存してエディットウィンドウを閉じます。


===ファイルの添付
ファイルを添付したい場合には、添付したいファイルをエクスプローラからエディットウィンドウにドラッグアンドドロップします。すると、[添付]のところに添付されたファイルが表示されます。

また、((<[ツール]-[添付ファイル]|URL:ToolAttachmentAction.html>))を選択し、[添付ファイル]ダイアログを使用してファイルを添付することもできます。

((<[添付ファイル]ダイアログ|"IMG:images/TutorialPop3AttachmentDialog.png">))

メッセージに別のメッセージを添付したい場合には、添付したいメッセージをリストビューからエディットウィンドウにドラッグアンドドロップします。このようにすることで、メッセージを((<"message/rfc822形式"|URL:http://www.ietf.org/rfc/rfc2046.txt>))で添付することができます。


==メールを送信する
メインウィンドウで、((<[ツール]-[送信]|URL:ToolSendAction.html>))を選択すると、送信箱に保存されたメッセージが全て送信されます。

// TODO 画像 同期ダイアログ

送信中は同期ダイアログが表示され、送信の進捗状況を確認できます。エラーがなければ送信が終了すれば同期ダイアログは自動的に閉じます。


==返信する
返信するときには、返信元のメッセージを選択して((<[メッセージ]-[返信]|URL:MessageCreateAction.html>))を選択します。返信用にメッセージを作成すると、返信用の件名が挿入され、本文が引用された状態でエディットウィンドウが開きます。また、このときには返信元のメッセージを特定するためのヘッダが挿入されます。

((<[メッセージ]-[全員に返信]|URL:MessageCreateAction.html>))を選択すると、返信元のメッセージの宛先やCcのアドレスが自動的にCcに設定されます。


==転送する
転送するときには、転送するメッセージを選択して((<[メッセージ]-[転送]|URL:MessageCreateAction.html>))を選択します。

メールを転送するときには、本文として転送する方法と、((<"message/rfc822形式"|URL:http://www.ietf.org/rfc/rfc2046.txt>))を使用して転送する方法の二通りが選べます。message/rfc822形式を使用した場合、複数のメッセージが選択されていると全てのメッセージが転送されます。どちらの形式を使用するかは、((<[オプション]ダイアログの[その他2]パネル|URL:OptionMisc2.html>))で指定できます。

=end

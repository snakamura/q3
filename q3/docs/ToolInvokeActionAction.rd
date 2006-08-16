=begin
=ToolInvokeActionアクション

引数で指定されたアクションを実行します。引数を指定しなかった場合には、[アクションの起動]ダイアログを開くので、実行するアクションを指定します。

アクションは以下のような形式で指定します。

 <アクション名> [引数 [引数]...](|<アクション名> [引数 [引数]...])

まず、はじめにアクション名を指定します。引数を指定する場合にはその後ろに空白区切りで引数を指定します。引数に空白が含まれる場合には引用符（""）で括ります。複数のアクションを連続して実行する場合には、それらを「|」で区切ります。

例えば、メッセージビューで返信メッセージを作成すると同時にメッセージウィンドウを閉じるには以下のように指定します。

 MessageCreate reply|FileClose


===[アクションの起動]ダイアログ
実行するアクションを指定します。

((<[アクションの起動]ダイアログ|"IMG:images/ActionDialog.png">))


+[アクション]
実行するアクションを指定します。


==引数
:1
  実行するアクション


==有効なウィンドウ・ビュー
*メインウィンドウ
*メッセージウィンドウ
*エディットウィンドウ
*アドレス帳ウィンドウ

=end

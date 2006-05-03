=begin
=POP3の設定

POP3でメッセージを受信するための設定を行います。

((<[POP3]タブ|"IMG:images/AccountPop3ReceivePage.png">))


+[APOPを使用]
認証時にAPOPを使用するかどうかを指定します。デフォルトではAPOPを使用しません。


+[サーバからメッセージを削除]
受信したメッセージをサーバから削除するかどうかを指定します。デフォルトではメッセージを削除しません。


+[?日経ったらサーバからメッセージを削除]
受信してから指定した日数以上経過したメッセージをサーバから削除します。0を指定するとサーバからメッセージを削除しません。[サーバからメッセージを削除]をチェックしたにはこの設定は無視されます。デフォルトではメッセージを削除しません。


+[メッセージをサーバから削除したらローカルからも削除]
サーバ上のメッセージを選択削除したときに、ローカルのメッセージも削除するかどうかを指定します。デフォルトでは削除しません。


+[Status:がROだったら既読にする]
Statusヘッダの値がROだったら受信したメッセージを既読にします。


+[重複するUIDは強制的に無視]
通常、UIDLコマンドで返されるUIDはメッセージごとに異なりますが、サーバの設定などによっては複数のメッセージに対して同じUIDを返すことがあります。QMAIL3ではこれらのメッセージを正しく取り込むため、同じUIDであっても異なるメッセージであると判断すると受信します。この判定がうまくいかず、同じメッセージを何度も取り込んでしまう場合、この設定にチェックを入れると同じUIDのメッセージは強制的に無視するようになります。デフォルトでは無視しません。

=end

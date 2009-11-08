=begin
=開封通知を送るにはどうすれば良いですか?

以下のような方法で開封通知要求が付いているメールに開封通知を送ることができます。

開封通知要求が付いているメールにはDisposition-Notification-Toというヘッダが付いています。このヘッダが付いているメールを表示したときにマクロを実行することで開封通知を送ります。このとき、同じメールに対して何度も開封通知を送ってしまわないように、User1フラグを使います。つまり、開封通知を送ったらUser1フラグを立てるようにし、User1フラグが立っているメールに対しては開封通知を送らないようにします。

メールを表示したときにマクロを実行させるためには、((<表示用のテンプレート|URL:ViewTemplate.html>))を使うのが一般的ですが、そうするとHTMLメールの表示ができなくなる等の制約があるため、ここでは((<header.xml|URL:HeaderXml.html>))の中にマクロを埋め込むことにします。以下のような部分を見つけ、

 <line class="mail|news">
  <static width="auto" style="bold" showAlways="true">件名:</static>
  <edit number="6">{@Subject()}</edit>
 </line>

以下のように書き換えます。

 <line class="mail|news">
  <static width="auto" style="bold" showAlways="true">件名:</static>
  <edit number="6">{
  @Progn(@If(@And(@Not(@User1()), Disposition-Notification-To),
             @Progn(@User1(@True()),
                    @If(@Equal(@MessageBox(@Concat(Disposition-Notification-To,
                                                  'に開封通知を送りますか?'),
                                           4),
                               6),
                        @InvokeAction('MessageCreate', 'notification', '', @URI()),
                        @True())),
             @True()),
         @Subject())
  }</edit>
 </line>

マクロの中でDisposition-Notification-Toが付いているかどうか、User1フラグが立っているかどうかを調べ、開封通知を送る必要があればメッセージボックスを表示して確認してから((<@InvokeAction|URL:InvokeActionFunction.html>))を使用してnotification.templateを使ってメッセージを作成しています。

notification.templateは以下のようなテンプレートを用意して、templates\mailの下に置きます。

 To: {Disposition-Notification-To}
 Subject: 開封通知: {Subject}
 X-QMAIL-Account: {@Account()}{
   @If(@Identity(),
       @Concat('\nX-QMAIL-SubAccount: ', @SubAccount()),
       '')
 }
 X-QMAIL-EditMacro: @InvokeAction('FileSendNow')
 
 読みましたよ～。

メッセージの内容は適当に書き換えてください。これによって、開封通知として送るメッセージを作成します。ここで、((<X-QMAIL-EditMacro|URL:SpecialHeaders.html>))を使って、エディットウィンドウが開く直前にマクロを実行しています。その中で((<FileSendNowアクション|URL:FileSendNowAction.html>))を使って、作成したメッセージを実際に送信します。

=end

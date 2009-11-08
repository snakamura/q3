=begin
=送信してもメッセージが送信されません

QMAIL3ではエディットウィンドウで、メニューから((<[ファイル]-[送信]|URL:FileSendAction.html>))を選択したり、ツールバーの((<[送信]|URL:FileSendAction.html>))をクリックしても、作成したメッセージが送信箱に保存されるだけで、すぐにメッセージがサーバに送信されるわけではありません。送信箱に保存されたメッセージは次回、送受信や巡回などを行ったときにサーバに送信されます。メインウィンドウで((<[ツール]-[送信]|URL:ToolSendAction.html>))を選択すると送信箱に保存されたメッセージをサーバに送信することができます。

また、エディットウィンドウで((<[ファイル]-[すぐに送信]|URL:FileSendNowAction.html>))を選択すると送信箱にメッセージを保存した後ですぐにそのメッセージをサーバに送信することができます。

ツールバーの[送信]ボタンをクリックしたときにすぐに送信するようにするためには、((<toolbars.xml|URL:ToolbarsXml.html>))を書き換えます。

 <button image="15" action="FileSend" text="Send"/>

という行を探して、以下のように書き換えます。

 <button image="15" action="FileSendNow" text="Send"/>

詳しくは、((<ツールバーのカスタマイズ|URL:CustomizeToolbars.html>))を参照してください。


=end

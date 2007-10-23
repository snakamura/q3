=begin
=@MessageId

 String @MessageId()


==説明
コンテキストメッセージのMessage-Idを返します。存在しない場合やパースに失敗した場合には空文字列を返します。(({Message-Id}))を使用すると、コンテキストメッセージのMessage-Idが不正な場合でも文字列として取得できてしまいますが、(({@MessageId()}))を使用すれば、そのような場合には空文字列になります。


==引数
なし


==エラー
*引数の数が合っていない場合


==条件
なし


==例
 # Message-Idを取得
 @MessageId()

=end

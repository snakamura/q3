=begin
=@ForEach

 Boolean @ForEach(MessageList messages, Expr expr)


==説明
messagesで指定された各メッセージをコンテキストメッセージとして、exprで指定された式を繰り返し評価します。常にTrueを返します。


==引数
:MessageList messages
  対象のメッセージリスト
:Expr expr
  評価する式


==エラー
*引数の数が合っていない場合
*引数の型が合っていない場合
*式を評価中にエラーが発生した場合


==条件
なし


==例
 # 受信箱内のすべてのメッセージを既読にする
 @ForEach(@Messages('受信箱'), @Seen(@True()))
 
 # コンテキストメッセージが属するスレッドのメッセージ全てをマークする
 @ForEach(@Thread(), @Marked(@True()))

=end

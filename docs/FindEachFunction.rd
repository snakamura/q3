=begin
=@FindEach

 Value @FindEach(MessageList messages, Expr condition, Expr expr?)


==説明
messagesで指定された各メッセージをコンテキストメッセージとして、conditionで指定された式を評価し、結果がTrueになったメッセージをコンテキストメッセージとしてexprを評価して返します。exprが指定されなかった場合には最後に評価してconditionの結果をそのまま返します。全てのメッセージに対してconditionを評価した値がFalseになった場合には、Falseを返します。


==引数
:MessageList messages
  対象のメッセージリスト
:Expr condition
  条件式
:Expr expr
  評価する式


==エラー
*引数の数が合っていない場合
*引数の型が合っていない場合
*式を評価中にエラーが発生した場合


==条件
なし


==例
 # コンテキストメッセージが所属するスレッドの先頭のメッセージのMessage-Idを取得
 @FindEach(@Thread(), @True(), Message-Id)
 
 # コンテキストメッセージが所属するスレッドに自分が送信したメッセージがあるかどうかを調べる
 @FindEach(@Thread(), @Equal(@Address(From), @Address(@I())))
 
 # コンテキストメッセージが所属するスレッドの中で先頭の未読のメッセージのSubjectを取得
 @FindEach(@Thread(), @Not(@Seen()), Subject)

=end

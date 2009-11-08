=begin
=@References

 String @References(Number size?)


==説明
コンテキストメッセージのReferencesを返します。sizeが指定された場合には、後から最大でその個数分だけを返します。


==引数
:Number size
  最大取得数


==エラー
*引数の数が合っていない場合
*コンテキストメッセージがない場合
*メッセージの取得に失敗した場合


==条件
なし


==例
 # 最大4個のReferencesを取得
 @References(4)

=end

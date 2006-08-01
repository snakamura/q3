=begin
=@Delete

 Boolean @Delete(Boolean direct?)


==説明
コンテキストメッセージを削除します。常にTrueを返します。

directにTrueを指定するとゴミ箱に入らず直接削除されます。Falseを指定した場合、または指定しなかった場合には、削除されたメッセージがゴミ箱に移動されるかは、((<EditDeleteアクション|URL:EditDeleteAction.html>))と同様に動作します。


==引数
:Boolean direct
  ゴミ箱を使わずに直接削除するかどうか


==エラー
*引数の数が合っていない場合
*コンテキストメッセージがない場合
*削除できない場合


==条件
*メッセージに対する変更が可能


==例
 # メッセージを削除
 @Delete()
 
 # メッセージを直接削除
 @Delete(@True())

=end

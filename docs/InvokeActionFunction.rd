=begin
=@InvokeAction

 Boolean @InvokeAction(String name, String arg+)


==説明
nameで指定されたアクションを起動します。それ以降の引数はアクションの引数としてアクションに渡されます。常にTrueを返します。

この関数は任意のアクションを起動することができるため、使用するには注意が必要です。たとえば、EditDeleteアクションを起動して処理中のメッセージを削除するとクラッシュすることがあります。


==引数
:String name
  アクションの名前
:arg
  アクションに渡す引数


==エラー
*引数の数が合っていない場合
*UIスレッド以外から呼び出した場合
*UIがない場合
*アクションを起動できない場合


==条件
*UIスレッドからのみ呼び出し可能
*UIが必要


==例
 # MessageCreateアクションでnew.templateを使って新規メッセージを作成する
 @InvokeAction('MessageCreate', 'new')

=end

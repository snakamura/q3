=begin
=@Exist

 Boolean @Exist(String name)


==説明
ヘッダに指定された名前のフィールドがあるかどうかを返します。

フィールドをBooleanに変換するとフィールドの存在が調べられるので以下の三つは同じ意味になります。

 @If(@Exist('X-ML-Name'), 1, 2)
 @If(X-ML-Name, 1, 2)
 @If(@Field('X-ML-Name'), 1, 2)


==引数
:String name
  フィールド名


==エラー
*引数の数が合っていない場合
*コンテキストメッセージがない場合
*メッセージの取得に失敗した場合


==条件
なし


==例
 # X-ML-Nameがあるかどうか調べる
 @Exist('X-ML-Name')

=end

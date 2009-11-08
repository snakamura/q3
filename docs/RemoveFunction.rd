=begin
=@Remove

 String @Remove(String address, String remove+)


==説明
addressで指定されたアドレスのリストから、removeで指定されたアドレスを削除した結果のアドレスのリストを返します。


==引数
:String address
  アドレスのリスト
:String remove
  削除するアドレス


==エラー
*引数の数が合っていない場合
*アドレスのパースに失敗した場合


==条件
なし


==例
 # Ccに指定されたアドレスから自分のアドレスを削除
 @Remove(Cc, @Address(@I()))
 
 # Toに指定されたアドレスから自分のアドレスとFromに指定されたアドレスを削除
 @Remove(To, @Address(@I()), @Address(From))

=end

=begin
=@MessageBox

 Number @MessageBox(String message, Number type?)


==説明
messageに指定されたメッセージを表示するメッセージボックスを表示します。typeにはメッセージボックスのタイプを指定します。メッセージボックスのタイプは、((<MessageBox|URL:http://msdn.microsoft.com/library/en-us/winui/winui/windowsuserinterface/windowing/dialogboxes/dialogboxreference/dialogboxfunctions/messagebox.asp?frame=true>)) APIのuTypeに指定できる値を指定できます。指定されなかった場合には、MB_OK | MB_ICONINFORMATIONが指定されたのと同じになります。メッセージボックスを表示した後で、MessageBox APIの返り値を返します。


==引数
:String message
  メッセージ
:Number type?
  タイプ


==エラー
*引数の数が合っていない場合
*UIがない場合


==条件
*UIが必要


==例
 # メッセージボックスを表示
 @MessageBox('Test')
 
 # メッセージボックスに本文を表示
 @MessageBox(@Body(:BODY-INLINE))
 
 # Yes/Noを尋ねるメッセージボックスを表示して処理を分ける
 @If(@Equal(@MessageBox('処理を続けますか?', 292), 6),
     @MessageBox('ごにょごにょ'),
     @Exit())

=end

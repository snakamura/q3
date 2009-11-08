=begin
=@MessageCount

 Number @MessageCount(String folder?)


==説明
メッセージ数を返します。folderが指定された場合にはそのフォルダ内のメッセージ数を返します。指定されなかった場合にはアカウント中のすべてのメッセージ数を返します。


==引数
:String folder
  フォルダの完全名


==エラー
*引数の数が合っていない場合
*指定されたフォルダが存在しない場合


==条件
なし


==例
 # アカウント中のメッセージ数を取得する
 @MessageCount()
 
 # カレントフォルダのメッセージ数を取得する
 @MessageCount(@Folder(:FN-FULLNAME, :FT-CURRENT))

=end

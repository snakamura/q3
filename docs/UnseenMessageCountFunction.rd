=begin
=@UnseenMessageCount

 Number @UnseenMessageCount(String folder?)


==説明
未読メッセージ数を返します。folderが指定された場合にはそのフォルダ内の未読メッセージ数を返します。指定されなかった場合にはアカウント中のすべての未読メッセージ数を返します。


==引数
:String folder
  フォルダの完全名


==エラー
*引数の数が合っていない場合
*指定されたフォルダが存在しない場合


==条件
なし


==例
 # アカウント中の未読メッセージ数を取得する
 @UnseenMessageCount()
 
 # カレントフォルダの未読メッセージ数を取得する
 @UnseenMessageCount(@Folder(:FN-FULLNAME, :FT-CURRENT))

=end

=begin
=@FolderParameter

 String @FolderParameter(String folder, String name)


==説明
folderで指定されたフォルダのnameで指定されたパラメータの値を返します。

folderにはフラグを調べたいフォルダの完全名を指定します。指定されたフォルダが見つからないとエラーになります。nameには取得するパラメータの名前を指定します。指定されたパラメータが設定されていなかった場合には空文字列を返します。


==引数
:String folder
  フォルダ名
:String name
  パラメータ名


==エラー
*引数の数が合っていない場合
*指定されたフォルダが見つからない場合


==条件
なし


==例
 # 現在のフォルダのToパラメータを取得
 @FolderParameter(@Folder(:FN-FULLNAME, :FT-CURRENT), 'To')

=end

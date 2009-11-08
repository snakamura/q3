=begin
=@AccountDirectory

 String @AccountDirectory(String account?)


==説明
accountで指定されたアカウントのディレクトリを返します。指定された名前のアカウントが見つからない場合にはエラーになります。引数が省略された場合には、コンテキストアカウントのディレクトリを返します。コンテキストアカウントがない場合にはエラーになります。

アカウントのディレクトリとは、そのアカウントに関するファイルが保存されるディレクトリです。


==引数
:String account
  アカウント名


==エラー
*引数の数が合っていない場合
*指定された名前のアカウントが見つからない場合
*コンテキストアカウントがない場合


==条件
なし


==例
 # コンテキストアカウントのディレクトリを取得
 # -> C:\Documents and Settings\username\Application Data\QMAIL3\accounts\Sub
 @AccountDirectory()
 
 # Mainという名前のアカウントのディレクトリを取得
 # -> C:\Documents and Settings\username\Application Data\QMAIL3\accounts\Main
 @AccountDirectory('Main')

=end

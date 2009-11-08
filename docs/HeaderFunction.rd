=begin
=@Header

 String @Header(String remove?, Part part?)


==説明
コンテキストメッセージのヘッダを返します。partが指定された場合にはそのパートのヘッダを返します。

removeには返されるヘッダから除外したいヘッダの名前を「,」区切りで指定します。


==引数
:String remove
  除外するヘッダのリスト
:Part part
  パート


==エラー
*引数の数が合っていない場合
*コンテキストメッセージがない場合
*メッセージの取得に失敗した場合
*指定したパートがない場合（partを指定した場合）


==条件
なし


==例
 # ヘッダを全て取得
 @Header()
 
 # ToとMessage-Idを除外してヘッダを取得
 @Header('To,Message-Id')
 
 # マルチパートメッセージではじめのパートのヘッダを取得
 @Header('', @Part(0))

=end

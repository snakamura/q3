=begin
=@Field

 Field @Field(String name, Part part?)


==説明
コンテキストメッセージの指定された名前のヘッダを返します。partが指定された場合にはそのパートの指定した名前のヘッダを返します。


==引数
:String name
  ヘッダ名
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
 # Toを取得
 @Field('To')
 
 # マルチパートメッセージではじめのパートのContent-IDを取得
 @Field('Content-ID', @Part(0))

=end

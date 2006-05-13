=begin
=@FieldParameter

 String @FieldParameter(String name, String paramName?, Part part?)


==説明
コンテキストメッセージの指定された名前のヘッダの指定された名前のパラメータの値を返します。partが指定された場合にはそのパートの指定した名前のヘッダの指定された名前のパラメータの値を返します。指定された名前のヘッダやパラメータが見つからない場合には空文字列を返します。指定された名前のヘッダが複数ある場合には先頭のヘッダが使用されます。

paramNameが空文字列か省略された場合には、パラメータでない部分を返します。たとえば、

 Content-Type: text/plain; charset=iso-2022-jp

というようなヘッダの場合、@FieldParameter('Content-Type')は「text/plain」を、@FieldParameter('Content-Type', 'charset')は「iso-2022-jp」を返します。


==引数
:String name
  ヘッダ名
:String paramName
  パラメータ名
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
 # Content-Typeの値を取得
 @FieldParameter('Content-Type')
 
 # Content-Typeのcharset値を取得
 @FieldParameter('Content-Type', 'charset')
 
 # マルチパートメッセージではじめのパートのContent-Dispositionのfilenameパラメータを取得
 @Field('Content-Disposition', 'filename', @Part(0))

=end

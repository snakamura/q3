=begin
=@Attachment

 String @Attachment(String separator?, Boolean uri?)


==説明
コンテキストメッセージの添付ファイルの名前を返します。複数の添付ファイルがある場合には、separatorで指定した文字列で区切られます。separatorを指定しなかった場合には、「, 」で区切られます。uriにTrueを指定すると名前の代わりにURIが返されます。


==引数
:String separator
  セパレータ
:Boolean uri
  URIを返すかどうか


==エラー
*引数の数が合っていない場合
*コンテキストメッセージがない場合
*コンテキストメッセージが一時的な場合（URIを取得する場合）
*メッセージの取得に失敗した場合


==条件
なし


==例
 # 添付ファイルの名前を取得
 @Attachment()
 
 # 添付ファイルのURIを「,」で区切って取得
 @Attachment(',', @True())

=end

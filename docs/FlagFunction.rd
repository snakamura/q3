=begin
=@Flag

 Boolean @Flag(Number flag, Boolean value?)


==説明
valueが指定されていない場合には、コンテキストメッセージのフラグを取得します。指定されている場合には、フラグを設定します。

この関数の代わりに以下の関数を使用してください。
*((<@Seen|URL:SeenFunction.html>))
*((<@Replied|URL:RepliedFunction.html>))
*((<@Forwarded|URL:ForwardedFunction.html>))
*((<@Sent|URL:SentFunction.html>))
*((<@Draft|URL:DraftFunction.html>))
*((<@Marked|URL:MarkedFunction.html>))
*((<@Deleted|URL:DeletedFunction.html>))
*((<@Download|URL:DownloadFunction.html>))
*((<@DownloadText|URL:DownloadTextFunction.html>))
*((<@Multipart|URL:MultipartFunction.html>))
*((<@Partial|URL:PartialFunction.html>))
*((<@User1|URL:User1Function.html>))
*((<@User2|URL:User2Function.html>))
*((<@User3|URL:User3Function.html>))
*((<@User4|URL:User4Function.html>))



==引数
:Number flag
  フラグ
:Boolean value
  設定する場合にフラグを立てるか倒すか


==エラー
*引数の数が合っていない場合
*コンテキストメッセージホルダがない場合
*コンテキストメッセージが一時的な場合（フラグを設定する場合）
*フラグの設定に失敗した場合（フラグを設定する場合）


==条件
なし


==例
 # 0x00000001のフラグを取得
 @Flag(1)

=end

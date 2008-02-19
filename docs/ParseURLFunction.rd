=begin
=@ParseURL

 String @ParseURL(String url)


==説明
urlで指定されたmailto URLをパースしてヘッダ形式の文字列（とあれば本文）を返します。

たとえば、<mailto:test@example.org>からは、

 To: test@example.org

を、<mailto:test@example.org?Cc=test2@example.org?Subject=Test&Body=Test%20Body>からは、

 To: test@example.org
 Cc: test2@example.org
 Subject: Test
 
 Test Body

を返します。処理されるヘッダは、To, Cc, Subject, In-Reply-To, Referencesとbodyです。

URL中の非ASCII文字列（%xxでエスケープされたものも含む）はUTF-8として正しいバイト列であるときにはUTF-8として、正しくないバイト列であるときにはプラットフォームのデフォルトエンコーディングとして処理されます。

urlで指定された文字列がmailto:から始まっていなかった場合には、Toとして指定された文字列を持ったヘッダ形式の文字列を返します。たとえば、test3@example.orgを引数として渡すと、

 To: test3@example.org

を返します。


==引数
:String url
  パースするmailto URL


==エラー
*引数の数が合っていない場合


==条件
なし


==例
 # urlという変数に設定されたURLをパース
 @ParseUrl($url)

=end

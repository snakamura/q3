=begin
=@Body

 String @Body(String quote?, Number type?, Part part?)


==説明
コンテキストメッセージの本文を返します。partが指定された場合にはそのパートの本文を返します。

quoteには引用符を指定します。空文字列以外を指定すると指定された引用符で引用されます。

typeには本文のフォーマット方法を指定します。指定できるのは以下のいずれかです。

::BODY-ALL
  本文全てを返します。マルチパートの場合にはパートを展開して返します。
::BODY-RFC822INLINE
  :BODY-INLINEと同じですが、message/rfc822のパートはContent-Dispositionに関わらず返されます。
::BODY-INLINE
  インラインの本文を返します。インラインの本文とは、Content-Dispositionが指定されていないかまたはinlineかつ、Content-Typeがtext/*またはmessage/rfc822のものです。

引数が省略された場合には:BODY-ALLを指定したのと同じになります。

==引数
:String quote
  引用符
:Number type
  フォーマット方法
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
 # 本文のエンコーディングを取得
 @BodyCharset()
 
 # インラインの本文のエンコーディングを取得
 @BodyCharset(:BODY-INLINE)

=end

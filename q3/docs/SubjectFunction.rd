=begin
=@Subject

 String @Subject(Boolean removeRe?, Boolean removeMl?)


==説明
コンテキストメッセージのSubjectを返します。取得できなかった場合には空文字列を返します。

removeReにTrueを指定すると先頭の「Re:」に類するものを取り除きます。removeMlにTrueを指定すると先頭のML番号（[qs:00123]のような文字列）を取り除きます。これらを指定しない場合には、Falseを指定したのと同じになります。

@Subject()はSubjectと同じ結果を返します。


==引数
:Boolean removeRe
  先頭のRe:を取り除くかどうか
:Boolean removeMl
  先頭のML番号を取り除くかどうか


==エラー
*引数の数が合っていない場合
*コンテキストメッセージがない場合
*メッセージの取得に失敗した場合


==条件
なし


==例
 # Re:とML番号を取り除いてSubjectを取得
 @Subject(@True(), @True())

=end

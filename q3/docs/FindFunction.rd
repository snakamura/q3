=begin
=@Find

 Number @Find(String s1, String s2, Number index?, Boolean case?)


==説明
s1の中で最初にs2が現れるインデックスを返します。indexが指定された場合には、指定された文字数目から検索を始めます。caseにTrueを指定すると大文字と小文字を区別し、Falseの場合や省略された場合には区別しません。インデックスは0ベースです。


==引数
:String s1
  文字列
:String s2
  検索する文字列
:Number index
  検索を始めるインデックス
:Boolean case
  大文字と小文字を区別する場合にはTrue、それ以外の場合にはFalse


==エラー
*引数の数が合っていない場合


==条件
なし


==例
 # 文字を検索
 # -> 2
 @Find('abcabcabc', 'c')
 
 # インデックスを指定
 # -> 5
 @Find('abcabcabc', 'Ca', 3)

=end

=begin
=@Length

 Number @Length(String s, Boolean byte?)


==説明
指定された文字列の長さを返します。byteにTrueを指定すると、プラットフォームのエンコーディング((-日本語プラットフォームの場合にはCP932-))に変換したときのバイト数を返します。


==引数
:String s
  文字列
:Boolean byte
  バイト列の長さを返すかどうか


==エラー
*引数の数が合っていない場合


==条件
なし


==例
 # 文字列の長さを取得
 # -> 3
 @Length('abc')
 
 # バイト数を取得
 # -> 6（日本語環境の場合）
 @Length('日本語', @True())

=end

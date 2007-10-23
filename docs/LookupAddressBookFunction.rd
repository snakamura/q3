=begin
=@LookupAddressBook

 String @LookupAddressBook(String address, Number type)


==説明
指定されたアドレスをアドレス帳から逆引きします。

typeには逆引きする値を指定します。以下のいずれかが指定できます。

::ADDRESS-NAME
  名前
::ADDRESS-SORTKEY
  ソートキー
::ADDRESS-ADDRESS
  アドレス
::ADDRESS-ALIAS
  エイリアス
::ADDRESS-CATEGORY
  カテゴリ
::ADDRESS-COMMENT
  コメント

指定しなかった場合には、:ADDRESS-NAMEを指定したのと同じになります。指定したアドレスのエントリが見つからなかった場合には空文字列を返します。

:ADDRESS-ADDRESSは指定したアドレスと同じ値を返します（大文字・小文字は変わる可能性があります）。このため、:ADDRESS-ADDRESSを指定して逆引きして空文字列かどうか調べることでアドレス帳にエントリがあるかどうかを調べることができます。


==引数
:Number ltype
  逆引きタイプ


==エラー
*引数の数が合っていない場合


==条件
なし


==例
 # 名前を逆引き
 @LookupAddressBook($address)
 
 # コメントを逆引き
 @LookupAddressBook($address, :ADDRESS-COMMENT)
 
 # アドレス帳にエントリがあるかどうかを調べる
 @If(@LookupAddressBook($address, :ADDRESS-ADDRESS),
     'アドレス帳にエントリあり',
     'アドレス帳にエントリなし')
=end

=begin
=@FormatAddress

 String @FormatAddress(Field field, Number type?, Number lookup?)


==説明
引数で指定されたフィールドをアドレスリストとしてパースし、その結果をフォーマットした文字列を返します。パースに失敗した場合には空文字列を返します。

typeに指定した値で各アドレスのフォーマット方法が代わります。以下のいずれかが指定できます。

::FORMAT-ALL
  名前 <アドレス>
::FORMAT-ADDRESS
  アドレス
::FORMAT-NAME
  名前（名前が指定されたいない場合にはアドレス）
::FORMAT-VIEW
  名前 <アドレス>（ただし、名前部分をエスケープしない）

指定しなかった場合には:FORMAT-ALLを指定したのと同じになります。:FORMAT-ALLと:FORMAT-VIEWは似ていますが、必要なエスケープをするかどうかが異なります。たとえば、

 To: "Test (Test)" <test@example.org>, "Yamada, Taro" <test2@example.org>

というヘッダを持つメッセージに、@FormatAddress(To, :FORMAT-ALL)を適用すると「"Test (Test)" <test@example.org>, "Yamada, Taro" <test2@example.org>」が返されますが、@FormatAddress(To, :FORMAT-VIEW)を適用すると「Test (Test) <test@example.org>, Yamada, Taro <test2@example.org>」が返されます。このように、:FORMAT-VIEWを指定すると見た目重視でフォーマットされるため、その文字列をパースすることができなくなる可能性があります。

lookupに指定した値で使用する名前をアドレス帳から逆引きするかどうかを指定します。以下のいずれかが指定できます。

::LOOKUP-NONE
  逆引きをしません
::LOOKUP-EMPTY
  名前が指定されていない場合のみ逆引きをします
::LOOKUP-FORCE
  常に逆引きをします

逆引きをした場合には、フォーマット時に名前の部分をアドレス帳から逆引きした名前で置き換えます。アドレス帳に逆引きしたアドレスが含まれていない場合には置き換えは行われません。また、同じアドレスが複数のエントリに表れる場合には始めに見つかったエントリが使われます。lookupを指定しなかった場合には:LOOKUP-NONEを指定したのと同じになります。


==引数
:Field field
  フォーマットするフィールド
:Number type
  フォーマットの形式
:Number lookup
  アドレス帳の逆引き方法


==エラー
*引数の数が合っていない場合
*引数の型が合っていない場合


==条件
なし


==例
 # Fromを表示用に逆引きしてフォーマット
 @FormatAddress(From, :FORMAT-VIEW, :LOOKUP-FORCE)
 
 # Toをフォーマット
 @FormatAddress(To)

=end

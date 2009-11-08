=begin
=@Address

 Address @Address(Field field)


==説明
引数で指定されたフィールドをアドレスリストとしてパースし、含まれているアドレスのリストを返します。フィールドがアドレスを指定するフィールドでなかった場合やパースに失敗した場合には空のリストを返します。

パースされた各アドレスに対してアドレスを取得します。グループアドレスの場合には含まれるアドレスを全て取り出します。

たとえば、

 To: test1@example.org,
  Test2 <test2@example.org>,
  Test3: test3@example.org, Test4 <test4@example.org>;

というヘッダを含むメッセージに対して、@Address(To)を適用すると、test1@example.org, test2@example.org, test3@example.org, test4@example.orgを含むアドレスのリストが返されます。


==引数
:Field field
  アドレスのリストを取得するフィールド


==エラー
*引数の数が合っていない場合
*引数の型が合っていない場合


==条件
なし


==例
 # Toで指定されたアドレスのリストを取得
 @Address(To)

=end

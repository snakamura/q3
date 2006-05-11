=begin
=@Folder

 String @Folder(Boolean full?)


==説明
コンテキストメッセージが保存されているフォルダの名前を返します。コンテキストメッセージがない場合には、コンテキストフォルダの名前を返します。fullにTrueが指定された場合、または引数が省略された場合には完全名が返されます。それ以外の場合には単一名が返されます。

例えば、「受信箱」の下にある「テスト」フォルダにメッセージが保存されている場合には、完全名は「受信箱/テスト」になり、単一名は「テスト」になります。


==引数
:Boolean full
  完全名を取得するかどうか


==エラー
*引数の数が合っていない場合
*コンテキストメッセージもコンテキストフォルダもない場合


==条件
なし


==例
 # フォルダ名を完全名で取得
 @Folder()
 
 # 単一名で取得
 @Folder(@False())

=end

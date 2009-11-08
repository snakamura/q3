=begin
=@Folder

 String @Folder(Boolean full?, Boolean current?)


==説明
コンテキストメッセージが保存されているフォルダ、またはコンテキストフォルダの名前を返します。

fullには完全名を返すか単一名を返すかを指定します。指定できるのは以下のいずれかです。

::FN-FULLNAME
  完全名を返します。
::FN-NAME
  単一名を返します。

引数が省略された場合には:FN-FULLNAMEを指定したのと同じになります。

例えば、「受信箱」の下にある「テスト」フォルダにメッセージが保存されている場合には、完全名は「受信箱/テスト」になり、単一名は「テスト」になります。

currentにはフォルダ名の取得方法を指定します。指定できるのは以下のいずれかです。

::FT-MESSAGE
  コンテキストメッセージが格納されているフォルダを返します。
::FT-CURRENT
  コンテキストフォルダを返します。

引数が省略された場合には:FT-MESSAGEを指定したのと同じになります。

例えば、検索フォルダを開いていた場合、:FT-MESSAGEを指定するとメッセージが格納されているフォルダの名前が返され、:FT-CURRENTを指定すると検索フォルダの名前が返されます。

:FT-MESSAGEを指定した場合にコンテキストメッセージホルダがない、または:FT-CURRENTを指定した場合にコンテキストフォルダがない（アカウントが選択されている）ときには空文字列を返します。


==引数
:Boolean full
  完全名を取得するかどうか
:Boolean current
  フォルダ名の取得方法


==エラー
*引数の数が合っていない場合
*コンテキストメッセージホルダがない場合


==条件
なし


==例
 # フォルダ名を完全名で取得
 @Folder()
 
 # 単一名で取得
 @Folder(:FN-NAME)
 
 # 現在のフォルダ名を完全名で取得
 @Folder(:FN-FULLNAME, :FT-CURRENT)

=end

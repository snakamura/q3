=begin
=@InputBox

 String @InputBox(String message, Boolean multiline?, String default?)


==説明
ユーザに入力を求めるためのダイアログを表示し、入力された文字列を返します。messageには表示するメッセージを指定します。defaultを指定するとダイアログを開いたときにその文字列が入力欄に表示されます。

multilineにはダイアログのタイプを指定します。指定できるのは以下のとおりです。

::INPUT-SINGLELINE
  単数行の入力を求めるダイアログ
  
  ((<単数行入力ダイアログ|"IMG:images/SingleLineInputBoxDialog.png">))
::INPUT-MULTILINE
  複数行の入力を求めるダイアログ
  
  ((<複数行入力ダイアログ|"IMG:images/MultiLineInputBoxDialog.png">))

指定しない場合には、:INPUT-SINGLELINEを指定したのと同じになります。


==引数
:String message
  表示するメッセージ
:Boolean multiline
  ダイアログのタイプ
:String default
  デフォルトの入力文字列


==エラー
*引数の数が合っていない場合
*UIがない場合

==条件
*UIが必要


==例
 # 単数行
 @InputBox('入力してください')
 
 # 単数行でデフォルトを指定
 @InputBox('入力してください', :INPUT-SINGLELINE, 'テスト')
 
 # 複数行
 @InputBox('入力してください', :INPUT-MULTILINE)

=end

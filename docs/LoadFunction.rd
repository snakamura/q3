=begin
=@Load

 String @Load(String path, Boolean template?, String encoding?)


==説明
指定されたパスのファイルから読み込んだ内容を返します。パスに相対パスが指定された場合にはメールボックスディレクトリからの相対パスになります。

templateにTrueを指定すると読み込んだ内容をテンプレートとして処理し、処理結果を返します。

encodingを指定すると指定されたエンコーディングで読み込みます。指定されない場合にはシステムのデフォルトのエンコーディングで読み込みます。


==引数
:String path
  ファイルパス
:Boolean template
  テンプレートとして読み込むかどうか
:String encoding
  エンコーディング


==エラー
*引数の数が合っていない場合
*読み込みに失敗した場合
*テンプレートの処理に失敗した場合（テンプレートとして読み込んだ場合）


==条件
なし


==例
 # ファイルを読み込む
 @Load('C:\\Temp\\Test.txt')
 
 # 相対パスでエンコーディングを指定
 @Load('profiles/qmail.xml', @False(), 'utf-8')
 
 # テンプレートを処理する
 @Load('templates/mail/test.template', @True())

=end

=begin
=@Include

 Value @Include(String path)


==説明
指定されたパスのファイルから読み込んだマクロを評価して結果を返します。つまり、@Eval(@Load(path))と同じです。パスに相対パスが指定された場合にはメールボックスディレクトリからの相対パスになります。ファイルはシステムのエンコーディングでエンコードされている必要があります。


==引数
:String path
  ファイルパス


==エラー
*引数の数が合っていない場合
*読み込みに失敗した場合
*マクロのパース・実行に失敗した場合


==条件
なし


==例
 # 外部ファイルにあるマクロを実行
 @Include('test.mac');

=end

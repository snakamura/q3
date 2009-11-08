=begin
=@Selected

 MessageList @Selected()


==説明
選択されているメッセージのリストを返します。


==引数
なし


==エラー
*引数の数が合っていない場合


==条件
なし


==例
 # 選択されているメッセージ全てにラベルを設定する
 @ForEach(@Selected(), @Label('qmail'))

=end

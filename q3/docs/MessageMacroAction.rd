=begin
=MessageMacroアクション

引数で指定されたマクロを実行します。引数を指定しなかった場合には、((<[マクロの実行]ダイアログ|URL:MacroDialog.html>))を開くので、実行するマクロを指定します。

リストビューにフォーカスがある場合には、選択されている各メッセージをコンテキストメッセージとしてマクロが評価されます。つまり、選択されているメッセージの回数だけマクロが実行されます。選択されているメッセージがない場合には何もしません。

プレビューやメッセージビューにフォーカスがある場合には、表示しているメッセージをコンテキストメッセージとして一回だけマクロが評価されます。メッセージが表示されていない場合には何もしません。

フォルダビューやフォルダリストビュー、フォルダコンボボックスにフォーカスがあるときには、選択されている各フォルダに対して一回ずつ、コンテキストメッセージがない状態でマクロが評価されます。フォルダリストビューで複数のフォルダを選択している場合には、選択されているフォルダの回数だけマクロが実行されます。


==引数
:1
  実行するマクロ


==有効なウィンドウ・ビュー
*メインウィンドウ
*メッセージウィンドウ

=end

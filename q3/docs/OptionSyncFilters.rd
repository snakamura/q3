=begin
=同期フィルタの設定

[オプション]ダイアログの[同期フィルタ]パネルでは((<同期フィルタ|URL:SyncFilter.html>))の設定を行います。

((<同期フィルタの設定|"IMG:images/OptionSyncFilters.png">))


+[追加]
同期フィルタセットを追加します。[同期フィルタ]ダイアログが開きます。


+[削除]
選択された同期フィルタセットを削除します。


+[編集]
選択された同期フィルタセットを編集します。[同期フィルタ]ダイアログが開きます。


+[上へ]
選択された同期フィルタセットをひとつ上に移動します。


+[下へ]
選択された同期フィルタセットをひとつ下に移動します。


===[同期フィルタ]ダイアログ
[同期フィルタ]ダイアログでは、同期フィルタセットの編集を行います。メインのリストには同期フィルタセットに含まれる同期フィルタのリストが表示されます。各同期フィルタには条件と動作が設定されていて、条件にマッチしたメッセージに対してその動作が実行されます。

((<[同期フィルタ]ダイアログ|"IMG:images/SyncFiltersDialog.png">))


+[追加]
同期フィルタを追加します。[同期フィルタ]ダイアログが開きます。


+[削除]
選択された同期フィルタを削除します。


+[編集]
選択された同期フィルタを編集します。[同期フィルタ]ダイアログが開きます。


+[上へ]
選択された同期フィルタをひとつ上に移動します。


+[下へ]
選択された同期フィルタをひとつ下に移動します。


===[同期フィルタ]ダイアログ
[同期フィルタ]では個々の同期フィルタについて、その同期フィルタが使われる条件とその動作を編集します。

((<[同期フィルタ]ダイアログ|"IMG:images/SyncFilterDialog.png">))


+[条件]
同期フィルタの条件を((<マクロ|URL:Macro.html>))で指定します。対象のメッセージに対して指定されたマクロを評価した結果がTrueになると、この同期フィルタで設定した動作が実行されます。


+[編集]
条件を((<条件エディタ|URL:ConditionEditor.html>))で編集します。


+[フォルダ]
同期フィルタを有効にするフォルダを指定します。指定しない場合には全てのフォルダに対して有効になります。フォルダはフォルダの完全名または正規表現で指定します。正規表現で指定する場合には、//で括って指定します。この場合、完全名がその正規表現にマッチするフォルダで有効になります。


+[動作]
動作を指定します。指定できるのは以下のいずれかです。

:ダウンロード (POP3)
  メッセージをダウンロードします。[最大行数]でダウンロードする最大行数を指定できます。[最大行数]に-1を指定するとメッセージ全体をダウンロードします。
:ダウンロード (IMAP4)
  メッセージをダウンロードします。[タイプ]でダウンロードする方法を指定できます。指定できるのは以下のいずれかです。
  :全て
    メッセージ全体をダウンロードします。
  :テキスト
    メッセージのうちテキスト部分をダウンロードし、添付ファイル部分はダウンロードしません。
  :HTML
    メッセージがHTMLメッセージだった場合にはHTML部分（HTMLのテキストと埋め込まれている画像など）をダウンロードします。HTMLメッセージでなかった場合には「テキスト」と同じです。
  :ヘッダ
    メッセージのヘッダをダウンロードします。
:ダウンロード (NNTP)
  メッセージをダウンロードします。
:削除 (POP3, IMAP4)
  メッセージをサーバ上から削除します。
:無視 (POP3, NNTP)
  無視します。インデックスも生成しません。


+[説明]
同期フィルタの説明を指定します。説明を指定すると、[同期フィルタ]ダイアログに表示されます。

=end

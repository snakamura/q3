=begin
=同期フィルタ

同期フィルタはサーバからメッセージを受信するときの動作をコントロールするために使用します。例えば、以下のようなことができます。

*POP3であるサイズ以上の大きさのメッセージは最初の100行だけ取り込む
*IMAP4で特定の相手から来たメッセージはテキスト部分だけキャッシュする
*NNTPで件名に特定の文字が含まれているメッセージだけダウンロードする

デフォルトでは以下のような同期フィルタが用意されています。

:ヘッダのみ (POP3)
  POP3でヘッダのみダウンロードします。

:最大100行 (POP3)
  POP3で本文の先頭100行までをダウンロードします。

:ヘッダのみ (IMAP4)
  IMAP4でヘッダのみキャッシュします。

:テキストのみ (IMAP4)
  IMAP4でテキスト部分のみキャッシュします。

:すべて (IMAP4)
  IMAP4でメッセージ全体をキャッシュします。

:すべて (NNTP)
  NNTPでメッセージ全体をキャッシュします。

これらの動作をカスタマイズしたり、特定の条件にマッチするメッセージに対してのみ動作を適用するには、同期フィルタを作成します。同期フィルタを作成するには、((<[ツール]-[オプション]|URL:ToolOptionsAction.html>))を選択して[オプション]ダイアログを開き、[同期フィルタ]パネルを選択します。設定方法については、((<同期フィルタの設定|URL:OptionSyncFilters.html>))を参照してください。

受信時にどの同期フィルタを使用するかは、((<アカウントの設定|URL:AccountProperty.html>))の((<高度の設定|URL:AccountAdvanced.html>))の[同期フィルタ]で指定します。また、((<巡回|URL:GoRound.html>))時には、ここで指定したのとは別の同期フィルタを指定することができます。詳細は、((<巡回の設定|URL:OptionGoRound.html>))を参照してください。

同期フィルタを指定しないと、POP3の場合にはメッセージ全体をダウンロードし、IMAP4とNNTPではインデックスのみ作成します。

=end

=begin
=巡回の設定

[オプション]ダイアログの[巡回]パネルでは((<巡回|URL:GoRound.html>))の設定を行います。

((<巡回の設定|"IMG:images/OptionGoRound.png">))


+[追加]
巡回コースを追加します。[巡回コース]ダイアログが開きます。


+[削除]
選択された巡回コースを削除します。


+[編集]
選択された巡回コースを編集します。[巡回コース]ダイアログが開きます。


+[上へ]
選択された巡回コースをひとつ上に移動します。


+[下へ]
選択された巡回コースをひとつ下に移動します。


===[巡回コース]ダイアログ
[巡回コース]ダイアログでは巡回コースの編集を行います。

((<[巡回コース]ダイアログ|"IMG:images/GoRoundCourseDialog.png">))


+[名前]
巡回コースの名前を指定します。


+[追加]
エントリを追加します。[エントリ]ダイアログが開きます。


+[削除]
選択されたエントリを削除します。


+[編集]
選択されたエントリを編集します。[エントリ]ダイアログが開きます。


+[上へ]
選択されたエントリをひとつ上に移動します。


+[下へ]
選択されたエントリをひとつ下に移動します。


====[タイプ]
巡回コースのタイプを指定します。


+[逐次]
各エントリを逐次巡回します。


+[並列]
各エントリを並列に巡回します。


+[確認する]
巡回する前に確認のダイアログを表示するかどうかを指定します。


+[ダイアルアップ]
[ダイアルアップ]ダイアログを開き、ダイアルアップの設定を行います。


===[エントリ]ダイアログ
巡回コースのエントリの設定を行います。

((<[エントリ]ダイアログ|"IMG:images/GoRoundEntryDialog.png">))


+[アカウント]
アカウントを指定します。


+[サブアカウント]
サブアカウントを指定します。「(未指定)」を指定すると巡回したとき選択されていたサブアカウントを使用します。


+[フォルダ]
フォルダを指定します。「(すべてのフォルダ)」を指定するとすべての同期可能なフォルダが同期されます。フォルダ名を指定するとそのフォルダが同期されます。そのほかに正規表現を指定することもできます。正規表現を指定する場合には、//で括って指定します。この場合、その正規表現にマッチする同期可能なすべてのフォルダが同期されます。


+[フォルダを選択する]
巡回するときに同期するフォルダを選択するかどうかを指定します。チェックを入れるた場合、巡回するたびに同期するフォルダをダイアログで指定します。


+[同期フィルタ]
使用する同期フィルタを指定します。同期フィルタについては、((<同期フィルタ|URL:SyncFilter.html>))を参照してください。


====タイプ
同期のタイプを指定します。


+[送受信]
送信と受信を行います。


+[受信]
受信のみを行います。


+[送信]
送信のみを行います。


+[送信前に受信サーバに接続する]
送信する場合に、送信サーバに接続する前に受信サーバに接続します。POP before SMTPを使用したい場合に使用します。


===[ダイアルアップ]ダイアログ
ダイアルアップの設定を行います。

((<[ダイアルアップ]ダイアログ|"IMG:images/DialupDialog.png">))


+[ダイアルアップ接続をしない]
ダイアルアップしません。


+[ネットワークに接続していないときにダイアルアップ接続する]
ネットワークに接続していないときに限りダイアルアップします。


+[常にダイアルアップ接続する]
常にダイアルアップします。


+[エントリ名]
ダイアルアップするときに使用するエントリ名を指定します。


+[ダイアル元]
ダイアル元を指定します。


+[接続前にダイアログを表示する]
ダイアルアップ接続する前に、ユーザ名などを指定できる[ダイアルアップ接続]ダイアログを表示するかどうかを指定します。


+[?秒待ってから切断する]
送受信終了後にダイアルアップを切断するときに指定した秒数だけ待ってから切断します。必要があればその間に切断をキャンセルすることができます。

=end

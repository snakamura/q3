=begin
=添付ファイル

==添付ファイルの処理
受信したメッセージに添付ファイルが付いているとヘッダビューに表示されます。

((<ヘッダビュー|"IMG:images/HeaderAttachment.png">))


===添付ファイルを保存する
添付ファイルを保存するには、((<[メッセージ]-[添付ファイル]-[保存]|URL:MessageDetachAction.html>))を選択します。[保存]ダイアログが開きますので、保存したいファイルにチェックが入った状態にし、[フォルダ]に保存先のフォルダを指定して[OK]をクリックします。[添付ファイルを保存した後でフォルダを開く]にチェックを入れると保存したフォルダをエクスプローラで開きます。

また、ヘッダビューで保存したい添付ファイルをを右クリックすると((<添付ファイルのコンテキストメニュー|URL:AttachmentMenu.html>))が表示されますので、そこから((<[保存]|URL:AttachmentSaveAction.html>))や((<[すべて保存]|URL:AttachmentSaveAllAction.html>))を選択しても添付ファイルを保存することができます。


===添付ファイルを開く
添付ファイルを保存せずに開くには、ヘッダビューで添付ファイルをダブルクリックするか、右クリックして((<添付ファイルのコンテキストメニュー|URL:AttachmentMenu.html>))を表示し、((<[開く]|URL:AttachmentOpenAction.html>))を選択します。また、[メッセージ]-[添付ファイル]の下に添付ファイルのリストが表示されますので、そこから開きたい添付ファイルを選択することもできます。

いずれの場合も添付ファイルは関連付けされたアプリケーションで開かれます。ただし、添付ファイルを開くときにShiftキーを押しておくとエディタで開きます。使用するエディタは、((<エディットビュー2の設定|URL:OptionEdit2.html>))で外部エディタとして設定されたエディタが使用されます。

添付ファイルが((<セキュリティの設定|URL:OptionSecurity.html>))で[これらの拡張子の添付ファイルを開くときに警告する]で指定された拡張子を持っている場合には、開く前に警告します。これらのファイルはウィルスなどを含んでいる可能性がありますので開くときには注意してください。


===添付ファイルを削除する
保存して必要のなくなった添付ファイルをメッセージから削除することができます。添付ファイルを削除するには、((<[メッセージ]-[添付ファイル]-[削除]|URL:MessageDeleteAttachmentAction.html>))を選択します。

実際には添付ファイルを取り除いた新しいメッセージを生成し、元のメッセージを削除します((-ゴミ箱があればゴミ箱に入ります-))。添付ファイルはコンテンツ部分のみが削除されます。つまり、MIMEのパート構成は変えないまま、実際のデータだけが削除されます。

添付ファイルを削除されたメッセージを表示すると、ヘッダビューの添付ファイルの欄の背景色が変わります。

((<ヘッダビュー|"IMG:images/HeaderAttachmentDeleted.png">))


==ファイルの添付
メッセージを作成するときにはファイルやメッセージを添付することができます。


===ファイルを添付する
メッセージを作成するときにファイルを添付するには、エディットビューのメニューから((<[ツール]-[添付ファイル]|URL:ToolAttachmentAction.html>))を選択します。すると、[添付ファイル]ダイアログが開くので、[追加]ボタンをクリックしてファイル選択ダイアログを開きます。

((<[添付ファイル]ダイアログ|"IMG:images/AttachmentDialog.png">))

開いたファイル選択ダイアログで添付したいファイルを選択します。複数のファイルを選択すると選択したすべてのファイルが添付されます。

また、エクスプローラなどからエディットビューにファイルを((<ドラッグアンドドロップ|URL:DragAndDrop.html>))してもファイルを添付することができます。

ファイルを添付するとヘッダエディットビューに添付するファイルの一覧が表示されます。

((<ヘッダエディットビュー|"IMG:images/HeaderEditAttachment.png">))


===添付するのをやめる
[添付ファイル]ダイアログの[削除]ボタンをクリックすると選択された添付ファイルがリストから削除されます。

また、ヘッダエディットビューで削除したいファイルを右クリックすると((<添付ファイル編集のコンテキストメニュー|URL:AttachmentEditMenu.html>))が表示されますので、そこから((<[削除]|URL:AttachmentEditDeleteAction.html>))を選択して削除することもできます。


===メッセージを添付する
受信したメッセージを添付する場合には、リストビューで添付したいメッセージを選択してエディットビューに((<ドラッグアンドドロップ|URL:DragAndDrop.html>))してください。メッセージを添付すると((<"message/rfc822形式"|URL:http://www.ietf.org/rfc/rfc2046.txt>))で添付されます。


===添付ファイルを圧縮する
Windows版では添付したファイルをZIPファイルとしてまとめて圧縮することができます。この機能を使用するには、zip32.dllが必要です。zip32.dllはインストーラでデフォルトの状態でインストールした場合には既にインストールされています。それ以外の場合には、((<Info-ZIP|URL:http://www.info-zip.org/>))などからzip32.dllを入手してください。

添付ファイルを圧縮するには、((<[ツール]-[添付ファイルを圧縮]|URL:ToolArchiveAttachmentAction.html>))を選択してチェックされた状態にします。デフォルトでチェックされるかどうかは、((<エディットビュー2の設定|URL:OptionEdit2.html>))で設定できます。チェックが入っていると、送信するときに[圧縮]ダイアログが開き、ZIPファイルのファイル名を尋ねられます。

((<[圧縮]ダイアログ|"IMG:images/ArchiveDialog.png">))

ただし、以下の添付ファイルは圧縮されません。

*添付したメッセージ
*((<qmail.xml|URL:QmailXml.html>))のGlobal/ExcludeArchiveで指定した正規表現にマッチするファイル名のファイル（デフォルトでは、.zip, .lzh, .tgz, .gzの拡張子を持つファイルが圧縮されません）
*テンプレートでマルチパートとして生成した各パート

=end

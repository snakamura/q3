=begin
=メインウィンドウのメニューバー

((<メインウィンドウのメニューバー|"IMG:images/MainWindowMenuBar.png">))

====[ファイル]

+((<[隠す]|URL:FileHideAction.html>))
全てのウィンドウを隠してタスクトレイにアイコンを表示します。


+((<[保存]|URL:FileSaveAction.html>))
保持しているデータを全てディスクに書き込みます。


+((<[印刷]|URL:FilePrintAction.html>))
メッセージを印刷します。


+((<[読み込み]|URL:FileImportAction.html>))
ファイルからメッセージを読み込みます。


+((<[書き出し]|URL:FileExportAction.html>))
ファイルにメッセージを書き出します。


+((<[オフライン]|URL:FileOfflineAction.html>))
オンラインとオフラインを切り替えます。


+[保守]

*((<[整理]|URL:FileCompactAction.html>))
 
 メッセージボックスを整理します。

*((<[救出]|URL:FileSalvageAction.html>))
 
 失われたメッセージを救出します。

*((<[検査]|URL:FileCheckAction.html>))
 
 メッセージボックスを検査します。

*((<[ダンプ]|URL:FileDumpAction.html>))
 
 フォルダ構成とメッセージをダンプします。

*((<[ロード]|URL:FileLoadAction.html>))
 
 ダンプしたフォルダ構成とメッセージをロードします。

*((<[アンインストール]|URL:FileUninstallAction.html>))
 
 レジストリに書き込んだ情報を削除します。


+((<[終了]|URL:FileExitAction.html>))
QMAIL3を終了します。


====[フォルダ]

+((<[作成]|URL:FolderCreateAction.html>))
新しいフォルダを作成します。


+((<[削除]|URL:FolderDeleteAction.html>))
選択されたフォルダを削除します。


+((<[名前の変更]|URL:FolderRenameAction.html>))
選択されたフォルダの名前を変更します。


+((<[フォルダリストを更新]|URL:FolderUpdateAction.html>))
IMAP4アカウントでフォルダリストを更新します。


+((<[購読]|URL:FolderSubscribeAction.html>))
RSSアカウントで((<新しいフィードを購読|URL:SubscribeRssFeed.html>))したり、NNTPアカウントで((<新しいグループを購読|URL:SubscribeNntpGroup.html>))します。


+((<[空にする]|URL:FolderEmptyAction.html>))
選択されたフォルダを空にします。


+((<[ゴミ箱を空にする]|URL:FolderEmptyTrashAction.html>))
ゴミ箱を空にします。


+((<[サイズを表示|URL:FolderShowSizeAction.html>))
フォルダのサイズを計算して表示します。


+((<[プロパティ]|URL:FolderPropertyAction.html>))
((<フォルダのプロパティ|URL:FolderProperty.html>))を表示します。


====[編集]

+((<[元に戻す]|URL:EditUndoAction.html>))
直前の操作を元に戻します。


+((<[切り取り]|URL:EditCutAction.html>))
メッセージを切り取ります。


+((<[コピー]|URL:EditCopyAction.html>))
メッセージやテキストをコピーします。


+((<[貼り付け]|URL:EditPasteAction.html>))
メッセージを貼り付けます。


+((<[すべて選択]|URL:EditSelectAllAction.html>))
全てのメッセージ、またはテキストを選択します。


+((<[削除]|URL:EditDeleteAction.html>))
メッセージを削除します。


+((<[スパムとして削除]|URL:EditDeleteJunkAction.html>))
メッセージをスパムとして削除します。


+((<[削除済みをクリア]|URL:EditClearDeletedAction.html>))
IMAP4アカウントで削除済みのメッセージをクリアします。


+((<[キャッシュを削除]|URL:EditDeleteCacheAction.html>))
IMAP4アカウントやNNTPアカウントでメッセージのキャッシュを削除します。


+((<[メッセージ内検索]|URL:EditFindAction.html>))
メッセージ内のテキストを検索します。


+((<[次を検索]|URL:EditFindNextAction.html>))
直前の検索条件で次を検索します。


+((<[前を検索]|URL:EditFindPrevAction.html>))
直前の検索条件で前を検索します。


====[表示]

+((<[次のメッセージ]|URL:ViewNextMessageAction.html>))
ひとつ後のメッセージを選択します。


+((<[前のメッセージ]|URL:ViewPrevMessageAction.html>))
ひとつ前のメッセージを選択します。


+((<[次の未読メッセージ]|URL:ViewNextUnseenMessageAction.html>))
次の未読メッセージを選択します。


+((<[メッセージを選択]|URL:ViewSelectMessageAction.html>))
検索フォルダで選択しているメッセージを元のフォルダで選択します。


+((<[更新]|URL:ViewRefreshAction.html>))
フォルダを同期したり、検索フォルダで検索しなおします。


+[モード]

*((<[すべて表示]|URL:ViewRawModeAction.html>))
 
 メッセージ全体をテキストで表示します。

*((<[ソース表示]|URL:ViewSourceModeAction.html>))
 
 メッセージのソースを表示します。

*((<[引用を線で表示]|URL:ViewQuoteModeAction.html>))
 
 引用を線で表示します。

*((<[キャレットを表示]|URL:ViewSelectModeAction.html>))
 
 キャレットを表示します。

*((<"[S/MIME]"|URL:ViewSMIMEModeAction.html>))
 
 ((<"S/MIME"|URL:SMIME.html>))の復号と署名の検証を行います。

*((<[PGP]|URL:ViewPGPModeAction.html>))
 
 ((<PGP|URL:PGP.html>))の復号と署名の検証を行います。


+[ソート]

*((<[<カラム名>]|URL:ViewSortAction.html>))
 
 <カラム名>のカラムでソートします。

*((<[昇順]|URL:ViewSortAscendingAction.html>))
 
 昇順でソートします。

*((<[降順]|URL:ViewSortDescendingAction.html>))
 
 降順でソートします。

*((<[スレッド表示しない]|URL:ViewSortFlatAction.html>))
 
 ((<スレッド表示|URL:Thread.html>))を無効にします。

*((<[スレッド表示]|URL:ViewSortThreadAction.html>))
 
 ((<スレッド表示|URL:Thread.html>))を有効にします。

*((<[スレッドを浮かせる]|URL:ViewSortFloatThreadAction.html>))
 
 ((<スレッド表示|URL:Thread.html>))を有効にしスレッドを浮かせます。


+[フィルタ]

*((<[なし]|URL:ViewFilterAction.html>))
 
 ((<フィルタ|URL:Filter.html>))を無効にします。

*((<[<フィルタ名>]|URL:ViewFilterAction.html>))
 
 <フィルタ名>の((<フィルタ|URL:Filter.html>))を有効にします。

*((<[カスタム]|URL:ViewFilterCustomAction.html>))
 
 カスタムフィルタを有効にします。

*((<[編集]|URL:ConfigFiltersAction.html>))
 
 ((<フィルタ|URL:Filter.html>))を編集します。


+[テンプレート]

*((<[なし]|URL:ViewTemplateAction.html>))
 
 ((<表示用のテンプレート|URL:ViewTemplate.html>))を使わずに表示します。

*[<テンプレート名>]
 
 <テンプレート名>の((<表示用のテンプレート|URL:ViewTemplate.html>))を使って表示します。


+[エンコーディング]

*((<[自動判定]|URL:ViewEncodingAction.html>))
 
 自動判定されたエンコーディングで表示します。

*((<[<エンコーディング名>]|URL:ViewEncodingAction.html>))
 
 <エンコーディング名>のエンコーディングで表示します。


+[HTML]

*((<[HTMLを表示]|URL:ViewHtmlModeAction.html>))
 
 HTMLをブラウザコントロールで表示します。

*((<[オンラインで表示]|URL:ViewHtmlOnlineModeAction.html>))
 
 HTML中に埋め込まれた画像などをインターネットから取得します。

*((<[インターネットゾーンで表示]|URL:ViewHtmlInternetZoneModeAction.html>))
 
 インターネットゾーンで表示します。

*((<[最大サイズ]|URL:ViewZoomAction.html>))
 
 文字サイズを最大にします。

*((<[大サイズ]|URL:ViewZoomAction.html>))
 
 文字サイズを大にします。

*((<[中サイズ]|URL:ViewZoomAction.html>))
 
 文字サイズを中にします。

*((<[小サイズ]|URL:ViewZoomAction.html>))
 
 文字サイズを小にします。

*((<[最小サイズ]|URL:ViewZoomAction.html>))
 
 文字サイズを最小にします。

*((<[デフォルト]|URL:ViewZoomAction.html>))
 
 文字サイズをデフォルトにします。

*((<[大きくする]|URL:ViewZoomAction.html>))
 
 文字サイズを大きくします。

*((<[小さくする]|URL:ViewZoomAction.html>))
 
 文字サイズを小さくします。


+((<[カラムをカスタマイズ]|URL:ConfigViewsAction.html>))
リストビューのカラムをカスタマイズします。


+[コントロールの表示]

*((<[ツールバーを表示]|URL:ViewShowToolbarAction.html>))
 
 ツールバーの表示と非表示を切り替えます。

*((<[ステータスバーを表示]|URL:ViewShowStatusBarAction.html>))
 
 ステータスバーの表示と非表示を切り替えます。

*((<[フォルダを表示]|URL:ViewShowFolderAction.html>))
 
 フォルダビューの表示と非表示を切り替えます。

*((<[タブを表示]|URL:ViewShowTabAction.html>))
 
 タブの表示と非表示を切り替えます。

*((<[カラムを表示]|URL:ViewShowHeaderColumnAction.html>))
 
 ヘッダカラムの表示と非表示を切り替えます。

*((<[プレビューを表示]|URL:ViewShowPreviewAction.html>))
 
 プレビューの表示と非表示を切り替えます。

*((<[ヘッダを表示]|URL:ViewShowHeaderAction.html>))
 
 ヘッダビューの表示と非表示を切り返します。

*((<[同期ダイアログを表示]|URL:ViewShowSyncDialogAction.html>))
 
 同期ダイアログを表示します。


====[メッセージ]

+((<[新規]|URL:MessageCreateAction.html>))
新規メッセージを作成します。


+((<[返信]|URL:MessageCreateAction.html>))
メッセージに返信します。


+((<[全員に返信]|URL:MessageCreateAction.html>))
メッセージを全員に返信します。


+((<[転送]|URL:MessageCreateAction.html>))
メッセージを転送します。


+((<[編集]|URL:MessageCreateAction.html>))
メッセージを編集します。


+[テンプレート]

*((<[<テンプレート名>]|URL:MessageCreateAction.html>))
 
 ((<作成用のテンプレート|URL:CreateTemplate.html>))を使用してメッセージを作成します。


+[マーク]

*((<[既読にする]|URL:MessageMarkSeenAction.html>))
 
 メッセージを既読にします。

*((<[未読にする]|URL:MessageUnmarkSeenAction.html>))
 
 メッセージを未読にします。

*((<[マークをつける]|URL:MessageMarkAction.html>))
 
 メッセージにマークをつけます。

*((<[マークを消す]|URL:MessageUnmarkAction.html>))
 
 メッセージのマークを消します。

*((<[ダウンロード予約]|URL:MessageMarkDownloadAction.html>))
 
 メッセージをダウンロード予約します。

*((<[本文をダウンロード予約]|URL:MessageMarkDownloadTextAction.html>))
 
 メッセージのテキスト部分をダウンロード予約します。

*((<[削除マークをつける]|URL:MessageMarkDeletedAction.html>))
 
 メッセージに削除マークをつけます。


+[移動]

*((<[<フォルダ名>]|URL:MessageMoveAction.html>))
 
 メッセージを<フォルダ名>のフォルダに移動します。

*((<[その他]|URL:MessageMoveAction.html>))
 
 メッセージをダイアログで指定したフォルダに移動します。


+((<[ラベル]|URL:MessageLabelAction.html>))
メッセージのラベルを編集します。


+((<[振り分け]|URL:MessageApplyRuleAction.html>))
メッセージを((<振り分け|URL:ApplyRules.html>))ます。


+((<[全てのフォルダを振り分け]|URL:MessageApplyRuleAllAction.html>))
全てのフォルダのメッセージを((<振り分け|URL:ApplyRules.html>))ます。


+((<[選択されたメッセージを振り分け]|URL:MessageApplyRuleSelectedAction.html>))
選択されたメッセージを((<振り分け|URL:ApplyRules.html>))ます。


+((<[検索]|URL:MessageSearchAction.html>))
メッセージを((<検索|URL:Search.html>))します。


+[添付ファイル]

*((<[保存]|URL:MessageDetachAction.html>))
 
 添付ファイルを保存します。

*((<[削除]|URL:MessageDeleteAttachmentAction.html>))
 
 添付ファイルを削除します。

*((<[<添付ファイル名>]|URL:MessageOpenAttachmentAction.html>))
 
 <添付ファイル名>の添付ファイルを関連付けで開きます。

*((<[ダイジェストを展開]|URL:MessageExpandDigestAction.html>))
 
 選択されているメッセージがダイジェスト形式の場合に展開します。

*((<[結合]|URL:MessageCombineAction.html>))
 
 分割されているメッセージを結合します。


+((<[プロパティ]|URL:MessagePropertyAction.html>))
選択されているメッセージのプロパティを表示します。


====[ツール]

+((<[送受信]|URL:ToolSyncAction.html>))
送受信します。


+((<[受信]|URL:ToolReceiveAction.html>))
受信します。


+((<[送信]|URL:ToolSendAction.html>))
送信します。


+((<[フォルダを同期]|URL:ToolReceiveFolderAction.html>))
選択されたフォルダを同期します。


+[巡回]

*((<[<巡回コース名>]|URL:ToolGoroundAction.html>))
 
 <巡回コース名>のコースで((<巡回|URL:GoRound.html>))します。

*((<[編集]|URL:ConfigGoRoundAction.html>))
 
 巡回コースを編集します。


+((<[ダイアルアップ接続]|URL:ToolDialupAction.html>))
((<ダイアルアップ|URL:Dialup.html>))接続します。


+((<[アドレス帳]|URL:ToolAddressBookAction.html>))
((<アドレス帳ウィンドウ|URL:AddressBookWindow.html>))を開きます。


+((<[送信者をアドレス帳に追加]|URL:ToolAddAddressAction.html>))
送信者をアドレス帳に追加します。


+[スクリプト]

*((<[<スクリプト名>]|URL:ToolScriptAction.html>))
 
 <スクリプト名>のスクリプトを実行します。


+((<[自動巡回]|URL:ToolAutoPilotAction.html>))
((<自動巡回|URL:AutoPilot.html>))のOn/Offを切り替えます。


+[サブアカウント]

*((<[(デフォルト)]|URL:ToolSubAccountAction.html>))
 
 デフォルトの((<サブアカウント|URL:SubAccount.html>))に切り替えます。

*((<[<サブアカウント名>]|URL:ToolSubAccountAction.html>))
 
 <サブアカウント名>の((<サブアカウント|URL:SubAccount.html>))に切り替えます。


+((<[アカウント]|URL:ToolAccountAction.html>))
アカウントの設定をします。


+((<[オプション]|URL:ToolOptionsAction.html>))
オプションの設定をします。


====[ヘルプ]

+((<[ドキュメント]|URL:HelpOpenURLAction.html>))
ドキュメントのページをブラウザで開きます。


+((<[Webサイト]|URL:HelpOpenURLAction.html>))
QMAIL3のWebサイトをブラウザで開きます。


+((<[更新を確認]|URL:HelpCheckUpdateAction.html>))
新しいバージョンがあるか確認します。


+((<[バージョン情報]|URL:HelpAboutAction.html>))
バージョン情報を表示します。

=end

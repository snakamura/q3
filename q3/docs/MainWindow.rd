=begin
=メインウィンドウ

((<メインウィンドウ|"IMG:images/MainWindow.png">))

メインウィンドウは大きく分けて、フォルダビュー、リストビュー、プレビューの三つの部分に分かれます。また、((<[表示]-[コントロールの表示]-[フォルダを隠す]|URL:ViewShowFolderAction.html>))を選択するとフォルダビューが隠され、代わりにフォルダコンボボックスが表示されます((-Pocket PC版ではデフォルトでこちらの表示になっています-))。

((<フォルダコンボボックス|"IMG:images/FolderComboBox.png">))

また、フォルダビューでアカウントを選択しているときにはリストビューの代わりにフォルダリストビューが表示されます。

((<フォルダリストビュー|"IMG:images/FolderListView.png">))

各ビューの配置は((<その他の設定|URL:OptionMisc.html>))で自由に変更できます。


==メニューバー
メニュバーについては、((<メインウィンドウのメニューバー|URL:MainWindowMenuBar.html>))を参照してください。


==ツールバー
ツールバーの各ボタンの機能は以下の通りです。

+((<[新規]|URL:MessageCreateAction.html>))
新規メッセージを作成します。


+((<[返信]|URL:MessageCreateAction.html>))
フォーカスのあるメッセージに返信します。


+((<[全員に返信]|URL:MessageCreateAction.html>))
フォーカスのあるメッセージを全員に返信します。


+((<[転送]|URL:MessageCreateAction.html>))
フォーカスのあるメッセージを転送します。


+((<[削除]|URL:EditDeleteAction.html>))
選択されているメッセージを削除します。


+((<[前]|URL:ViewPrevMessageAction.html>))
一つ前のメッセージを選択します。


+((<[次]|URL:ViewNextMessageAction.html>))
一つ後のメッセージを選択します。


+((<[次の未読]|URL:ViewNextUnseenMessageAction.html>))
次の未読メッセージを選択します。


+((<[検索]|URL:MessageSearchAction.html>))
メッセージを((<検索|URL:Search.html>))します。


+((<[同期]|URL:ToolSyncAction.html>))
選択されているアカウントを同期します。右側の矢印をクリックするとメニューが表示され、((<送信のみ|URL:ToolSendAction.html>))や((<受信のみ|URL:ToolReceiveAction.html>))を行うことができます。


+((<[巡回]|URL:ToolGoroundAction.html>))
((<巡回|URL:GoRound.html>))します。右側の矢印をクリックすると巡回するコースをメニューから選択できます。左側のアイコンをクリックすると先頭の巡回コースで巡回します。


ツールバーを隠すには、メニューから((<[表示]-[コントロールの表示]-[ツールバーを隠す]|URL:ViewShowToolbarAction.html>))を選択します。ツールバーのボタンのカスタマイズについては、((<ツールバーのカスタマイズ|URL:CustomizeToolbars.html>))を参照してください。


==ステータスバー
ステータスバーは7個のペインに分けられています。ただしプレビューを非表示にしている場合には、最初の3ペインしか表示されません。各ペインに表示される情報は左から順番に以下の通りです。

+1
一般的なメッセージなどが表示されます。


+2
((<オンラインかオフラインか|URL:OnlineOffline.html>))がアイコンで表示されます。


+3
使用している((<フィルタ|URL:Filter.html>))が表示されます。右リックするとメニューが表示され、フィルタを変更することができます。


+4
表示しているメッセージのエンコーディングが表示されます。右クリックするとメニューが表示され、((<エンコーディングを指定|URL:ViewEncodingAction.html>))することができます。エンコーディングの自動判定に失敗して文字化けしている場合には、エンコーディングを指定することでメッセージを表示できます。


+5
使用している((<表示用のテンプレート|URL:ViewTemplate.html>))が表示されます。右クリックするとメニューが表示され、表示用テンプレートを変更することができます。


+6
メッセージが((<"S/MIME"|URL:SMIME.html>))や((<PGP|URL:PGP.html>))で暗号化されていて、それを復号した場合にはアイコンが表示されます。


+7
メッセージが((<"S/MIME"|URL:SMIME.html>))や((<PGP|URL:PGP.html>))で署名されていて、それを検証した場合にはアイコンが表示されます。検証に成功したかどうかでアイコンは異なります。署名を検証した場合、クリックすると((<検証結果|URL:MessageCertificateAction.html>))が表示されます。

ステータスバーを隠すには、メニューから((<[表示]-[コントロールの表示]-[ステータスバーを隠す]|URL:ViewShowStatusBarAction.html>))を選択します。



==フォルダビュー
フォルダビューには((<アカウント|URL:Account.html>))と((<フォルダ|URL:Folder.html>))が、ツリー表示されます。フォルダを選択するとリストビューにそのフォルダ内のメッセージ一覧が表示されます。アカウントを選択すると、フォルダリストビューでそのアカウント内のフォルダ一覧がされます。

右クリックすると表示されるコンテキストメニューについては、((<フォルダビューのコンテキストメニュー|URL:FolderMenu.html>))を参照してください。

フォルダビューを隠すには、メニューから((<[表示]-[コントロールの表示]-[フォルダを隠す]|URL:ViewShowFolderAction.html>))を選択します。フォルダビューを隠すとフォルダコンボボックスが表示されます。

フォルダビューの各種設定は、((<フォルダビューの設定|URL:OptionFolder.html>))で設定できます。


==フォルダコンボボックス
フォルダビューには((<アカウント|URL:Account.html>))と((<フォルダ|URL:Folder.html>))が、リスト表示されます。フォルダを選択するとリストビューにそのフォルダ内のメッセージ一覧が表示されます。アカウントを選択すると、フォルダリストビューでそのアカウント内のフォルダ一覧がされます。

右クリックすると表示されるコンテキストメニューについては、((<フォルダコンボボックスのコンテキストメニュー|URL:FolderMenu.html>))を参照してください。

フォルダコンボボックスの各種設定は、((<フォルダビューの設定|URL:OptionFolder.html>))で設定できます。


==タブ
タブを使用すると複数のフォルダやアカウントを素早く切り替えることができます。新しいタブを開くには、フォルダビューやフォルダコンボボックスで開きたいフォルダを選択して、コンテキストメニューから((<[新しいタブで開く]|URL:TabCreateAction.html>))を選択します。

右クリックすると表示されるコンテキストメニューについては、((<タブのコンテキストメニュー|URL:TabMenu.html>))を参照してください。

タブは、Alt+1からAlt+0までで切り替えることもできます。これはタブを隠しているときも有効です。また、タブの順番を入れ替えるには、Alt+LeftやAlt+Rightを使用します。

タブを隠すには、メニューから((<[表示]-[コントロールの表示]-[タブを隠す]|URL:ViewShowTabAction.html>))を選択します。

タブの各種設定は、((<タブの設定|URL:OptionTab.html>))で設定できます。


==フォルダリストビュー
フォルダビューやフォルダコンボボックスでアカウントを選択すると、フォルダリストビューが表示されます。フォルダリストビューには、アカウント内のフォルダがリスト表示されます。各フォルダの左にあるチェックボックスのチェックを外すことでそのフォルダを非表示にすることができます。

フォルダのサイズの欄は選択した状態では表示されません。表示するには、((<[フォルダ]-[サイズを表示]|URL:FolderShowSizeAction.html>))を選択します。

右クリックすると表示されるコンテキストメニューについては、((<フォルダリストビューのコンテキストメニュー|URL:FolderListMenu.html>))を参照してください。


==リストビュー
フォルダビューやフォルダコンボボックスでフォルダを選択すると、リストビューが表示されます。リストビューには、フォルダ内のメッセージがリスト表示されます。メッセージは((<スレッド表示|URL:Thread.html>))することもできます。

リストビューの上部にはヘッダカラムが表示されます。ヘッダカラムをクリックすることによりメッセージのソート方法を変更することができます。ヘッダカラムを隠すには、メニューから((<[表示]-[コントロールの表示]-[ヘッダカラムを隠す]|URL:ViewShowHeaderColumnAction.html>))を選択します。カラムを追加したり表示する内容を変更するには、((<[表示]-[カラムをカスタマイズ]|URL:ConfigViewsAction.html>))を選択します。詳細については、((<リストビューのカスタマイズ|URL:CustomizeListView.html>))を参照してください。

右クリックすると表示されるコンテキストメニューについては、((<リストビューのコンテキストメニュー|URL:ListMenu.html>))を参照してください。

リストビューの各種設定は、((<リストビューの設定|URL:OptionList.html>))で設定できます。


==ヘッダビュー
ヘッダビューには、リストビューでフォーカスのあるメッセージの宛先や送信者、件名、添付ファイルなどが表示されます。詳細は、((<メッセージウィンドウ|URL:MessageWindow.html>))を参照してください。


==プレビュー
プレビューには、リストビューでフォーカスのあるメッセージの本文が表示されます。((<HTML表示|URL:HtmlView.html>))を有効にしている場合、HTMLメールはブラウザコントロールを用いて表示されます。また、((<表示用のテンプレート|URL:ViewTemplate.html>))を使用して表示する内容をカスタマイズすることもできます。

右クリックすると表示されるコンテキストメニューについては、((<プレビューのコンテキストメニュー|URL:MessageMenu.html>))を参照してください。

プレビューの各種設定は、((<プレビューの設定|URL:OptionPreview.html>))で設定できます。

=end

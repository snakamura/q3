=begin
=アカウントの作成

==アカウントの作成
アカウントの作成画面を開くまでは、POP3アカウントの((<アカウントの作成|URL:CreatePop3Account.html>))とほぼ同じです。

[アカウントの作成]ダイアログでは、[クラス]に「rss」を選択し、[受信プロトコル]に「RSS」を、[送信プロトコル]に「Blog」を選択します。

((<[アカウントの作成]ダイアログ|"IMG:images/TutorialRssCreateAccountDialog.png">))


==アカウントの設定
[一般]タブ, [ユーザ]タブ, [詳細]タブは指定する項目がないので、[RSS]タブをクリックします。

((<[RSS]タブ|"IMG:images/TutorialRssRssPage.png">))

プロキシサーバを使用する場合には、[HTTPプロキシ]で指定してください。[インターネットオプションを使用]を選択すると、Internet Explorerで指定されているプロキシサーバが使用されます。


[Blog]タブは設定項目がないので飛ばします。[ダイアルアップ]タブと[高度]タブはPOP3アカウントと同様に指定します。


==購読するフィードの指定
購読するRSSを指定するにはフォルダを作成します。フォルダを作成するにはメニューから、((<[フォルダ]-[作成]|URL:FolderCreateAction.html>))を選択します。

((<[フォルダの作成]ダイアログ|"IMG:images/TutorialRssCreateFolderDialog.png">))

[名前]には任意の名前を指定します。[タイプ]は[ローカルフォルダ]を指定し、[同期可能]にチェックが入っているのを確認して、[OK]をクリックします。

フォルダが作成されると、作成されたフォルダの[プロパティ]ダイアログが開き、[パラメータ]タブが選択されます。

((<[プロパティ]ダイアログ|"IMG:images/TutorialRssFolderPropertyDialog.png">))

URLの行を選択して[編集]ボタンをクリックすると、URLを指定する[パラメータ]ダイアログが開きますので、[URL]に、RSSのURLを指定します。

((<[パラメータ]ダイアログ|"IMG:images/TutorialRssParameterDialog.png">))

[OK]をクリックするとURLが設定されます。その他のパラメータについては、((<RSSのパラメータ|URL:FolderPropertyRssParameter.html>))を参照してください。

[OK]を押して[プロパティ]ダイアログを閉じるとフィードの設定は完了です。

購読するフィードごとに以上の手順を繰り返します。

=end

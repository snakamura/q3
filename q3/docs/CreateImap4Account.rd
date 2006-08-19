=begin
=アカウントの作成

==アカウントの作成
IMAP4のアカウントの作成は、POP3のアカウントの作成とほぼ同じなので、まずはPOP3の((<アカウントの作成|URL:CreatePop3Account.html>))を参照してください。ここでは、POP3アカウントと異なる部分のみ説明します。

((<[アカウントの作成]ダイアログ|"IMG:images/TutorialImap4CreateAccountDialog.png">))

[アカウントの作成]ダイアログでは、[受信プロトコル]として「IMAP4」を、[送信プロトコル]として「SMTP」を選択します。


==アカウントの設定
アカウントの設定ダイアログの、[一般], [ユーザ], [詳細]タブはPOP3アカウントと同様に指定します。


[IMAP4]タブでIMAP4の設定を行います。

((<[IMAP4]タブ|"IMG:images/TutorialImap4Imap4Page.png">))

基本的にはデフォルトのままで問題ありませんが、UW-IMAPサーバを使用する場合に、特定のディレクトリ以下をメールボックスとして扱う場合には、[ルートフォルダ]でディレクトリ名を指定します。

[SMTP], [ダイアルアップ], [高度]タブもPOP3アカウントと同様に指定します。


[OK]をクリックしてアカウントの設定を終えると、フォルダリストを更新するかどうかを尋ねられます。

((<フォルダリストを更新しますか?|"IMG:images/TutorialImap4UpdateFolderList.png">))

ネットワークに繋がっている場合には[はい]をクリックしてサーバからフォルダのリストを取得します。ネットワークに繋がっていない場合には、後でメニューから((<[フォルダ]-[更新]|URL:FolderUpdateAction.html>))を選択することでフォルダリストの更新をすることができます。


フォルダの更新が終了したら、((<[ファイル]-[オフライン]|URL:FileOfflineAction.html>))を選択してオンラインモードにします。


==特殊フォルダの設定
QMAIL3でメールを扱うには、以下のような幾つかの特殊フォルダが必要になります。

:受信箱
  受信したメールが入るフォルダです。IMAP4では常に存在するのでフォルダの更新をすれば取得されます。
:送信箱
  送信するメールを入れるフォルダです。作成しないとメールの送信ができません。
:送信控え
  送信したメールを入れるフォルダです。作成しないとメールの送信ができません。
:草稿箱
  草稿メールを入れるフォルダです。作成しないと草稿が保存できません。
:ゴミ箱
  削除したメールを入れるフォルダです。作成しない場合、直接メールが削除されます。
:スパム
  スパムを入れるフォルダです。作成しない場合、スパムフィルタが使用できません。

IMAP4アカウントの場合、受信箱は常にInboxという名前になります。それ以外のフォルダの名前には任意の名前が付けられます。

まずは、特殊フォルダとして使用するフォルダを作成します。すでにサーバ上にそのためのフォルダがある場合にはこの手順は飛ばしてください。

フォルダを作成するには、フォルダビューで作成するフォルダの親フォルダ（ルートに作成する場合にはアカウント）を選択して、((<[フォルダ]-[作成]|URL:FolderCreateAction.html>))を選択します。一部のIMAP4サーバでは、フォルダの下にフォルダが作成できなかったり、ルートにフォルダが作成できなかったりといった制限があることがあります。ここでは、送信箱を作成します。

((<フォルダの作成|"IMG:images/TutorialImap4CreateFolder.png">))

[フォルダの作成]ダイアログで、作成したいフォルダの名前を指定します。

((<[フォルダの作成]ダイアログ|"IMG:images/TutorialImap4CreateFolderDialog.png">))

[名前]に作成するフォルダの名前を指定します。[タイプ]はそのまま[リモートフォルダ]を選択します((-特殊フォルダを[ローカルフォルダ]として作成することも可能ですが、あまりお勧めしません-))。[OK]を押すとフォルダが作成されます。

次に、フォルダビューで作成されたフォルダを選択し、((<[フォルダ]-[プロパティ]|URL:FolderPropertyAction.html>))を選択します。

((<[プロパティ]ダイアログ|"IMG:images/TutorialImap4FolderPropertyDialog.png">))

送信箱として使用するために、[プロパティ]ダイアログの[タイプ]で、[送信箱]にチェックを入れます。[OK]をクリックしてダイアログを閉じると、フォルダビューでのアイコンが変わって送信箱として認識されたことがわかります。

同様の手順で必要な全ての特殊フォルダの設定を行います。特殊フォルダの詳細については、((<フォルダ|URL:Folder.html>))を参照してください。

=end

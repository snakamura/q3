=begin
=フラグ

メッセージには以下のフラグを付けることができます。

:既読
  既に読んだメッセージに付けられます。
:返信済み
  返信済みのメッセージに付けられます。
:転送済み
  転送済みのメッセージに付けられます。
:送信済み
  送信済みのメッセージに付けられます。
:草稿
  草稿のメッセージに付けられます。このフラグが付いているメッセージは草稿箱に入っていても送信されません。
:マーク
  マークされたメッセージに付けられます。マークされたメッセージはリストビューのアイコンにマークが付きます。このフラグは自由に使用することができます。
:削除
  IMAP4では削除されたメッセージに付けられます。削除フラグの付いたメッセージは、((<[編集]-[削除済みをクリア]|URL:EditClearDeletedAction.html>))を選択すると実際に削除されます。または、((<IMAP4の設定|URL:AccountImap4.html>))で、[CLOSEを送信]にチェックをするとフォルダを同期した後で削除されます。POP3では、このフラグの付いたメッセージは次回受信するときにサーバから削除されます。
:ダウンロード
  次回受信時にダウンロードするメッセージに付けます。部分的に受信したメッセージにこのフラグを付けると、次回受信するときにメッセージ全体がダウンロードされます。
:本文ダウンロード
  ダウンロードと同様ですが、メッセージ全体ではなく本文部分（テキストの部分）のみがダウンロードされます。IMAP4以外では使用できません。
:宛先に含まれる
  宛先に自分が含まれるメッセージに付けられます。このフラグは受信時に自動的に付けられます。
:Ccに含まれる
  Ccに自分が含まれるメッセージに付けられます。このフラグは受信時に自動的に付けられます。
:ユーザ1, ユーザ2, ユーザ3, ユーザ4
  自由に使用することができるフラグです。


メッセージにどのフラグが付いているかは、((<[メッセージ]-[プロパティ]|URL:MessagePropertyAction.html>))を選択して[プロパティ]ダイアログで確認することができます。このダイアログではフラグを設定することもできます。また、以下のアクションを使って、フラグを付けたり外したりすることもできます。

*((<[メッセージ]-[マーク]-[既読にする]|URL:MessageMarkSeenAction.html>)), ((<[メッセージ]-[マーク]-[未読にする]|URL:MessageUnmarkSeenAction.html>))
*((<[メッセージ]-[マーク]-[マークを付ける]|URL:MessageMarkAction.html>)), ((<[メッセージ]-[マーク]-[マークを消す]|URL:MessageUnmarkAction.html>))
*((<[メッセージ]-[マーク]-[削除マークを付ける]|URL:MessageMarkDeletedAction.html>)), ((<MessageUnmarkDeletedアクション|URL:MessageUnmarkDeletedAction.html>))
*((<[メッセージ]-[マーク]-[ダウンロード予約]|URL:MessageMarkDownloadAction.html>)), ((<MessageUnmarkDownloadアクション|URL:MessageUnmarkDownloadAction.html>))
*((<[メッセージ]-[マーク]-[本文をダウンロード予約]|URL:MessageMarkDownloadTextAction.html>)), ((<MessageUnmarkDownloadTextアクション|URL:MessageUnmarkDownloadTextAction.html>))
*((<MessageMarkUser1アクション|URL:MessageMarkUser1Action.html>)), ((<MessageUnmarkUser1アクション|URL:MessageUnmarkUser1Action.html>))
*((<MessageMarkUser2アクション|URL:MessageMarkUser2Action.html>)), ((<MessageUnmarkUser2アクション|URL:MessageUnmarkUser2Action.html>))
*((<MessageMarkUser3アクション|URL:MessageMarkUser3Action.html>)), ((<MessageUnmarkUser3アクション|URL:MessageUnmarkUser3Action.html>))
*((<MessageMarkUser4アクション|URL:MessageMarkUser4Action.html>)), ((<MessageUnmarkUser4アクション|URL:MessageUnmarkUser4Action.html>))

=end

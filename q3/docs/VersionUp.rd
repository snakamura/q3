=begin
=バージョンアップ

インストールした方法によってバージョンアップの方法が異なります。インストーラでインストールした場合には、インストーラを再度実行してください。Zipファイルからインストールした場合には、すべてのファイルを上書きしてください。


QMAIL3では、メニューやツールバーなどをカスタマイズ可能にするために初回起動時にこれらの定義ファイルをメールボックスフォルダにコピーします。これらのファイルは、バージョンアップ後の最初の起動時にバージョンアップされていないかどうかが確認され、バージョンアップされていれば自動的に上書きされます。

ただし、これらのファイルがユーザによって編集されていた場合には、下のようなダイアログを表示して上書きするかどうかを確認します。基本的には上書きした上で、必要に応じて編集しなおしてください。大きな変更があった場合などには上書きしないと正常に動作しなくなる可能性があります。

このチェックの対象になるのは以下のファイルです。

*images/account_mail.bmp
*images/account_news.bmp
*images/account_rss.bmp
*images/folder.bmp
*images/list.bmp
*images/listdata.bmp
*images/toolbar.bmp
*profiles/header.xml
*profiles/headeredit.xml
*profiles/keymap.xml
*profiles/menus.xml
*profiles/toolbars.xml
*profiles/views.xml
*templates/mail/new.template
*templates/mail/reply.template
*templates/mail/reply_all.template
*templates/mail/forward.template
*templates/mail/edit.template
*templates/mail/url.template
*templates/mail/print.template
*templates/mail/quote.template
*templates/news/new.template
*templates/news/reply.template
*templates/news/reply_all.template
*templates/news/forward.template
*templates/news/edit.template
*templates/news/url.template
*templates/news/print.template
*templates/news/quote.template


===[リソース]ダイアログ

((<リソース|"IMG:images/ResourceDialog.png">))

上書きするファイルのチェックボックスにチェックを入れてください。


+[すべて選択]
すべてのファイルにチェックを入れます。


+[すべてクリア]
すべてのファイルのチェックをはずします。


+[バックアップする]
上書きしたファイルのバックアップファイルを作成します。バックアップファイルは同じフォルダにファイル名の後ろに「.bak」を付加した名前で作成されます。

=end

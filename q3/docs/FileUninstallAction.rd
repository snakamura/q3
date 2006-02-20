=begin
=FileUninstallアクション

QMAIL3が使用するレジストリを削除します。具体的には、HKEY_CURRENT_USER\Software\sn\q3以下の全てのキーを削除し、それ以外に子キーがなければ、HKEY_CURRENT_USER\Software\snも削除します。

OSへのメールクライアントとしての登録やmailto URLへの関連付けなどは削除されません。インストーラで行われるこれらの設定はアンインストーラによって削除されます。


==引数
なし


==有効なウィンドウ・ビュー
*メインウィンドウ

=end

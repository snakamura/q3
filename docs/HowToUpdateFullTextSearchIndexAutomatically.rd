=begin
=全文検索のインデックスを自動で更新するにはどうすればよいですか?

((<全文検索|URL:FullTextSearch.html>))のインデックスは[インデックスの更新]ボタンをクリックしないと更新されません。インデックスを自動で更新するには、インデックスを更新するためのバッチファイルを作成し、それをタスクスケジューラで定期的に実行するようにします。たとえば、全てのアカウントのインデックスを更新するには以下のようなバッチファイルが使えます。

 @echo off
 set BASE=C:\Documents and Settings\username\Application Data\QMAIL3
 for /D %%f in (*.*) do estcmd gather -cl -fm -cm -sd "%BASE%\accounts\%%f\index" "%BASE%\accounts\%%f\msg"

メールボックスディレクトリは環境に合わせて書き換える必要があります。また、この例ではHyper Estraierのestcmd.exeを起動していますが、Namazuを使用する場合にはmknmz.batを呼び出すようにします。

=end

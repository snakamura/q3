=begin
=同期中にキャンセルしてもすぐにキャンセルされません

同期中に同期ダイアログのキャンセルボタンをクリックすると、進行中の全ての同期がキャンセルされます。しかしサーバの反応が遅い場合などには、以下のような理由によりすぐにキャンセルされないことがあります。

同期をキャンセルには、上位プロトコルレベルでのキャンセルと下位プロトコルレベルでのキャンセルがあります。前者は、POP3やIMAP4, SMTPといったプロトコルレベルでキャンセルできるところ（たとえば、メッセージを一通受信した後など）でプロトコル的に問題のないようにキャンセルを行います。後者は上位のプロトコルの状態に関わらず強制的に接続を切断します。前者の方法でキャンセルするほうが望ましいため、キャンセルボタンがクリックされたときにはまず前者の方法でキャンセルを試み、5秒間キャンセルできなかった場合には後者の方法でキャンセルします。

このため、サーバとの通信が極端に遅い場合などにはキャンセルしても5秒間はキャンセルされない状態が生じます。

また、サーバのホスト名を検索している最中はキャンセルが効きませんので、この間にキャンセルを行ってもしばらくキャンセルされないことがあります。

=end

=begin
=ログ

QMAIL3には二種類のログがあります。一つ目はシステムログでもう一つは通信ログです。


==システムログ
システムログは主に通信以外の部分でログを出力するときに使用されます。システムログはメールボックスディレクトリのlogsディレクトリの中にスレッドごとにユニークな名前をつけて出力されます。

ログのエントリには5つのレベルがあります。レベルは、高い順からFATAL, ERROR, WARN, INFO, DEBUGです。((<その他の設定|URL:OptionMisc.html>))の[ログレベル]で、指定したレベル以上のログのみを出力するように設定することができます。たとえば、ERRORを指定するとFATALとERRORレベルのエントリのみが出力され、DEBUGを指定すると全てのエントリが出力されます。

システムログはバグ報告などがあったときに動作を調べるために追加することが多いので、エラーがあってもまったくログが出力されない場合も多いです。


==通信ログ
通信ログは通信中にサーバとやり取りした内容を記録するために使用されます。サーバとうまく接続できなかったりエラーが発生するときにログを取ることにより、エラーの内容を詳しく調べることができます。通信ログはアカウントディレクトリのlogディレクトリの中にセッションごとにユニークな名前をつけて出力されます。

サーバと通信した内容が全て書き込まれますので、クリアテキストで認証している場合にはパスワードも書き込まれます。トラブルの解決のためにログを開示するときには注意してください。


=end

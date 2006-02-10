=begin
=レジストリを使用しない方法

QMAIL3ではメールディレクトリやプロファイル名の保持にレジストリを使用します。レジストリを使わないようにするには以下の二つの方法があります。


==((<コマンドラインオプション|URL:CommandLine.html>))を使用する
-dと-pを使用してメールディレクトリやプロファイル名を指定するとレジストリを使用しなくなります。ショートカットなどを用いて以下のように指定すれば良いでしょう。

 -d "C:\mail" -p ""

メールディレクトリとしてC:\mailを使用し、プロファイル名は空という指定になります。


==メールディレクトリを固定する
USBメモリなどにQMAIL3を入れてさまざまな環境で使う場合、絶対パスが異なるために上記の方法が使えない場合があります。この場合、q3u.exeが存在するディレクトリにmailという名前のディレクトリを作成してください。QMAIL3は起動時にこのようなディレクトリが存在した場合、強制的にこのディレクトリをメールディレクトリとして使用します。

=end

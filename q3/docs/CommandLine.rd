=begin
=コマンドライン

QMAIL3に指定可能なコマンドラインは以下の通りです。

-dと-pは常に指定可能です。-g, -s, -a, -c, -rは排他です。ただし、-sと-aは組み合わせて指定することができます。


==-d <メールボックスディレクトリ>

===説明
メールボックスディレクトリを指定します。通常、メールボックスディレクトリのパスはレジストリから取得しますが、このオプションを指定すると指定されたパスをメールボックスディレクトリとして使用します。異なるメールボックスディレクトリを指定すればQMAIL3を複数起動することができます（Windows版のみ）。メールボックスディレクトリを指定した場合、-pでプロファイル名を指定しない限りレジストリからプロファイル名を読み込みません。

===例
 -d C:\mail


==-p <プロファイル名>

===説明
プロファイル名を指定します。通常、プロファイル名はレジストリから取得しますが、このオプションを使用すると指定されたプロファイル名を使用します。((<プロファイル|URL:Profile.html>))を参照してください。


===例
 -p mobile


==-g <巡回コース>

===説明
起動直後に指定されたコースで巡回します。指定された巡回コースが見つからない場合にはすべてのアカウントのすべてのフォルダを送受信します。巡回については、((<巡回|URL:GoRound.html>))を参照してください。

===例
 -g "All Inboxes"


==-s <URL>

===説明
指定されたURLを開きます。URLはmailto URL、またはfeed URLである必要があります。例えば、mailto:info@example.orgを渡した場合、info@example.org宛てのメール作成画面が開かれます。

このオプションを使用して関連付けを行うことで、ブラウザなどでmailto URLやfeed URLをクリックしたときにQMAIL3を起動させることができるようになります。

基本的には、現在選択されているアカウントが使用されます。ただし、現在選択しているアカウントがメールアカウントでなかった場合には、以下のように使用するアカウントを決定します。

(1)mailto URLの場合
   (1)((<qmail.xml|URL:QmailXml.html>))のGlobal/DefaultMailAccountで指定されたアカウント
   (2)指定されていなかった場合、一番上にあるメールアカウント
(2)feed URLの場合
   (1)((<qmail.xml|URL:QmailXml.html>))のGlobal/DefaultRssAccountで指定されたアカウント
   (2)指定されていなかった場合、一番上にあるRSSアカウント

そのようなアカウントが存在しない場合には何もしません。

===例
 -s mailto:info@example.org
 -s feed://www.example.org/index.rdf


==-a <ファイル名>

===説明
指定されたファイルを添付ファイルとして添付した状態でエディットビューを開きます。-sでのmailto URLの指定と同時に指定できます。

===例
 -a C:\temp\test.png
 -s test@example.org -a "C:\Data Files\Test.doc"


==-c [<ファイル名>]

===説明
指定されたファイルを読み込んでメールを作成します。読み込まれるファイルは、プラットフォームのデフォルトのエンコーディングでエンコードされている必要があります。また、RFC2822に基づく形式になっている必要がありますが、ヘッダの文字列はRFC2047やRFC2231に基づいてエンコードされている必要はありません（エンコードされていても構いません）。例えば、以下のような内容でファイルを作成します。

 To: foo@example.com
 Subject: これはテストです
 
 ここに本文が入ります。
 ヘッダと本文の間には空行が必要です。

ファイル名を省略した場合には、クリップボードから取得した文字列を使用してメールを作成します。

外部エディタなどからメールを作成する場合に使用します。

===例
 -c
 -c "C:\Temp\mail.txt"


==-r [<ファイル名>]

===説明
-cと同様ですが、草稿としてメールを作成します。

===例
 -r
 -r "C:\Temp\mail.txt"

=end

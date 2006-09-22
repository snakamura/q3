=begin
=[アカウントの作成]ダイアログ

((<アカウントの作成|"IMG:images/CreateAccountDialog.png">))

[OK]を押すと作成したアカウントのプロパティを指定するダイアログが開きます。


+[名前]
アカウント名を指定します。

ファイル名として使えない文字は使えません。


+[クラス]
アカウントクラスを指定します。選択できるのは、「mail」「news」「rss」のいずれかです。メールアカウントを作成するには「mail」を、ニュースアカウント作成するには「news」を、RSSアカウントを作成するには「rss」を選択します。


+[受信プロトコル]
受信するのに使用するプロトコルを指定します。アカウントクラスによって選択できるプロトコルが変わります。各アカウントクラスで選択できるのはそれぞれ以下のプロトコルになります。

*mail
  *POP3
  *IMAP4
*news
  *NNTP
*rss
  *RSS


+[送信アカウント]
送信するのに使用するプロトコルを指定します。アカウントクラスによって選択できるプロトコルが変わります。各アカウントクラスで選択できるのはそれぞれ以下のプロトコルになります。

*mail
  *SMTP
  *POP3 (XTND XMIT)
*news
  *NNTP
*rss
  *Blog


====[メッセージボックス]
メッセージボックスの作り方を指定します。以下の二つから選択できます。

基本的には[1メッセージ1ファイル]をお勧めします。ただし、Windows CEで外部メモリカードを使用する場合には、[1ファイル]にした方がディスクの使用量が少なくて済みます。

[1ファイル]にした場合に、ウィルス入りのメッセージを受信したときに、ウィルス対策ソフトがファイルを消してしまうことがあります。このような動作をするウィルス対策ソフトを使用している場合には、この形式は使わないでください。


+[1メッセージ1ファイル]
一通のメッセージを一つのファイルにします。

*利点
  *トラブルが起きたときにメッセージが失われる可能性が低い
  *全文検索が使える
  *インデックスファイルが壊れたときに復元しやすい
*欠点
  *ディスクを多く消費する
  *少し遅い


+[1ファイル]
すべてのメッセージを一つのファイルにします。

*利点
  *ディスクの使用効率が良い
*欠点
  *ファイルサイズは4GB程度までしか使えない
  *全文検索が使えない

[1ファイル]を選んだ場合には、[ブロックサイズ]を指定できます。0以外を指定すると指定したファイルサイズでファイルを分割します。この機能は、Windows CEで本体メモリに大きなファイルを置くと極端に処理速度が落ちるのを回避するために主に使用します。


+[インデックスのブロックサイズ]
インデックスファイルのブロックサイズを指定します。上記のどちらの形式を選んだ場合でも、メッセージのインデックス情報は別の一つのファイルに保存されます。0以外を指定すると、そのファイルを指定したファイルサイズで分割します。
=end

=begin
=アカウント

アカウントはメールを管理する単位で、基本的には接続するメールサーバひとつに対してひとつ作成します。アカウントには、メールサーバのホスト名やログインするためのユーザ名、パスワード、またそのアカウントからメッセージを送信したときに使用されるメールアドレスなどを設定します。また、アカウントごとにそのアカウントで送受信したメッセージを保存するためのフォルダが作成されます。


==アカウントクラス
アカウントにはクラスがあります。アカウントのクラスとは、そのアカウントがどのように使われるかを指定するものです。アカウントクラスには以下の3つがあります。

*メール
*ニュース
*RSS

メールアカウントはメールサーバに接続してメールを送受信するためのアカウントで、プロトコルとしてPOP3, IMAP4, SMTPを使用することができます。ニュースアカウントはNetnews用のアカウントで、プロトコルとしてNNTP使用することができます。RSSアカウントはRSSやAtomのフィードを取り込むためのアカウントで、HTTPやHTTPSを使用してRSSやAtomを取り込みます。

アカウントのクラスはアカウントを作成するときにのみ指定でき、後から変えることはできません。

また、アカウントのクラスによってメール作成時に使われるテンプレートが異なります。詳細は、((<テンプレート|URL:Template.html>))を参照してください。


==アカウントの作成
新しいアカウントの作成方法については、((<アカウントの作成|URL:CreateAccount.html>))を参照してください。また、アカウントに設定する項目については、((<アカウントのプロパティ|URL:AccountProperty.html>))を参照してください。また、以下のチュートリアルも参考にしてください。

*((<POP3アカウントの作成|URL:CreatePop3Account.html>))
*((<IMAP4アカウントの作成|URL:CreateImap4Account.html>))
*((<NNTPアカウントの作成|URL:CreateNntpAccount.html>))
*((<RSSアカウントの作成|URL:CreateRssAccount.html>))


==マルチアカウント
複数のメールサーバを使用するには、それぞれのメールサーバごとにアカウントを作成します。詳細は、((<マルチアカウント|URL:MultiAccount.html>))を参照してください。


==サブアカウント
アカウントは複数の((<サブアカウント|URL:SubAccount.html>))を持つことができます。サブアカウントを使用することによって、例えばLAN経由でネットワークに接続しているときとダイアルアップで接続しているときにアカウントの設定を切り替えるというようなことができます。詳しくは、((<サブアカウント|URL:SubAccount.html>))を参照してください。

=end

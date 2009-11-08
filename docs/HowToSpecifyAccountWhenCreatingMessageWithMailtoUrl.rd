=begin
=関連付けでエディットビューを開いたときのアカウントを固定するにはどうすればよいですか?

mailto URLに関連付けをすると、ブラウザなどでメールアドレスをクリックしたときに、QMAIL3のエディットビューが開きます。このときに使われるアカウントは、((<コマンドライン|URL:CommandLine.html>))で説明されているように決まります。つまり、

(1)現在選択されているメールアカウント
(2)選択されているのがメールアカウントではない場合には、((<qmail.xml|URL:QmailXml.html>))のGlobal/DefaultMailAccountで指定されたアカウント
(3)指定されていなかった場合、一番上にあるメールアカウント

の順に検索されて決まります。

常に特定のアカウントを使いたい場合には、url.templateを編集して、

 X-QMAIL-Account: {@Account()}{
   @If(@Identity(),
       @Concat('\nX-QMAIL-SubAccount: ', @SubAccount()),
       '')
 }

の部分を、

 X-QMAIL-Account: Main

のようにします。ここで、Mainは常に使用するアカウントの名前です。

=end

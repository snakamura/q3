=begin
=なぜリストビューで選択されたメッセージがプレビューに表示されないことがあるのですか?

リストビューではメッセージが選択されているのに、そのメッセージがプレビューには表示されないことがあります。たとえば、振り分けした後や、IMAP4でサーバ上のメッセージが削除された場合などにこのような状態になります。

QMAIL3では、ある操作をしたときに次にどのメッセージが選択されるかが事前に明らかでない場合には、リストビューでメッセージが選択されてもプレビューには表示しないようになっています。これは、望まないメッセージを誤って表示してしまわないための仕様です。たとえば、振り分けを行ったときに現在表示しているメッセージが移動された場合、どのメッセージが次に選択されるかは明らかではありません。

逆にメッセージを手動で削除したときや移動したときには、次に選択されるメッセージがあらかじめ明らかなため、次の（削除済みでない）メッセージが選択されます。

プレビューが表示されない状態になった場合には、リストビューでそのメッセージをクリックするか、スペースキーを押すことでプレビューにそのメッセージを表示することができます。

また、((<qmail.xml|URL:QmailXml.html>))のPreviewWindow/UpdateAlwaysを1にすると、どのような状況でもプレビューに選択されたメッセージが表示されるようになります。

=end

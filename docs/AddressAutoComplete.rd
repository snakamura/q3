=begin
=アドレスの自動補完

ヘッダエディットビューでアドレスを入力するフィールドでは、アドレスの自動補完が行われます。

((<アドレスの自動補完|"IMG:images/AddressAutoComplete.png">))

アドレスの自動補完の対象になるのは、アドレス帳のエントリ、外部アドレス帳から取り込んだエントリ、最近使用したメールアドレスです。入力した文字列や文字列中のドメイン部分が、これらのエントリの名前やアドレス、アドレスのドメイン部分に含まれると、自動補完用のドロップダウンリストが表示されます。使用したいアドレスを選択すると入力中の文字列に置き換えられます。入力をそのまま続けると自動的に候補が絞り込まれます。

アドレスの自動補完中はカーソルキーの上下でリスト内のアドレスを選択できます。候補リストを消すにはESCを押します。

アドレス帳に登録されていないメールアドレスにメールを送信すると、そのアドレスが記憶され、次回の自動補完時に候補として使用されます。デフォルトでは最新10件のアドレスが記憶されます。記憶するアドレスの個数は((<qmail.xml|URL:QmailXml.html>))のRecentAddress/Maxで指定できます。自動で記憶されたアドレスを削除するには、候補リストが表示されているときに消したいアドレスを選択してShift+Delを押します。


==補完の例
アドレス帳に以下のエントリが含まれるときにどのように補完の候補が作られるかの例を示します。

*Taro Yamada <taro@example.org>
*Jiro Yoshida <tjiro@sub.example.org>
*test@example.com

入力文字列がtの場合、すべてのエントリが補完対象になります。

*Taro Yamada <taro@example.org>
*Jiro Toyama <tjiro@docomo.co.jp>
*test@example.com

taまで入力すると最初のひとつが補完対象になります。

*Taro Yamada <taro@example.org>

入力文字列がyの場合、最初のエントリが補完対象になります。

*Taro Yamada <taro@example.org>

入力文字列がexamの場合、最初と最後のエントリが補完対象になります。

*Taro Yamada <taro@example.org>
*test@example.com

入力文字列がfoo@eの場合、以下の文字列が補完対象になります。

*foo@example.org
*foo@example.com

=end

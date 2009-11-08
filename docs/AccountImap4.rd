=begin
=IMAP4の設定

IMAP4でメッセージを受信するための設定を行います。

((<[IMAP4]タブ|"IMG:images/AccountImap4ReceivePage.png">))


+[ルートフォルダ]
サーバがUW-IMAPの時にルートとなるフォルダを指定します。素のUW-IMAPでは何も指定しないとホームディレクトリ以下の全てのファイルがフォルダとして表示されてしまいます。ここでルートとなるディレクトリを指定することで、特定のディレクトリ以下のフォルダだけを表示することができます。


+[1リクエストあたりの取得数]
ENVELOPEコマンドなどでメッセージのデータを取得するときに、一回のリクエストで何通のメッセージのデータを取得するかを指定します。大きくした方が効率は良くなりますが、通信環境が悪い場合には小さくしたほうが接続が切れた場合などに無駄になるデータが少なくなります。デフォルトでは100通です。


+[最大セッション数]
オンラインモードで使用するセッション数の最大値を指定します。オンラインモードでは各フォルダに対して別のセッションを張りますが、ここで指定したセッション数を超えると、最も使っていないセッションを再利用するようになります。デフォルトは5です。


+[ENVELOPEを使用]
ENVELOPEコマンドを送るかどうかを指定します。一部のサーバにはバグがあって、ENVELOPEコマンドの結果が不正になるために処理を続行できなくなることがあります。チェックを外すと、ENVELOPEコマンドを送る代わりに必要なヘッダを取得してクライアント側でパースします。デフォルトでは使用します。


+[常にBODYSTRUCTUREを使用]
全てのメッセージに対してBODYSTRUCTUREコマンドを送るかどうかを指定します。一部のサーバにはバグがあって、BODYSTRUCTUREの結果が不正になるため処理を続行できなくなることがあります。チェックを外すと、マルチパートのメッセージに対してのみBODYSTRUCTUREコマンドを発行するようになります。デフォルトでは常に使用します。


+[BODYSTRUCTUREを信頼]
BODYSTRUCTUREの結果を信頼するかどうかを指定します。一部のサーバにはバグがあって、BODYSTRUCTUREの結果から取得したマルチパートのバウンダリなどが間違っていることがあります。チェックを外すとこれらのサーバのバグを回避するために追加のコマンドを送るようになります。デフォルトでは信頼します。


+[削除済みメッセージを破棄]
フォルダを同期した後で削除フラグの付いたメッセージを実際に破棄するかどうかを指定します。デフォルトでは破棄しません。


+[NAMESPACEを使用]
フォルダリストの更新をするときにNAMESPACEコマンドを使用するどうかを指定します。NAMESPACEコマンドを使用する場合には、下の[個人], [その他], [共有]にチェックを入れることでどのネームスペースのフォルダを使用するかを指定できます。デフォルトでは使用しません。


+[個人]
個人フォルダを使用します。


+[その他]
その他のフォルダを使用します。


+[共有]
共有フォルダを使用します。

=end

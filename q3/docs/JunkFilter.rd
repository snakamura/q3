=begin
=スパムフィルタ

==スパムフィルタについて
QMAIL3には学習型のスパムフィルタが実装されています。初期状態ではすべてのメールをスパムでないメール（クリーン）と判断しますが、スパムをスパムフォルダに移動することによってスパムフィルタを学習させることができます。ある程度学習すると、スパムを自動的にスパムフォルダに移動するようになります。

==使い方
スパムフィルタはアカウントごとに有効・無効を切り替えることができます。デフォルトでは無効になっていますので、有効にするにはアカウントの設定の[高度]タブで[スパムフィルタを有効にする]にチェックを入れてください。

スパムフィルタが使用できるのは、受信プロトコルとしてPOP3, IMAP4, NNTPを使用している各アカウントのみになります。

==学習
初期状態のスパムフィルタは何も学習していないので、すべてのメールをクリーンと判断します。スパムを受信したらそのメールをスパムフォルダに移動します。そうすると、そのメールの内容をスパムとして学習します。

クリーンなメールを学習する必要があることにも注意してください。デフォルトの設定では受信したときにクリーンと判断されたメールはクリーンなメールとして学習されます。

強制的にクリーンなメールやスパムを学習させるには以下のようにします。

(1)学習したいメッセージを選択して、Alt+Shift+Xを入力します
(2)[アクションの起動]ダイアログが開くので、そこに「MessageAddClean」（クリーンなメールを学習する場合）または「MessageAddJunk」（スパムを学習する場合）と入力し、OKボタンを押します


==設定
((<スパムフィルタの設定|URL:OptionJunkFilter.html>))をご覧ください。

==振り分けとの関係
スパムフィルタは振り分けの一部として実装されています。

具体的には、スパムフィルタを有効にすると以下のような振り分け条件が自動的に自動実行用の振り分けルールの先頭に挿入され、真と判定されるとスパムとして扱われます。

:POP3, NNTPの場合
  @Junk()
:IMAP4の場合
  @And(@Not(@Deleted()), @Junk(@Seen()))

スパムフィルタの処理の前に別のルールで処理したい場合などには、スパムフィルタを無効にした上で、振り分けルールにて@Junk()を使用することでスパム判定を行うことができます。


==注意事項
QMAIL3のスパムフィルタは、クリーンなメールとスパムの両方に現れるトークン（単語）を保持し、判定時にはメール中にあわられる各トークンがどれくらいクリーンとして、またはスパムとして学習されたかに基づいてスパムかどうかの判定を行います。このため、極端にスパムばかり学習させた状態だとスパムでないメールをスパムと判定してしまう可能性が高くなります。これを防ぐため、最低でも100通のクリーンなメールを学習しないと機能しないようにしてあります。

経験上、おおよそクリーンを2000通程度、スパムを1000通程度学習させるとかなり正確な判定ができるようになります。

=end

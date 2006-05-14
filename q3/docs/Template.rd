=begin
=テンプレート

テンプレートはメッセージを作成するときや表示するときなどに適用されます。たとえば、新しくメッセージを作成するときに空のメッセージを作成したり、返信するときに返信元のメッセージを引用したメッセージを作成したりします。表示用のテンプレートを使用すると、受信したメッセージの表示方法を通常とは変えることもできます。印刷時など、その他の場面で使用されるテンプレートもあります。これらのテンプレートを編集することで、メール作成時に定型文を挿入したり、印刷するときのフォーマットを変更することができます。

テンプレートのファイルはメールボックスディレクトリのtemplatesディレクトリの下に置かれています。templatesディレクトリ以下にはアカウントクラスごとにディレクトリが存在し、その中に各テンプレートのファイルが置かれています。デフォルトでは以下のテンプレートが置かれています。

:new.template
  新規作成用
:reply.template
  返信用
:reply_all.template
  全員に返信用
:forward.template
  転送用
:edit.template
  編集用
:print.template
  印刷用
:quote.template
  引用用
:url.template
  mailto URL処理用

各テンプレートの使い方については以下を参照してください。

*((<作成用のテンプレート|URL:CreateTemplate.html>))
*((<表示用のテンプレート|URL:ViewTemplate.html>))
*((<その他のテンプレート|URL:OtherTemplate.html>))

テンプレートのファイルはプラットフォームのデフォルトのエンコーディングで保存する必要があります。


==テンプレートの検索順
作成用や表示用のテンプレートは以下の順に検索されます。たとえば、new.templateを検索する場合には、

(1)<アカウントディレクトリ>/templates/new_<フォルダID>.template
(2)<アカウントディレクトリ>/templates/new.template
(3)<メールボックスディレクトリ>/templates/<アカウントクラス>/new.template

たとえば、メールボックスディレクトリがC:\Documents and Settings\test\Application Data\QMAIL3で、名前がMainというメール用のアカウントで、現在のフォルダが受信箱（IDは1）だった場合には以下の順に検索されます。

(1)C:\Documents and Settings\test\Application Data\QMAIL3\accounts\Main\templates\new_1.template
(2)C:\Documents and Settings\test\Application Data\QMAIL3\accounts\Main\templates\new.template
(3)C:\Documents and Settings\test\Application Data\QMAIL3\templates\mail\new.template

このように、特定のアカウントやフォルダで使用するテンプレートを切り替えたい場合には、適切なディレクトリに適切な名前でテンプレートを置くことによってテンプレートを切り替えることができます。


==テンプレート書式
テンプレートはテンプレート書式にしたがって記述されています。テンプレートを作成・編集する場合にはこの書式にしたがって記述する必要があります。テンプレート書式で書いたテンプレートは、実行時に評価されて文字列になります。

テンプレート書式では、テンプレートはリテラルとマクロに分けられます。リテラルはそのまま評価結果となり、マクロはそのマクロを評価した結果が評価結果になります。マクロは{}で括って記述します。

たとえば、Fromで指定された文字列をToヘッダに指定したい場合には以下のように書くことになります。

 To: {From}

「To: 」がリテラルで「From」がマクロです。マクロで括られています。

リテラルの中やマクロの中で{や}を使いたいときには{{や}}のように二重にします。たとえば以下のようになります。

 {{これはリテラル}}
 {@RegexMatch(Date, /\d{{4}}/)}

一行目は「{これはリテラル}」という文字列に評価され、二行目では「@RegexMatch(Date, /\d{4}/)」というマクロを評価した結果に評価されます。

マクロについては、((<マクロ|URL:Macro.html>))を参照してください。

=end

=begin
=リストビューの色

リストビューの各行には条件に応じて文字色を指定することができます。たとえば、デフォルトでは未読のメッセージは赤で、それ以外のメッセージは黒で表示されるようになっています。どの条件でどの色を使用するかは、((<色の設定|URL:OptionColors.html>))で行います。条件は((<マクロ|URL:Macro.html>))で指定しますが、よく使われる条件に関してはマクロを記述することなく指定することができます。


==色の指定の詳細
色の指定は以下のような構造をしています。まず、QMAIL3内部では色セットのリストを持っています。色セットは、その色セットが使用されるアカウント・フォルダと、色エントリのリストを持っています。色エントリでは、そのエントリが使用される条件と、文字色・背景色・フォント（太字）の設定を持っています。

使用される色は以下のように決定されます。

(1)まず、現在のアカウントとフォルダ名を使って、使用可能な全ての色セットの中から全ての色エントリをリストを作ります。
(2)(1)で作った色エントリのリストの上から順に条件をチェックしていきます。条件にあう色エントリがあれば、そのエントリで指定されている文字色・背景色・フォント（太字）を取得します。
(3)エントリによっては文字色のみ指定されていたり、文字色とフォント（太字）のみ指定されているので、文字色・背景色・フォント（太字）の全てが確定するまで次の色エントリを調べます。
(4)三つ全てが決まるか、最後の色エントリまで調べ終わったら終了します。
(5)最後まで調べても決まらなかった項目に関してはデフォルトの値（リストビューで指定された文字色・背景色と、通常のフォント）が使用されます。

=end

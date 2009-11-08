=begin
=検索の設定

[オプション]ダイアログの[検索]パネルでは((<検索|URL:Search.html>))の設定を行います。

((<検索の設定|"IMG:images/OptionSearch.png">))


====[基本]
基本検索の設定を行います。

+[マクロ]
基本検索で使われるマクロを指定します。@Fは、正規表現を使うかどうかによって、@Containか@RegexMatchを呼び出す関数です。$Search, $Caseと$Regexにはそれぞれ検索文字列と大文字小文字を区別するかどうか、正規表現を使うかどうかの値が実行時に割り当てられます。デフォルトでは「@Or(@F(%Subject, $Search, $Case), @F(%From, $Search, $Case), @F(%To, $Search, $Case), @F(@Label(), $Search, $Case))」です。詳細については、((<検索|URL:Search.html>))を参照してください。


====[全文検索]
全文検索で使うエンジンを指定します。

+[Namazu]
((<Namazu|URL:http://www.namazu.org/>))を使用します。

+[Hyper Estraier]
((<Hyper Estraier|URL:http://hyperestraier.sourceforge.net/>))を使用します。

+[カスタム]
カスタムを選択した場合には、検索・更新時に呼び出すコマンドを指定します。コマンドの指定では以下の置換文字列が使用できます。

*検索時
  :$index
    インデックスのパス
  :$condition
    検索条件
  :$encoding
    システムのエンコーディング
*更新時
  :$msg
    メッセージボックスのパス
  :$index
    インデックスのパス
  :$encoding
    システムのエンコーディング

検索時には指定されたコマンドを実行し、標準出力から結果を取り込みます。標準出力に出力される結果は以下のような形式である必要があります。

(1)一行に一つのヒットしたファイル名が出力される
(2)一行の一番最後に現れる/の後ろがファイル名になっている

((:(2):))の条件に合致しない行は無視されます。

=end

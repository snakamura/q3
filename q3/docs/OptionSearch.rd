=begin
=検索の設定

[オプション]ダイアログの[検索]パネルでは検索関係の設定を行います。

((<検索の設定|"IMG:images/OptionSearch.png">))


====[基本]
マクロ検索の設定を行います。

+[マクロ]
マクロ検索で使われるマクロを指定します。$Conditionと$Caseはそれぞれ検索文字列と大文字小文字を区別するかどうかの値を実行時に割り当てられます。デフォルトでは「@Or(@Contain(%Subject, $Search, $Case), @Contain(%From, $Search, $Case), @Contain(%To, $Search, $Case))」です。

ここで指定されたマクロを@X()と考えると、検索時のマクロは以下のようになります。

 @Progn(@Set('Search', <検索文字列>),
        @Set('Case', <大文字と小文字を区別する>),
        @X())

さらに検索時のオプションで、[すべてのヘッダを検索する]や[本文を検索する]にチェックを入れた場合には、以下のようなマクロが使われます。

*すべてのヘッダを検索する場合
  @Or(@X(), @Contain(@Decode(@Header()), $Search, $Case))
*本文を検索する場合
  @Or(@X(), @Contain(@Body(), $Search, $Case))
*上記の両方
  @Or(@X(),
      @Contain(@Decode(@Header()), $Search, $Case),
      @Contain(@Body(), $Search, $Case))


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

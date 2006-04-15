=begin
=基本検索

基本検索ではマクロを使用した検索を行います。

((<基本検索|"IMG:images/MacroSearchPage.png">))


+[検索文字列]
検索文字列を指定します。


+[大文字小文字を区別する]
検索時に大文字と小文字を区別するかどうかを指定します。


+[すべてのヘッダを検索する]
すべてのヘッダを検索するかどうかを指定します。


+[本文を検索する]
本文を検索するかどうかを指定します。


+[マクロ]
検索文字列で指定された文字列をマクロとして処理し、そのマクロにマッチするメッセージを検索します。


====[フォルダ]
検索対象のフォルダを指定します。


+[現在]
現在のフォルダを検索対象とします。


+[再帰]
現在のフォルダとそのすべての子孫フォルダを検索対象とします。


+[すべて]
すべてのフォルダを検索対象とします。


+[新しい検索フォルダを作成する]
通常、指定した条件がデフォルトの検索フォルダに設定され、その検索フォルダを開くことで検索結果を表示します。このチェックボックスにチェックを入れると、指定した条件で新たに検索フォルダを作成し、そのフォルダを開きます。


==マクロの生成方法
基本検索では指定された条件からマクロを生成し、そのマクロにマッチするメッセージを検索します。生成されるマクロは以下のように決定されます。

まず、[マクロ]にチェックを入れた場合には、[検索文字列]がそのままマクロになります。それ以外の場合、((<検索の設定|URL:OptionSearch.html>))の[マクロ]で指定したマクロを使用します。デフォルトでは、「@Or(@Contain(%Subject, $Search, $Case), @Contain(%From, $Search, $Case), @Contain(%To, $Search, $Case), @Contain(@Label(), $Search, $Case))」です。$Searchは[検索文字列]で指定された文字列に、$Caseは[大文字小文字を区別する]にチェックを入れた場合には真に、それ以外の場合には偽になります。

ここで指定されたマクロを@X()と考えると、検索時のマクロは以下のようになります。

 @Progn(@Set('Search', <検索文字列>),
        @Set('Case', <大文字と小文字を区別する>),
        @X())

さらに、[すべてのヘッダを検索する]や[本文を検索する]にチェックを入れた場合には、以下のようなマクロが使われます。

*[すべてのヘッダを検索する]にチェックを入れた場合
  @Or(@X(), @Contain(@Decode(@Header()), $Search, $Case))
*[本文を検索する]にチェックを入れた場合
  @Or(@X(), @Contain(@Body(), $Search, $Case))
*上記の両方の場合
  @Or(@X(),
      @Contain(@Decode(@Header()), $Search, $Case),
      @Contain(@Body(), $Search, $Case))



=end

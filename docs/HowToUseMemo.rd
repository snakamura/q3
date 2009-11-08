=begin
=メッセージにメモを付けることはできませんか?

短いメモであれば、((<ラベル|URL:Label.html>))を使うことができます。もっと長いメモを付けたい場合には、以下のようにマクロなどを組み合わせることで、メッセージにメモをつけることができます。

各メッセージに付けられたメモを一つのファイルとして保存するという方法を取ります。まず、メモを保存するディレクトリとして、メールボックスフォルダにmemoというディレクトリを作成しておきます。

それから、メールボックスディレクトリの下にmacroというディレクトリを作成し、以下のマクロをmemo.macroというファイル名で保存します。

 @Progn(# メモのファイル名を取得します。
        @Defun('GetFileName',
               @Concat('memo/',
                       @RegexReplace(Message-Id,
                                     /[^A-Za-z0-9_.!@#$%^&-]/,
                                     '',
                                     @True()))),
        # メモを読み込んで返します。読み込めない場合には空文字列を返します。
        @Defun('LoadMemo',
               @If(@CanMemo(),
                   @Catch(@Load(@GetFileName()), ''),
                   '')),
        # メモを保存します。一番目の引数に保存するメモを指定します。
        @Defun('SaveMemo',
               @If(@CanMemo(),
                   @Save(@GetFileName(), $1),
                   @False())),
        # メモが取れるかどうか調べます。Message-Idが無いとメモは取れません。
        @Defun('CanMemo',
               Message-Id),
        # 入力ダイアログを開いてメモを入力し保存します。
        @Defun('InputMemo',
               @If(@CanMemo(),
                   @Catch(@SaveMemo(@InputBox('メモを入力してください',
                                              :INPUT-MULTILINE,
                                              @LoadMemo())),
                          @MessageBox('メモの保存に失敗しました')),
                   @MessageBox('Message-Idがないのでメモが取れません'))))

そして、((<menus.xml|URL:MenusXml.html>))を編集し、メニューを表示したいところに以下のようなエントリを追加します。

 <menuitem text="Mem&amp;o..."
           action="MessageMacro"
           param="@Progn(@Include('macro/memo.macro'),@InputMemo())"/>

必要に応じて、((<toolbars.xml|URL:ToolbarsXml.html>))にエントリを追加してツールバーのボタンにすることもできます。

さらに、((<header.xml|URL:HeaderXml.html>))を編集してメッセージに付けたメモを表示するようにします。以下のようなエントリをメモを表示したいところに追加します。

 <line hideIfEmpty="memo">
   <static width="auto" style="bold" showAlways="true">Memo:</static>
   <edit name="memo" multiline="4" wrap="true">{@Progn(@Include('macro/memo.macro'), @LoadMemo())}</edit>
 </line>

ただし、この方法では以下のような制限があります。

*メモを入力した直後はビューが更新されないのでヘッダビューにメモが表示されない
*メモを削除してもファイルが削除されない
*IMAP4アカウントでもサーバ側にメモを持つわけではないので別のPCとメモを共有できない

=end

=begin
=同じスレッドのメッセージをまとめて既読にするにはどうすれば良いですか?

((<MessageMacroアクション|URL:MessageMacroAction.html>))を使用することで現在選択しているメッセージと同じスレッドのメッセージをまとめて既読にすることができます。使用するマクロは、

 @ForEach(@Thread(), @Seen(@True()))

です。

このアクションをメニューやツールバー、キーボードショートカットに割り当てることもできます。詳細は、((<メニューのカスタマイズ|URL:CustomizeMenus.html>))、((<ツールバーのカスタマイズ|URL:CustomizeToolbars.html>))、((<キーボードショートカットのカスタマイズ|URL:CustomizeAccelerators.html>))を参照してください。

=end

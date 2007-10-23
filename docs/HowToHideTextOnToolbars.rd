=begin
=ツールバーの文字列を表示しないようにするにはどうすればよいですか?

ツールバーのボタンの下に文字列を表示するかどうかは、((<toolbars.xml|URL:ToolbarsXml.html>))で指定できます。各ツールバーのshowText属性をfalseにするとテキストが表示されなくなります。

例えば、メインウィンドウのツールバーの場合、

 <toolbar name="mainframe" showText="true">

を、

 <toolbar name="mainframe" showText="false">

にします。

=end

=begin
=スペルチェックはできますか?

QMAIL3にはスペルチェック機能はありませんが、((<aspell|URL:http://aspell.net/>))などのスペルチェッカを呼び出すことでスペルチェックを行うことができます。ここでは、aspellを使う方法を説明します。

まず、aspellと必要な辞書をインストールし、単体で正しく動作することを確認します。

次に、以下のようなスクリプトを用意し、メールボックスディレクトリのscriptsディレクトリの下に"Spell Check.js"というファイル名で保存します。

 var aspell = "\"C:\\Program Files\\aspell\\bin\\aspell.exe\"";
 var tempFile = "C:\\Documents and Settings\\username\\Application Data\\QMAIL3\\temp\\spellcheck.txt";
 
 // テキストをすべて選択してクリップボードにいれ、
 // クリップボード経由でテキストを取得する
 editFrameWindow.invokeAction("EditSelectAll");
 editFrameWindow.invokeAction("EditCopy");
 var getClipboardMacro = macroParser.parse("@Clipboard()");
 var inText = getClipboardMacro.evaluate(application.nothing, document.accounts(0));
 
 // 取得したテキストを一時ファイルに書き込む
 var fs = new ActiveXObject("Scripting.FileSystemObject");
 try {
   var inFile = fs.OpenTextFile(tempFile, 2, true);
   try {
     inFile.Write(inText);
   }
   finally {
     inFile.Close();
   }
 
   // aspellを起動して終了するまで待つ
   var shell = new ActiveXObject("WScript.Shell");
   shell.Run(aspell + " -c \"" + tempFile + "\"", 5, true);
 
   // aspellが修正したファイルを読み込む
   var outText = null;
   var outFile = fs.OpenTextFile(tempFile, 1);
   try {
     outText = outFile.ReadAll();
   }
   finally {
     outFile.Close();
   }
 
   // クリップボードを経由してエディットビューに貼り付け
   var setClipboardMacro = macroParser.parse("@Clipboard($value)");
   setClipboardMacro.setVariable("value", outText);
   setClipboardMacro.evaluate(application.nothing, document.accounts(0));
   editFrameWindow.invokeAction("EditPaste");
 }
 finally {
   fs.DeleteFile(tempFile);
 }

aspellやtempFileで指定しているパスは適宜書き換えてください。

実際に使うときには、エディットビューのメニューから[スクリプト]-[Spell Check]を選択します。すると、aspellが起動してスペルチェックが行われ、終了するとその結果がエディットビューに取り込まれます。

((<キーボードショートカットのカスタマイズ|URL:CustomizeAccelerators.html>))を行うことで、ショートカットキーを割り当てることもできます。


=end

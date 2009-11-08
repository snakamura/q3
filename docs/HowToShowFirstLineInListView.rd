=begin
=リストビューにメッセージの一行目を表示することはできますか?

MLのアーカイブなどでリスト表示・スレッド表示をするときに、Subjectの替わりに本文の一行目を表示する場合があります。MLなどの場合、Subjectは「Re: ...」のような感じで情報量が少ないことが多いので、これはなかなか便利です。同じ事を、QMAIL3のリストビューでするには以下のようにします。

方針としては、ビューのカスタマイズで一行目を取り出すマクロを指定するという方針です。一行目を取り出すといっても、引用や挨拶などは飛ばしたいので、これをマクロでやるのはできなくはありませんが、ちょっと酷です。そこで、@Scriptを使って一行目の取り出しはJScriptで書くことにしました。できたのはこんな感じです。

 @If(@Contain(%Subject, 'Re:'), @Script('
 
 function getDescription(text) {
   var maxline = 20; // 指定した行数以内に見つからなかったらあきらめる
   var minchar = 5;  // これより短い行は無視
   var regexes = [   // これらの正規表現にマッチしたら無視
     /^\\s+$/,                            // 空白だけの行
     /^[>|#＞｜＃]/,                      // 引用符っぽい文字から始まっている
     /^In (message|article)\\s/i,         // 引用の説明っぽい
     /[+-]\\d{4},?\\s*$/,                 // タイムゾーンで終わっているのも引用の説明っぽい
     /(writes|wrote|さん)[:：]\\s*$/i,    // これも引用の説明っぽい
     /こんにちは|こんにちわ|こんばんは/,  // 挨拶
     /(申します|です)。?$/,               // 名前を述べていそう
     /世話/,                              // 「お世話になります」とか
     /(.)\\1{4}/,                         // 同じ文字が5文字以上続くのは区切りっぽい
     /^[\\w-]+:/                          // ヘッダの引用っぽい
   ];
   var lines = text.split(/\\n/, maxline);
   for (n in lines) {
     var line = lines[n];
     if (line.length < minchar)
       continue;
     var match = false;
     for (r in regexes) {
       var regex = regexes[r];
       match = line.search(regex) != -1;
       if (match)
         break;
     }
     if (!match)
       return line;
   }
   return lines.length > 0 ? lines[0] : \"\";
 }
 result.value = getDescription(arguments.item(0)).replace(/^[\\s　]*/, \"\");
 
 ', 'JScript', @Body('', @True())), %Subject)

使いたいフォルダを開いて、((<[表示]-[カラムをカスタマイズ]|URL:ConfigViewsAction.html>))を選び、ダイアログで[件名]を選んで[編集]をクリックします。そのダイアログで[タイプ]に[その他]を指定して、[マクロ]に上記のマクロを指定し、[キャッシュ]にチェックを入れて[OK]をクリックします。

上記のダイアログではマクロを指定するところに一行しか入れられないので、上のマクロを一行にしたものが以下です。

@If(@Contain(%Subject, 'Re:'), @Script('function getDescription(text) { var maxline = 20; var minchar = 5; var regexes = [ /^\\s+$/, /^[>|#＞｜＃]/, /^In (message|article)\\s/i, /[+-]\\d{4},?\\s*$/, /(writes|wrote|さん)[:：]\\s*$/i, /こんにちは|こんにちわ|こんばんは/, /(申します|です)。?$/, /世話/, /(.)\\1{4}/, /^[\\w-]+:/ ]; var lines = text.split(/\\n/, maxline); for (n in lines) { var line = lines[n]; if (line.length < minchar) continue; var match = false; for (r in regexes) { var regex = regexes[r]; match = line.search(regex) != -1; if (match) break; } if (!match) return line; } return lines.length > 0 ? lines[0] : \"\"; } result.value = getDescription(arguments.item(0)).replace(/^[\\s　]*/, \"\");', 'JScript', @Body('', @True())), %Subject)

=end

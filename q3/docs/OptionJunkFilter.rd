=begin
=スパムフィルタの設定

[オプション]ダイアログの[スパムフィルタ]パネルでは((<スパムフィルタ|URL:JunkFilter.html>))の設定を行います。

((<スパムフィルタの設定|"IMG:images/OptionJunkFilter.png">))


+[スパムフォルダに、またはスパムフォルダからメッセージを移動したら学習する]
メールをスパムフォルダに移動したらスパムとして、スパムフォルダから別のフォルダに移動したらクリーンとして学習するかどうかを指定します。デフォルトでは学習します。


+[フィルタした結果を学習する]
フィルタしてスパムと判定された場合にはスパムとして、クリーンと判定された場合にはクリーンとして学習します。デフォルトでは学習します。

上記の二つは通常チェックを入れた状態で使用してください。十分学習が進み、これ以上学習しなくても良い場合にはチェックをはずしても構いません。


+[閾値]
スパムらしさの判定は0から1までの範囲で行われます。数値が大きいほどスパムらしいことになります。ここで指定した数値よりも大きいスパムらしさを持ったメールがスパムとして処理されます。デフォルトでは、0.95です。

ほとんどの場合、クリーンなメールのスパムらしさは限りなく0に近くなり、スパムのスパムらしさは限りなく1に近くなります。このため、ほとんどの場合、閾値を指定する必要はありません。閾値を設定したくなった場合には、先により多くのメールを学習させることをお勧めします。


+[最大サイズ]
学習に使用する最大バイト数を指定します。ここで指定したサイズよりも大きなメールは先頭から指定したバイト数までが判定や学習に使用されます。デフォルトは、32768(32KB)です。


+[ホワイトリスト], [ブラックリスト]
ホワイトリストにリストされたメールアドレスからのメールは必ずクリーンとして判定され、ブラックリストにリストされたメールアドレスからのメールは必ずスパムとして判定されます。

一行に一つずつメールアドレスを指定します。メールアドレスは大文字小文字を区別して部分一致で比較されます。つまり、「@example.com」のように指定すると、foo@example.comもbar@example.comもマッチすることになります。


+[修復]
QMAIL3が不正終了した場合などにスパムデータベースが読み込めなくなり、スパムフィルタが動作しなくなることがあります。この場合に[修復]ボタンを押すとデータベースを修復しようと試みます。必ず修復できるとは限りませんし、修復した結果が意図したものである保障もありませんが、多少おかしくなっていても機能的には問題ないことが多いので、破損した場合には試してみることをお勧めします。

=end

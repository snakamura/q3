=begin
=変数

マクロでは任意の名前の変数を作成することができます。変数を宣言するには、((<@Set|URL:SetFunction.html>))を使用します。例えば、Subjectの内容を変数に設定するには、

 @Set('subject', Subject)

のように記述します。@Setの第一引数に指定する変数名は文字列で指定する必要があります。マクロを参照するには、$の後ろに変数名を指定します。例えば、上の例で作成したsubjectという名前の変数にアクセスするには、

 $subject

のように記述します。定義されていない変数を参照すると空文字列が返されます。また、定義されているかどうかわからない変数にアクセスするには、((<@Variable|URL:VariableFunction.html>))を使用することもできます。

変数にはローカル変数とグローバル変数があります。((<@Set|URL:SetFunction.html>))で変数を定義するときに第三引数に:GLOBALを指定するとグローバル変数になります。ローカル変数は定義した以降でそのマクロを評価し終わるまで有効です。グローバル変数は定義した以降で有効ですが、有効な範囲は使われる状況によって異なります。例えば、テンプレート内のマクロでグローバル変数を使用すると、テンプレート内の各マクロでグローバル変数は共有されます。

 {@Progn(@Set('local', Subject),
         @Set('global', Subject, :GLOBAL))}
 {$local}  # ここは空文字列
 {$global} # こちらはSubjectの値

また、振り分け時のマクロでは、その一回の振り分け中でグローバル変数が共有されます。

=end

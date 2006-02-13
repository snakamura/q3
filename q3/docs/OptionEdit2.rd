=begin
=エディットビュー2の設定

[オプション]ダイアログの[エディットビュー2]パネルではエディットビューの設定を行います。

((<エディットビュー2の設定|"IMG:images/OptionEdit2.png">))


====[整形]
((<整形機能|URL:Reform.html>))の設定を行います。


+[カラム数]
折り返しを行うカラム数を指定します。デフォルトでは74です。


+[送信前に自動的に整形する]
指定すると、エディットビューでメッセージを作成するときに自動的に整形するのがデフォルトになります。実際に整形するかどうかはエディットビューでその都度指定することができます。デフォルトでは自動的に整形します。


====[外部エディタ]
外部エディタの設定を行います。


+[デフォルトで外部エディタを使用する]
指定するとデフォルトで外部エディタを使用するようになります。実際には、((<MessageCreateアクション|URL:MessageCreateAction.html>))と((<MessageCreateExternalアクション|URL:MessageCreateExternalAction.html>))の動作を入れ替えます。デフォルトでは外部エディタは使用しません。

+[エディタ]
外部エディタを指定します。実行ファイルだけを指定すると、その実行ファイルに編集用のファイルのパスが引数として渡されます。引数に「%f」を含めるとその部分が編集用ファイルのパスに置き換わります。含めない場合には、引数の一番最後に編集用ファイルのパスが渡されます。

つまり以下のようになります（C:\temp\temp.txtは編集用のファイルだとします）。

:notepad.exe
  notepad.exe C:\temp\temp.txt
:editor.exe -f "%f" -m
  editor.exe -f "C:\temp\temp.txt" -m
:editor.exe -f
  editor.exe -f C:\temp\temp.txt

+[エディタが終了したら自動的にメッセージを作成する]
指定すると、エディタが終了したときに、編集用のファイルから自動的にメッセージを作成します。指定しない場合にはエディタが終了しても何もしませんので、エディタのマクロなどを使用して、((<コマンドラインオプション|URL:CommandLine.html>))などを使ってメッセージを作成する必要があります。


+[添付ファイルを圧縮する]
ファイルを添付するときに圧縮するのをデフォルトにします。実際に圧縮するかどうかはエディットビューでその都度指定することができます。デフォルトは圧縮しません。

添付ファイルを圧縮するにはzip32.dllが必要です。


+[セキュリティ]
セキュリティ関係の設定を行うダイアログを開きます。


===[セキュリティ]ダイアログ
セキュリティ関係の設定を行います。

((<セキュリティダイアログ|"IMG:images/SecurityDialog.png">))


====[S/MIME]
S/MIMEの設定を行います。


+[デフォルトで暗号化する]
デフォルトで暗号化するようになります。実際に暗号化するかはエディットビューでその都度指定することができます。デフォルトでは暗号化しません。


+[デフォルトで署名する]
デフォルトで署名するようになります。実際に署名するかはエディットビューでその都度指定することができます。デフォルトでは署名しません。


+[署名時にmultipart/signedを使う]
署名時にmultipart/signedでクリア署名をするかどうかを指定します。デフォルトではクリア署名します。


+[自分用に暗号化する]
作成したメールを自分用にも暗号化します。自分用に暗号化しないと、送信したメッセージを自分で読むことができなくなります。デフォルトでは自分用には暗号化しません。


====[PGP/GnuPG]
PGP/GnuPGの設定を行います。


+[デフォルトで暗号化する]
デフォルトで暗号化するようになります。実際に暗号化するかはエディットビューでその都度指定することができます。デフォルトでは暗号化しません。


+[デフォルトで署名する]
デフォルトで署名するようになります。実際に署名するかはエディットビューでその都度指定することができます。デフォルトでは署名しません。


+[PGP/MIMEを使う]
デフォルトでPGP/MIMEを使うか、インラインPGPを使うかを指定します。実際にPGP/MIMEを使うかどうかはエディットビューでその都度指定することができます。デフォルトではPGP/MIMEを使用します。

=end

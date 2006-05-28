=begin
=@AddressBook

 String @AddressBook(String to?, String cc?, String bcc?)


==説明
アドレス選択ダイアログを開きます。to, cc, bccの各引数にRFC2822形式のアドレスリストを指定するとそれらのアドレスが選択された状態でダイアログを開きます。アドレス選択ダイアログでOKをクリックして閉じると、選択されたアドレスがRFC2822のヘッダ形式で返されます。

アドレス選択ダイアログについては、((<ToolSelectAddressアクション|URL:ToolSelectAddressAction.html>))を参照してください。


==引数
:String to
  Toのアドレス
:String cc
  Ccのアドレス
:String bcc
  Bccのアドレス


==エラー
*引数の数が合っていない場合
*UIスレッド以外から呼び出した場合
*UIがない場合

==条件
*UIスレッドからのみ呼び出し可能
*UIが必要


==例
 # アドレス選択ダイアログを開く
 @AddressBook()
 
 # Bccに自分のアドレスを選択した状態でアドレス選択ダイアログを開く
 @AddressBook('', '', @I())

=end

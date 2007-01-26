=begin
=受信したメールの送信者をアドレス帳に登録した名前で表示するにはどうすれば良いですか?

==リストビューの[送信者 / 宛先]の欄にアドレス帳に登録した名前で表示する

(1)メニューから[表示]-[カラムをカスタマイズ]を選択し、((<[カラムのカスタマイズ]ダイアログ|URL:ViewsDialog.html>))を開きます。

(2)リストボックスで[送信者 / 宛先]の行を選択し、[編集]ボタンをクリックし、((<[カラム]ダイアログ|URL:ViewsColumnDialog.html>))を開きます。

(3)[タイプ]コンボボックスで「その他」を選択し、[マクロ]に以下のマクロを指定します
    @FormatAddress(From, :FORMAT-NAME, :LOOKUP-FORCE)
   ((<カラムのカスタマイズ|"IMG:images/ShowNameInAddressBook.png">))

(4)[フラグ]の[キャッシュ]にチェックが入っていることを確認します。

(5)順に[OK]ボタンを押してダイアログを閉じます。

:FORMAT-NAMEの代わりに:FORMAT-ALLを指定することで、名前とメールアドレスの両方を表示することもできます。詳細は、((<@FormatAddress|URL:FormatAddressFunction.html>))を参照してください。


==ヘッダビューの[送信者]の欄にアドレス帳に登録した名前で表示する

((<header.xml|URL:HeaderXml.html>))を編集します。以下のところを探し、

 <line class="mail|news">
  <static width="auto" style="bold" showAlways="true">送信者:</static>
  <edit background="{@If(@Not(@Param('Verify')), '', @Contain(@Param('Verify'), 'AddressMatch'), 'f5f6be', @Contain(@Param('Verify'), 'AddressMismatch'), 'ec7b95', '')}" number="4">{@FormatAddress(From, 3)}</edit>
  <static width="auto" style="bold" align="right" showAlways="true">日付:</static>
  <edit width="10em" number="5">{@FormatDate(@Date(Date), @Profile('', 'Global', 'DefaultTimeFormat'))}</edit>
 </line>

以下のように書き換えます。

 <line class="mail|news">
  <static width="auto" style="bold" showAlways="true">送信者:</static>
  <edit background="{@If(@Not(@Param('Verify')), '', @Contain(@Param('Verify'), 'AddressMatch'), 'f5f6be', @Contain(@Param('Verify'), 'AddressMismatch'), 'ec7b95', '')}" number="4">{@FormatAddress(From, :FORMAT-ALL, :LOOKUP-FORCE)}</edit>
  <static width="auto" style="bold" align="right" showAlways="true">日付:</static>
  <edit width="10em" number="5">{@FormatDate(@Date(Date), @Profile('', 'Global', 'DefaultTimeFormat'))}</edit>
 </line>

=end

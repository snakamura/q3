=begin
=@Param

 String @Param(String name)


==説明
コンテキストメッセージのパラメータを返します。指定された名前のパラメータが設定されていない場合には空文字列を返します。

以下のパラメータ名が取得できます。

:SignedBy
  S/MIMEの署名を検証したときに取得した署名したDN、またはPGPの署名を検証したときに取得した署名したユーザ名。
:Certificate
  S/MIMEの署名を検証したときに取得した証明書、またはPGPの署名を検証したときに取得した情報。
:Verify
  S/MIMEやPGPで署名を検証したときや復号した結果。以下の文字列を空白文字で区切った文字列になります。
  :Verified
    検証に成功した場合
  :AddressMatch
    アドレスがマッチした場合（検証に成功した場合のみ）
  :AddressMismatch
    アドレスがマッチしなかった場合（検証に成功した場合のみ）
  :VerifyFailed
    検証に失敗した場合
  :Decrypted
    復号した場合


==引数
:String name
  パラメータ名


==エラー
*引数の数が合っていない場合
*コンテキストメッセージがない場合
*メッセージの取得に失敗した場合


==条件
なし


==例
 # SignedByパラメータを取得
 @Param('SignedBy')
 
 # 署名が検証されたかどうかを調べる
 @Contain(@Param('Verify'), 'Verified')

=end

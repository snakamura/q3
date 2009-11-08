=begin
=S/MIME

QMAIL3ではS/MIMEによる暗号化と署名をサポートしています。


==必要なライブラリ
SSLを使用するには、OpenSSLのライブラリが必要です。インストーラでインストールした場合、[SSL, S/MIME]にチェックを入れていればOpenSSLのライブラリは既にインストールされています。その他の場合には、ダウンロードページからライブラリをダウンロードし、libeay32.dllとlibssl32.dllをq3u.exeと同じディレクトリに置きます。

また、qscryptou.dllがない場合には、配布ファイルからインストールする必要があります。


==証明書
S/MIMEで使用するためのルート証明書はデフォルトでシステムの証明書ストアからロードされます。詳細については、((<ルート証明書|URL:RootCertificate.html>))を参照してください。


==自分の秘密鍵と証明書
S/MIMEで署名をしたり暗号化されて送られてきたメールを復号化するには自分の秘密鍵と証明書をインストールする必要があります。自分の秘密鍵はPEM形式でアカウントのフォルダ（accounts/<アカウント名>）にkey.pemという名前で置いてください。証明書は同じフォルダに同じくPEM形式でcert.pemという名前で置いてください。

ただし、サブアカウントでIdentityを使っている場合には、ファイル名はそれぞれkey_<Identity名>.pemとcert_<Identity名>.pemになります。


==他の人の証明書
暗号化されたメールを送信するには受信者の証明書をインストールする必要があります。証明書はPEM形式にして任意のファイル名でメールボックスディレクトリ以下のsecurityディレクトリにおいてください。拡張子は.pemにします。さらに、((<アドレス帳|URL:AddressBook.html>))でその証明書を指定します。

たとえば、foo@example.comというアドレスの人の証明書をインストールするには、security/foo.pemに証明書をPEM形式でおき、アドレス帳の証明書の指定で「foo」と指定します。


==復号と署名の検証
復号や署名の検証を行うには、((<"[表示]-[モード]-[S/MIME]"|URL:ViewSMIMEModeAction.html>))にチェックを入れてS/MIMEモードをOnにします。S/MIMEモードをOnにするとメッセージを読み込むときにS/MIMEの復号や署名の検証が自動的に行われます。復号時に((<パスワード|URL:Password.html>))が必要な場合には、[パスワード]ダイアログが開きます。

メッセージを復号したり署名を検証するとステータスバーにアイコンが表示されます。

((<ステータスバーのアイコン|"IMG:images/SMIMEStatusBar.png">))

復号した場合には鍵マークが、署名を検証した場合にはチェックマークが、検証に失敗した場合には×マークが表示されます。チェックマークまたは×マークをクリックすると、署名したときに使用した証明書が表示されます。

((<検証結果|"IMG:images/SMIMECertificateDialog.png">))

また、署名の検証に成功した場合には、ヘッダビューのFromの行の背景色が薄い黄色に変わります。また、Signed byの行に、署名するのに使用された証明書のDNが表示されます。

((<ヘッダビュー|"IMG:images/SMIMEHeaderView.png">))


==暗号化と署名
メッセージを暗号化するには、エディットウィンドウで、((<"[ツール]-[S/MIME]-[暗号化]"|URL:ToolSMIMEEncryptAction.html>))にチェックを入れて暗号化するように設定します。同様に署名するには、((<"[ツール]-[S/MIME]-[署名]"|URL:ToolSMIMESignAction.html>))にチェックを入れて署名するように設定します。

これらのデフォルト値は、((<エディットビュー2の設定|URL:OptionEdit2.html>))で指定することができます。

署名時に((<パスワード|URL:Password.html>))が必要な場合には、[パスワード]ダイアログが開きます。


==設定
署名の形式としてapplication/pkcs7-mime形式を使用するかmultipart/signed形式を使用するか、暗号化するときに自分の鍵でも暗号化するかといったS/MIMEの設定は、((<エディットビュー2の設定|URL:OptionEdit2.html>))で行います。


==証明書や鍵ファイルの作り方
システムの証明書ストアからPKCS#12形式でエクスポートしたファイルはOpenSSLのコマンドを使用してPEM形式にすることができます。

 #CAの証明書の取得
 openssl pkcs12 -in example.p12 -nokeys -cacerts -out ca.pem
 # 自分の証明書の取得
 openssl pkcs12 -in example.p12 -nokeys -clcerts -out cert.pem
 # 自分の秘密鍵の取得
 openssl pkcs12 -in example.p12 -nocerts -nodes -out key.pem

=end

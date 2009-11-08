=begin
=ルート証明書

((<SSL|URL:SSL.html>))や((<"S/MIME"|URL:SMIME.html>))で使用するためのルート証明書はデフォルトでシステムの証明書ストアからロードされます。ただし、HPC Pro, HPC2000, Pocket PC 2000ではシステムの証明書ストアが利用できないため、QMAIL3の証明書ストアに証明書を追加する必要があります。

また、システムにデフォルトでインストールされていないルート証明書を使用する場合には、そのルート証明書をシステムの証明書ストアに取り込むか、QMAIL3の証明書ストアに追加する必要があります。


==システムの証明書ストア
システムの証明書ストアは、コントロールパネルの[インターネットオプション]の[コンテンツ]タブで[証明書]ボタンを押すと表示されます。証明書を追加するのはここから追加してください。

システムの証明書ストアから証明書を読み込むかどうかは、((<セキュリティの設定|URL:OptionSecurity.html>))で指定できます。


==QMAIL3の証明書ストア
システムの証明書ストアを使えないプラットフォームや環境のために、QMAIL3独自の証明書ストアがあります。QMAIL3の証明書ストアはテキストファイル形式でメールボックスディレクトリのsecurity/ca.pemというファイルになります。このファイルはデフォルトでは作成されませんので必要に応じて作成します。そして、必要なCAの証明書をPEM形式で格納してください。

ファイルの内容はたとえば以下のような形式になります。

 -----BEGIN CERTIFICATE-----
 MIICWjCCAcMCAgGlMA0GCSqGSIb3DQEBBAUAMHUxCzAJBgNVBAYTAlVTMRgwFgYD
 VQQKEw9HVEUgQ29ycG9yYXRpb24xJzAlBgNVBAsTHkdURSBDeWJlclRydXN0IFNv
 bHV0aW9ucywgSW5jLjEjMCEGA1UEAxMaR1RFIEN5YmVyVHJ1c3QgR2xvYmFsIFJv
 b3QwHhcNOTgwODEzMDAyOTAwWhcNMTgwODEzMjM1OTAwWjB1MQswCQYDVQQGEwJV
 UzEYMBYGA1UEChMPR1RFIENvcnBvcmF0aW9uMScwJQYDVQQLEx5HVEUgQ3liZXJU
 cnVzdCBTb2x1dGlvbnMsIEluYy4xIzAhBgNVBAMTGkdURSBDeWJlclRydXN0IEds
 b2JhbCBSb290MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQCVD6C28FCc6HrH
 iM3dFw4usJTQGz0O9pTAipTHBsiQl8i4ZBp6fmw8U+E3KHNgf7KXUwefU/ltWJTS
 r41tiGeA5u2ylc9yMcqlHHK6XALnZELn+aks1joNrI1CqiQBOeacPwGFVw1Yh0X4
 04Wqk2kmhXBIgD8SFcd5tB8FLztimQIDAQABMA0GCSqGSIb3DQEBBAUAA4GBAG3r
 GwnpXtlR22ciYaQqPEh346B8pt5zohQDhT37qw4wxYMWM4ETCJ57NE7fQMh017l9
 3PR2VX2bY1QY6fDq81yx2YtCHrnAlU66+tXifPVoYb+O7AWXX1uw16OFNMQkpw0P
 lZPvy5TYnh+dXIVtx6quTx8itc2VrbqnzPmrC3p/
 -----END CERTIFICATE-----
 
 -----BEGIN CERTIFICATE-----
 MIIDAjCCAmsCEDnKVIn+UCIy/jLZ2/sbhBkwDQYJKoZIhvcNAQEFBQAwgcExCzAJ
 BgNVBAYTAlVTMRcwFQYDVQQKEw5WZXJpU2lnbiwgSW5jLjE8MDoGA1UECxMzQ2xh
 c3MgMSBQdWJsaWMgUHJpbWFyeSBDZXJ0aWZpY2F0aW9uIEF1dGhvcml0eSAtIEcy
 MTowOAYDVQQLEzEoYykgMTk5OCBWZXJpU2lnbiwgSW5jLiAtIEZvciBhdXRob3Jp
 emVkIHVzZSBvbmx5MR8wHQYDVQQLExZWZXJpU2lnbiBUcnVzdCBOZXR3b3JrMB4X
 DTk4MDUxODAwMDAwMFoXDTE4MDUxODIzNTk1OVowgcExCzAJBgNVBAYTAlVTMRcw
 FQYDVQQKEw5WZXJpU2lnbiwgSW5jLjE8MDoGA1UECxMzQ2xhc3MgMSBQdWJsaWMg
 UHJpbWFyeSBDZXJ0aWZpY2F0aW9uIEF1dGhvcml0eSAtIEcyMTowOAYDVQQLEzEo
 YykgMTk5OCBWZXJpU2lnbiwgSW5jLiAtIEZvciBhdXRob3JpemVkIHVzZSBvbmx5
 MR8wHQYDVQQLExZWZXJpU2lnbiBUcnVzdCBOZXR3b3JrMIGfMA0GCSqGSIb3DQEB
 AQUAA4GNADCBiQKBgQCq0Lq+Fi24g9TK0g+8djHKlNgdk4xWArzZbxpvUjZudVYK
 VdPfQ4chEWWKfo+9Id5rMj8bhDSVBZ1BNeuS65bdqlk/AVNtmU/t5eIqWpDBucSm
 Fc/IReumXY6cPvBkJHalzasab7bYe1FhbqZ/h8jit+U03EGI6glAvnOSPWvndQID
 AQABMA0GCSqGSIb3DQEBBQUAA4GBAIv3GhDOdlwHq4OZ3BeAbzQ5XZg+a3Is4cei
 e0ApuXiIukzFo2penm574/ICQQxmvq37rqIUzpLzojSLtLK2JPLl1eDI5WJthHvL
 vrsDi3xXyvA3qZCviu4Dvh0onNkmdqDNxJ1O8K4HFtW+r1cIatCgQkJCHvQgzKV4
 gpUmOIpH
 -----END CERTIFICATE-----

Windowsの証明書ストアからPEM形式で証明書をエクスポートするには、形式として[Base 64 encoded X509 (.CER)]を選択します。複数の証明書をまとめてエクスポートするとPKCS#7形式しか選択できませんが、この場合エクスポートしたファイルをOpenSSLのコマンドでPEM形式に変換することができます。

 openssl pkcs7 -inform DER -in export.p7b -outform PEM -out export.pem -print_certs

export.p7bは証明書ストアからエクスポートしたファイルのファイル名です。export.pemがPEM形式のファイルになります。

=end

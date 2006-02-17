=begin
=Zipファイルによるインストール

Zipファイルによるインストールは、以下の手順で行います。

(1)Zipファイルを任意のディレクトリに展開します
(2)UIを日本語化したい場合には日本語UIのZipファイルを同じディレクトリに展開します
(3)Windows CE版の場合には、すべてのファイルをデバイスにコピーします

((<インストールディレクトリ|"IMG:images/InstallDirectory.png">))


含まれるファイルはコンポーネント別にDLLに分けられています。必要のないDLLは削除しても構いません。以下が各コンポーネントの機能です。プラットフォームによって一つ以上のDLLが含まれていない可能性があります。たとえば、Windows CE版ではPGP, GnuPGはサポートされていないので、qmpgpu.dllは含まれません。

:q3u.exe, qmu.dll, qsu.dll
  必ず必要です
:qmpop3u.dll
  POP3コンポーネント
:qmsmtpu.dll
  SMTPコンポーネント
:qmimap4u.dll
  IMAP4コンポーネント
:qmnntpu.dll
  NNTPコンポーネント
:qmrssu.dll
  RSS, Atomコンポーネント
:qmscriptu.dll
  スクリプトコンポーネント
:qmpgpu.dll
  PGP, GnuPGコンポーネント
:qscryptou.dll
  SSL, S/MIMEコンポーネント
:qsconvjau.dll
  英語環境で日本語を使用するためのコンポーネント


日本語UIのZipファイルに含まれるファイルは、各EXE, DLLの名前の後ろに「.0411.mui」が付加されています。


機能によっては追加のDLLが必要な場合があります。

*SSLやS/MIMEの機能を使う場合には((<OpenSSL|URL:http://www.openssl.org/>))のDLL (libeay32.dllとlibssl32.dll)が必要です
*（Windows版で）添付ファイルの圧縮機能を使う場合には((<zip32.dll|URL:http://www.info-zip.org>))が必要です

=end

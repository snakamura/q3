=begin
=@FolderFlag

 Boolean @FolderFlag(String folder, Number flag)


==説明
folderで指定されたフォルダがflagで指定したフラグを持っているかどうかを返します。

folderにはフラグを調べたいフォルダの完全名を指定します。指定されたフォルダが見つからないとエラーになります。

flagには調べたいフラグを指定します。指定できるのは以下のいずれかです。

::FF-NOSELECT
  選択可能かどうか。選択可能な場合にはFalseが、それ以外の場合にはTrueが返されます。IMAP4アカウントではメッセージを入れられないフォルダではこのフラグがTrueになります。
::FF-NOINFERIORS
  子フォルダを持つことができるかどうか。子フォルダを持つことができる場合にはFalseが、それ以外の場合にはTrueが返されます。IMAP4アカウントでは子フォルダを作成することができないフォルダではこのフラグがTrueになります。
::FF-CUSTOMFLAGS
  カスタムフラグをサポートするかどうか。IMAP4アカウントではサーバ上に任意のフラグを保存できる場合にはTrueが、それ以外の場合にはFalseが返されます。
::FF-NORENAME
  名前を変更することができるかどうか。名前を変更できる場合にはFalseが、それ以外の場合にはTrueが返されます。
::FF-LOCAL
  ローカルフォルダかどうか。ローカルフォルダの場合にはTrueが、それ以外の場合にはFalseが返されます。
::FF-HIDE
  隠されているかどうか。隠されている場合にはTrueが、それ以外の場合にはFalseが返されます。
::FF-CHILDOFROOT
  IMAP4アカウントで指定されたルートフォルダの子フォルダの場合にはTrueが、それ以外の場合にはFalseが返されます。
::FF-IGNOREUNSEEN
  未読を無視するかどうか。未読を無視する場合にはTrueが、それ以外の場合にはFalseが返されます。
::FF-INBOX
  受信箱かどうか。
::FF-OUTBOX
  送信箱かどうか。
::FF-SENTBOX
  送信控えかどうか。
::FF-TRASHBOX
  ゴミ箱かどうか。
::FF-DRAFTBOX
  草稿箱かどうか。
::FF-SEARCHBOX
  検索かどうか。
::FF-JUNKBOX
  スパムかどうか。
::FF-SYNCABLE
  同期可能かどうか。
::FF-SYNCWHENOPEN
  開いたときに同期するかどうか。
::FF-CACHEWHENREAD
  本文を読んだときにキャッシュするかどうか。


==引数
:String folder
  フォルダ名
:Number flag
  調べるフラグ


==エラー
*引数の数が合っていない場合
*指定されたフォルダが見つからない場合


==条件
なし


==例
 # 選択されているメッセージが格納されているフォルダが受信箱かどうかを調べる
 @FolderFlag(@Folder(), :FF-INBOX)
 
 # 現在のフォルダが同期可能かどうか調べる
 @FolderFlag(@Folder(:FN-FULLNAME, :FT-CURRENT), :FF-SYNCABLE)

=end

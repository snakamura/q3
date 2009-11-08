=begin
=@SpecialFolder

 String @SpecialFolder(Number type, String account?)


==説明
特殊フォルダの完全名を取得します。

typeには取得したい特殊フォルダのタイプを指定します。以下のいずれかが指定できます。

::SF-INBOX
  受信箱
::SF-OUTBOX
  送信箱
::SF-SENTBOX
  送信済み
::SF-TRASHBOX
  ゴミ箱
::SF-DRAFTBOX
  草稿箱
::SF-SEARCHBOX
  検索
::SF-JUNKBOX
  スパム

accountにアカウント名を指定すると指定したアカウントの特殊フォルダを取得します。それ以外の場合にはコンテキストアカウントの特殊フォルダを取得します。


==引数
:Number type
  取得する特殊フォルダのタイプ
:String account
  アカウント


==エラー
*引数の数が合っていない場合
*コンテキストアカウントがない場合（アカウントを指定しなかった場合）
*指定されたアカウントが見つからない場合（アカウントを指定した場合）
*指定されたタイプが不正な場合


==条件
なし


==例
 # 受信箱のフォルダ名を取得
 @Folder(:SF-INBOX)
 
 # Testという名前のアカウントのスパムフォルダの名前を取得
 @Folder(:SF-JUNKBOX, 'Test')

=end

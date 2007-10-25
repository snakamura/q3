=begin
=リリースノート

==3.0.2
===注意点
ライセンスが変更されました。変更後のライセンスは、((<ライセンス|URL:license.html>))を参照してください。

===新機能
*巡回の設定を使ってバックグラウンドで振り分けることができるようにした
*MessageApplyRuleBackgroundアクションとMessageApplyRuleBackgroundAllアクションを追加

===変更
*STLport-5.1.4に更新
*OpenSSL 0.9.8gに更新

===修正
*同期終了時のfolders.xmlの保存でエラーが発生することがあるのを修正
*同じフォルダを同時に同期しないための仕組みが正しく動いていなかったのを修正
*msvcp80.dllがインストールされず、起動できないことがあったのを修正


==3.0.1
===注意点
このバージョンで、Windows版の実行に必要なランタイムライブラリが更新されています。((<ダウンロードページ|URL:download.html>))からランタイムライブラリをダウンロードしてインストールしてください。

===新機能
*Windows Mobile版にランドスケープ用のダイアログを追加した
*コマンドラインで-fが指定された場合には、ロックファイルが検出されてもそのまま続行するようにした
*GnuPGでパスワードを尋ねるときにユーザIDを表示するようにした
*GnuPGで暗号化するときにBccや、ToもしくはCcにグループアドレスが含まれ、そのアドレスがFrom, Sender, Reply-Toのいずれにも現れない場合には匿名で暗号化するようにした（PGP/HiddenRecipientで有効・無効を切り替え可能）
*スパムフィルタで一部の添付ファイルの中身もスキャンできるようにした（xdoc2txtが必要）
*自動巡回のエントリを一時的に無効にできるようにした
*振り分けルールのエントリを一時的に無効にできるようにした
*ToolSubAccountアクションをメッセージウィンドウからも使えるようにした
*動的メニューのToolSubAccountをメッセージウィンドウからも使えるようにした

===変更
*Windows Mobile 5.0版のメニューをソフトキー対応にした
*ウィンドウを隠した状態から復帰するときにメッセージウィンドウも復帰させるようにした
*multipart/appledoubleの下にあるapplication/applefileは添付ファイルとして扱わないようにした
*同期終了時にfolders.xmlも保存するようにした
*スパムフィルタでmessage/rfc822パートの内部もスキャンするようにした
*STLport-5.1.3に更新（stlport.5.1.dllと（Windows Mobile 5.0版では）msvcr80.dllが必要）
*boost-1.34.0に更新

===修正
*ヘッダビューの表示・非表示を切り替えたときにメッセージを表示しなおすようにした
*RSSやAtomを処理したときにリンク先のURLやIDが空だと、次回起動したときに全てのフィードを取得しなおしてしまう事があるのを修正
*IMAP4アカウントで本文をキャッシュしていないときに、ネストしたマルチパートを正しく復元できないことがあるのを修正
*添付ファイルを開くときに、警告する設定の拡張子でも警告が出ないことがあるのを修正


==3.0.0
*2.9.33からの変更点はありません。
*このバージョンで以下のプラットフォームのサポートは終了します。
  *Windows 95/98/98SE/Me/NT 4.0
  *Windows Mobile 2003 SE
  *Windows Mobile 2003
  *Sigmarion III
  *Pocket PC 2002
  *Pocket PC
  *HPC2000
  *HPC Pro

=end

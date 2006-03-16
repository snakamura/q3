=begin
=スレッド表示

QMAIL3ではメッセージをスレッド表示することができます。サポートされているスレッド表示は2種類あります。一つ目は通常のスレッド表示で、もう一つはフローティングスレッド表示です。

通常のスレッド表示では、スレッド同士はスレッドの先頭のメッセージでソートされますが、フローティングスレッド表示では、スレッド内で最も条件を満たすメッセージでソートされます。たとえば、日付でソートした場合には、スレッド内の最も新しいメッセージでソートされます。

スレッド表示は以下のアクションにより切り替えることができます。

:((<ViewSortFlatアクション|URL:ViewSortFlatAction.html>))
  スレッド表示をやめて全てのメッセージをフラットに表示します。

:((<ViewSortThreadアクション|URL:ViewSortThreadAction.html>))
  通常のスレッド表示にします。

:((<ViewSortFloatThreadアクション|URL:ViewSortFloatThreadAction.html>))
  フローティングスレッド表示にします。

:((<ViewSortToggleThreadアクション|URL:ViewSortToggleThreadAction.html>))
  フラット表示とスレッド表示を切り替えます。

=end

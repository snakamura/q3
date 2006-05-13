=begin
=RSSアカウントのパラメータ

RSSアカウントのフォルダのパラメータは以下のとおりです。これらのパラメータはフィードのフォルダ（同期可能なフォルダ）にのみ設定できます。

:URL
  フィードのURLを指定します。
:UserName
  フィードにアクセスするのにBasic認証が必要な場合のユーザ名を指定します。Basic認証が必要ない場合には空にします。
:Password
  フィードにアクセスするのにBasic認証が必要な場合のパスワード。Basic認証が必要ない場合には空にします。
:MakeMultipart
  フィードのエントリが<content:encoded>タグを含む場合にこれを保存するかどうかを指定します。「true」を指定すると保存し、「false」を指定すると保存しません。保存した場合、そのエントリを選択するとWebページを取得する代わりに保存したコンテンツを表示します。
:UseDescriptionAsContent
  MakeMultipartをtrueにしてコンテンツを保存する場合に、<content:encoded>タグの代わりに<description>タグを使用するかどうかを指定します。「true」を指定すると<description>タグを使用し、「false」を指定すると使用しません。<description>タグの中にHTMLのコンテンツが含まれる場合に指定します。
:UpdateIfModified
  エントリに変更があった場合に取得しなおすかどうかを指定します。「true」を指定すると、エントリに変更があった場合には取得しなおします。「false」を指定すると、同じID（またはURL）のエントリは一度しか取得しません。
:Cookie
  指定したCookieを送りたい場合に、「name=value」の形で指定します。

=end

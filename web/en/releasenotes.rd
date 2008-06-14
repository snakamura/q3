=begin
=Release Notes

==3.0.5
===New features
*Allow to specify the directory where a junk database is placed by JunkFilter/Path in qmail.xml.
*Added Imap4/SubscribeOnly to account.xml.

===Changes
*Prohibit dropping messages dragged from list view to list view.
*Updated OpenSSL to 0.9.8h

===Bug fixes
*Fixed a file name became ".eml" when attaching a message whose subject was empty.
*Fixed a message count was not fixed up after crash.
*Fixed configuration was reset when it failed to reload a configuration file.
*Fixed SMTP PLAIN authentication didn't work.
*Fixed @UnseenMessageCount became @MessageCount after it was saved.


==3.0.4
===New features
*Allow to open a message in message/rfc822 format in message window.
*Added FileOption action to open a specified file in message window.
*Added -o command line option.
*Added ToolInsertMacro action.
*Added ToolApplyTemplate action.
*ToolAddAddress action now adds X.509 certificate to an address book when a message is signed with S/MIME.
*Added [Regular Expression] check box to simple search dialog.
*Made a phone number a link.
*Made messages and attachment files to be able to be droped on applications other than Explorer.
*Added Global/PrintCommand to qmail.xml to specify the command to print a message.
*Show erro details in a sync dialog when an error occurs at socket or SSL.
*Show an error in a sync dialog when an error occurs with an operation in online mode at IMAP4 or NNTP accounts.

===Changes
*Disconnect from POP3 server before applying a spam filter and rules.
*Reuse an IMAP4 session to apply rules.
*Decode as the platform's default encoding when processing mailto URLs if it is not a proper UTF-8 string.
*Select the first account after account dialog is closed if no account was selected.
*Do not allow to get messages from an account other than the current context account using @Messages from threads other than UI thread.
*Give preference to Atom over RSS while performing RSS Auto Discovery.

===Bug fixes
*Fixed it got the same messages more than once from POP3 server when an error occured while processing QUIT command.
*Fixed it sometimes refreshed all messages in a folder in IMAP4 account.
*Fixed it didn't add X-QMAIL-Account when saving a message in edit window.
*Fixed it added a wrong X-QMAIL-Signature when a signature was not set.
*Fixed X-QMAIL-Signature was not processed when creating a message using an external editor.
*Fixed UI brocked when a message count was changed in background threads.
*Fixed cursor was not updated at message view and edit view when it scrolled.
*Fixed Tab didn't work in option dialog when editable combo box had a focus.
*Fixed multiple default buttons might exist at option dialog.
*Fixed experting a message using DnD might made a file whose name was device name.


==3.0.3
===New features
*Added ViewSelectFolder action.
*Added @MessageCount and @UnseenMessageCount.
*Specify log level with command line.
*Overwrite the system association using qmail.xml when it opens URL.

===Changes
*Show preamble and epilogue when it shows a whole message.
*Allow invalid Content-ID when showing images embedded in HTML.
*Do not start auto pilot when an edit window is opened.
*Auto pilot as soon as possible after it failed to auto pilot because a dialog was shown.
*Allow applying a next rule after copying messages.
*Allow calling @I() from background thread when the both two arguments are specified.
*Divide CamelCase text into multiple tokens when spam filter is applied.

===Bug fixes
*Fixed it might cause crush when it parsed a multipart message.
*Fixed a day of week was wrong when formatting date with @FormatDate using %W2 and %W3.
*Fixed it showed a wrong message after deleting messages more than the count specified at Global/IndexMaxSize in account.xml
*Fixed it deleted the next character of META tag when there was META tag which specified charset.


==3.0.2
===Notes
License has been changed. Please visit ((<License|URL:license.html>)) to see the new license.

===New features
*Enable applying rules in background using go-round settings.
*Add MessageApplyRuleBackground and MessageApplyRuleBackgroundAll action.

===Changes
*Update to STLport-5.1.4.
*Update to OpenSSL 0.9.8g.

===Bug fixes
*Fixed it caused an error when saving folders.xml after synchronizing.
*Fixed the locking mechanism to prohibit synchronizing the same folders didn't work.
*Fixed it might not launch because msvcp80.dll was not installed.


==3.0.1
===Notes
Runtime libraries which are required to run Windows version have been updated. Please download the new runtime libraries from ((<download page|URL:download.html>)) and install them.

===New features
*Add landscape dialogs for Windows Mobile version.
*With command line option -f, it continues launching even after a lock file is detected.
*Show user id when asking a password for GnuPG
*When encrypting with GnuPG, use anonymous encryption when a messages has addresses in Bcc or group addresses in To or Cc, and they don't appear in From nor Sender nor Reply-To (You can disable this feature by edithing PGP/HiddenRecipient in qmail.xml).
*Spam filter scans attachments (It requires xdoc2txt)
*Supports to disable entries of auto pilot.
*Supports to disable entries of rules.
*ToolSubAccount action can be now used in message windows.
*ToolSubAccount dynamic menu can be now used in message windows.

===Changes
*Supports soft menu for Windows Mobile 5.0 version.
*Resotre message windows when recovering from hidden state.
*Don't treat application/applefile parts under multipart/appledouble aprts as attachments.
*Save folders.xml after finishing synchronization.
*Spam filter scans inside message/rfc822 parts.
*Update to STLport-5.1.3. (It needs stlport.5.1.dll and msvcr80.dll (Windows Mobile 5.0 only))
*Update to boost-1.34.0.

===Bug fixes
*Refresh message after showing or hiding a header view.
*Fixed that it got all feeds again after retrieving RSS or Atom which has an item without URL nor ID.
*Fixed that some multipart messages was not built correctly when IMAP4 account doesn't cache them.
*Fixed that it didn't worn when it opened attachment files with certain extensions which should be warned.


==3.0.0
*No changes from 2.9.33.
*These platforms will no longer be supported.
  *Windows 95/98/98SE/Me/NT 4.0
  *Windows Mobile 2003 SE
  *Windows Mobile 2003
  *Sigmarion III
  *Pocket PC 2002
  *Pocket PC
  *HPC2000
  *HPC Pro

=end

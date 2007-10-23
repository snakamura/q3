=begin
=Release Notes

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
*Update to STLport-5.1.3 (It needs stlport.5.1.dll and msvcr80.dll (Windows Mobile 5.0 only))
*Update to boost-1.34.0

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

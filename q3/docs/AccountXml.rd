=begin
=account.xml

�A�J�E���g���Ƃ̐ݒ��ۑ�����XML�t�@�C���ł��B���̃t�@�C���Őݒ�ł��鑽���̍��ڂ�((<�A�J�E���g�̃v���p�e�B|URL:AccountProperty.html>))�ȂǂŐݒ�ł��܂����A�ꕔ�̍��ڂ͒��ڂ��̃t�@�C����ҏW���Ȃ��Ɛݒ�ł��܂���B�ݒ�ł��鍀�ڂ̈ꗗ�͔��l���Q�Ƃ��Ă��������B


==����
������((<qmail.xml|URL:QmailXml.html>))�Ɠ����ł��̂ł�������Q�Ƃ��Ă��������B


==�T���v��

 <?xml version="1.0" encoding="utf-8"?>
 <profile>
  <section name="Global">
   <key name="Class">mail</key>
   <key name="SenderAddress">taro@example.org</key>
   <key name="SenderName">Taro Yamada</key>
  </section>
  <section name="Receive">
   <key name="Host">mail.example.org</key>
   <key name="Port">110</key>
   <key name="Type">pop3</key>
   <key name="UserName">taro</key>
  </section>
  <section name="Send">
   <key name="Host">mail.example.org</key>
   <key name="Port">25</key>
   <key name="Type">smtp</key>
  </section>
 </profile>


==�X�L�[�}
�X�L�[�}��((<qmail.xml|URL:QmailXml.html>))�Ɠ����ł��̂ł�������Q�Ƃ��Ă��������B


==���l
���̃t�@�C���ł̓Z�N�V�����ƃL�[�Œl���w�肵�܂��B�Ⴆ�΁A��̗�ł�Global�Z�N�V������Class�L�[��mail�Ƃ����l���w�肳��Ă��܂��B���̃h�L�������g���ł͂����Global/Class�̂悤�ɋL�q���Ă��邱�Ƃ�����܂��B

���ꂼ��̃L�[�̓f�t�H���g�̒l�������Ă��āA�w�肳��Ă��Ȃ��ꍇ�ɂ͂��̒l���g�p����܂��B�܂��A�l���f�t�H���g�̒l�Ɠ����ꍇ�ɂ̓t�@�C���ɂ͏����o����܂���B���݂��Ȃ��L�[�̒l���w�肷��ꍇ�ɂ́A�V�����Z�N�V������L�[��ǉ����Ă��������B

�w��ł���Z�N�V�����ƃL�[�͈ȉ��̒ʂ�ł��B


===Dialup�Z�N�V����
�_�C�A���A�b�v�̐ݒ�����܂��B

+DisconnectWait (0)
�_�C�A���A�b�v��ؒf����܂ł̑҂����ԁB�P�ʂ͕b�B


+Entry
�_�C�A���A�b�v�̃G���g�����B


+ShowDialog (0 @ 0|1)
�_�C�A���A�b�v���Ƀ_�C�A���O��\�����邩�ǂ����B


+Type (0 @ 0|1|2)
�_�C�A���A�b�v�̃^�C�v�B

:0
  �_�C�A���A�b�v���Ȃ�
:1
  �l�b�g���[�N�ڑ�����Ă��Ȃ��Ƃ������_�C�A���A�b�v����
:2
  ��Ƀ_�C�A���A�b�v����


===FullTextSearch�Z�N�V����
�S�������̐ݒ�����܂��B

+Index
�C���f�b�N�X�̂���f�B���N�g���B��̏ꍇ�ɂ̓A�J�E���g�f�B���N�g����index�f�B���N�g���B


===Global�Z�N�V����
�S�ʓI�Ȑݒ�����܂��B

+AddMessageId (1 @ 0|1)
Message-Id��t�����邩�ǂ����B


+AutoApplyRules (0 @ 0|1)
�����U�蕪����L���ɂ��邩�ǂ����B


+BlockSize (0)
���b�Z�[�W�{�b�N�X�̃u���b�N�T�C�Y�B

:0
  �ꃁ�b�Z�[�W��t�@�C��
:-1
  �����Ȃ�
:����ȊO
  �w�肳�ꂽ���l�ŕ����i�P�ʂ�MB�j


+Class
�A�J�E���g�N���X�B


+Identity
�T�u�A�J�E���g�̓��ꐫ�B


+IndexBlockSize (-1)
�C���f�b�N�X�̃u���b�N�T�C�Y�B-1�ŕ������Ȃ��B�P�ʂ�MB�B


+IndexMaxSize (-1)
�������ɃL���b�V������C���f�b�N�X�̐��B-1�Ŗ������B


+LogTimeFormat (%Y4/%M0%D-%h:%m%s%z)
�ʐM���O�̓��t�t�H�[�}�b�g�B�w����@�́A((<@FormatDate|URL:FormatDateFunction.html>))���Q�ƁB


+MessageStorePath
���b�Z�[�W��ۑ�����f�B���N�g���B��̏ꍇ�ɂ̓A�J�E���g�f�B���N�g���B


+ReplyTo
Reply-To�ɐݒ肷��A�h���X�B


+SenderAddress
From�ɐݒ肷��A�h���X�B


+SenderName
From�ɐݒ肷�閼�O�B


+ShowUnseenCountOnWelcome (1 @ 0|1)
Windows XP�̂悤������ʂɖ��ǃ��b�Z�[�W����\�����邩�ǂ����B


+SslOption (0)
SSL�̃I�v�V�����B�w��ł���l�́A((<SSL|URL:SSL.html>))���Q�ƁB


+StoreDecodedMessage (0 @ 0|1)
S/MIME��PGP�ŕ����������b�Z�[�W��S�������p�ɕۑ����邩�ǂ����B


+SubAccount
���݂̃T�u�A�J�E���g�B


+Timeout (60)
�^�C���A�E�g�B�P�ʂ͕b�B


+TrasnferEncodingFor8Bit ( @ 8bit, base64, quoted-printable)
8bit�̕������G���R�[�h������@�B�w�肵�Ȃ��ꍇ�ɂ͕����R�[�h�ɂ���Č��܂�B


+TreatAsSent (1 @ 0|1)
��M�������b�Z�[�W��From�������̃A�h���X�������ꍇ�ɑ��M�ς݂Ƃ��Ĉ������ǂ����B


===Http�Z�N�V����
HTTP�Ɋւ���ݒ�����܂��BRSS�A�J�E���g�Ŏg�p����܂��B

+UseProxy (0 @ 0|1)
�v���L�V���g�����ǂ����B


+UseInternetSetting
�C���^�[�l�b�g�̃v���p�e�B�Ŏw�肵���v���L�V�̐ݒ���g�����ǂ����B


+ProxyHost, ProxyPort (8080), ProxyUserName, ProxyPassword
�v���L�V�̃z�X�g���A�|�[�g�A���[�U���A�p�X���[�h�B


===Imap4�Z�N�V����
IMAP4�Ɋւ���ݒ�����܂��BIMAP4�A�J�E���g�Ŏg�p����܂��B

+AdditionalFields
�T�[�o���烁�b�Z�[�W�̃C���f�b�N�X���擾����Ƃ��ɒǉ��Ŏ擾����w�b�_���B�󔒂ŋ�؂��Ďw��B


+AuthMethods
�g�p��������F�ؕ����B


+CloseFolder (0 @ 0|1)
�t�H���_�����Ƃ��ɍ폜�}�[�N�̕t�������b�Z�[�W���폜���邩�ǂ����B


+FetchCount (100)
�C���f�b�N�X���擾����Ƃ��Ɉ��̃��N�G�X�g�Ŏ擾���郁�b�Z�[�W�̐��B


+ForceDisconnect (0)
�����I�ɃZ�b�V������ؒf����܂ł̑҂����ԁB�P�ʂ͕b�B

�Z�b�V�������g���Ƃ��ɂ����Ŏw�肵�����Ԉȏ�ɃA�C�h����Ԃ��������Z�b�V�����͋����I�ɐؒf����܂��B0���w�肷��Ɛؒf���܂���B�Ⴆ�΁ANAT���g���Ă���ꍇ�AIMAP4�T�[�o���ڑ���؂�O��NAT�̕ϊ��e�[�u�����N���A����Ă��܂��ƁA�ؒf���ꂽ�̂����o�ł��Ȃ����ߕ��ʂɐؒf����̂Ɏ��Ԃ�������܂��B���̏ꍇ�ANAT�̃e�[�u�����N���A����鎞�Ԃ����Z�����Ԃ������Ɏw�肷��ƁA����ȏ�A�C�h����Ԃ�������ڑ��������I�ɐؒf����̂Ŏ��Ԃ������邱�Ƃ��Ȃ��Ȃ�܂��B


+MaxSession (5)
�I�����C�����[�h�Ŏg�p����Z�b�V�����̍ő吔�B


+Option (255)
�I�v�V�����B�ȉ��̑g�ݍ��킹���\�i���Ŏw�肷��B

:0x01
  ENVELOPE���g��
:0x02
  BODYSTRUCTURE����Ɏg��
:0x04
  BODYSTRUCTURE��M������


+OutboxFolder (Outbox), DraftFolder (Outbox), SentboxFolder (Sentbox), TrashFolder (Trash), JunkFolder (Junk)
���M���A���e���A���M�ς݁A�S�~���A�X�p���t�H���_�Ƃ��Ďg���t�H���_���B


+Reselect (1 @ 0|1)
�����������Ԃ����O�Ƀt�H���_��I�����Ă����ꍇ�ɑI�����Ȃ������ǂ����B

IMAP4�T�[�o�ɂ���Ắi�Ⴆ�΁ACourier-IMAP�T�[�o�j�A�Z�b�V����A���t�H���_1��I��������ŃZ�b�V����B���t�H���_1�Ƀ��b�Z�[�W��ǉ������肷��ƁA�Z�b�V����A����̓t�H���_��I�����Ȃ����܂Œǉ��������b�Z�[�W�������܂���BReselect��1���w�肷��ƁA�t�H���_��I���������Ԃ������������Ԃ����O�̏ꍇ�ɂ̓t�H���_��I�����Ȃ����܂��B


+RootFolder
���[�g�t�H���_�B


+RootFolderSeparator (/)
���[�g�t�H���_�̃Z�p���[�^�B


+SearchCharset
�������Ɏg�p���镶���R�[�h�B�f�t�H���g�ł͎�������B


+SearchUseCharset (1)
�������ɕ����R�[�h���w�肷�邩�ǂ����B


+UseNamespace (0)
�l�[���X�y�[�X���g�����ǂ����B


+UsePersonal (1), UseShared (1), UseOthers (1)
�l�[���X�y�[�X���g���Ƃ��ɁA�p�[�\�i���A���L�A���̑��̃t�H���_�������邩�ǂ����B


===JunkFilter�Z�N�V����
�X�p���t�B���^�̐ݒ�����܂��B

+Enabled (0 @ 0|1)
�X�p���t�B���^���L�����ǂ����B


===Misc�Z�N�V����

+IgnoreError (0 @ 0|1)
�G���[���N���Ă��������邩�ǂ����B


===Nntp�Z�N�V����
NNTP�Ɋւ���ݒ�����܂��BNNTP�A�J�E���g�Ŏg�p����܂��B

+ForceDisconnect (0)
�����I�ɃZ�b�V������ؒf����܂ł̑҂����ԁB�P�ʂ͕b�B

�ڍׂ�Imap4�Z�N�V�����̓��������Q�ƁB


+InitialFetchCount (300)
���߂ă��b�Z�[�W���擾����Ƃ��Ɏ擾���郁�b�Z�[�W���B


+UseXOVER (1 @ 0|1)
XOVER�R�}���h���g�p���邩�ǂ����B


+XOVERStep (100)
XOVER�R�}���h�ŃC���f�b�N�X���擾����Ƃ��Ɉ��̃��N�G�X�g�Ŏ擾���郁�b�Z�[�W�̐��B


===Pop3�Z�N�V����
POP3�Ɋւ���ݒ�����܂��BPOP3�A�J�E���g�Ŏg�p����܂��B

+Apop (0 @ 0|1)
APOP���g�p���邩�ǂ����B


+DeleteBefore (0)
��M���Ă���T�[�o��̃��b�Z�[�W���폜����܂ł̓����B0�̏ꍇ�ɂ͍폜���Ȃ��B


+DeleteLocal (0 @ 0|1)
�T�[�o��̃��b�Z�[�W���폜����Ƃ��Ƀ��[�J���̃��b�Z�[�W���폜���邩�ǂ����B


+DeleteOnServer (0 @ 0|1)
��M�������b�Z�[�W���T�[�o����폜���邩�ǂ����B


+GetAll (20)
UIDL��LIST�̃��N�G�X�g���܂Ƃ߂ďo��臒l�B

UIDL��LIST�́A���ׂẴ��b�Z�[�W�̕����܂Ƃ߂Ď擾������@�ƁA�e���b�Z�[�W�ŌʂɎ擾������@������܂��B�T�[�o��Ɋ��Ɏ�M�ς݂̃��b�Z�[�W�������ꍇ�ɂ͊e���b�Z�[�W�ŌʂɎ擾����ق��������Ȃ�܂��B�T�[�o���x�ʂ̃��b�Z�[�W������A�T�[�o��ɂ�����Ɏ�M�ς݂̃��b�Z�[�W��y�ʂ������ꍇ�ɁAx/(x - y)�������Ŏw�肵�����l�𒴂����ꍇ�ɂ͂܂Ƃ߂Ď擾���܂��B


+HandleStatus (0 @ 0|1)
���b�Z�[�W��Status: RO�̃w�b�_���t���Ă����Ƃ��Ɋ��ǂɂ��邩�ǂ����B


+SkipDuplicatedUID (0 @ 0|1)
UIDL���������b�Z�[�W�������I�ɖ������邩�ǂ����B

POP3�ŃT�[�o��̃��b�Z�[�W�𖢓ǊǗ�����Ƃ��ɂ́A�O��Ō�Ɏ�M�������b�Z�[�W��UID�������b�Z�[�W��T���A����ȍ~�̃��b�Z�[�W���擾���܂��B���̂��߁A���̃��b�Z�[�W�������Ɋ��Ɏ�M�������b�Z�[�W������Ɠ�d�Ɏ�M���邱�Ƃ�����܂��BSkipDuplicatedUID��1�ɂ���Ɠ���UID�������b�Z�[�W�͂ǂ��Ɍ���Ă��������܂��B���R�قȂ郁�b�Z�[�W��UID���T�[�o��œ����ɂȂ��Ă��܂����ꍇ�ɂ���M���Ȃ��Ȃ�܂��̂Œ��ӂ��Ă��������B


===Pop3Send�Z�N�V����
���M�p��POP3�Ɋւ���ݒ�����܂��B���M�v���g�R���Ƃ���POP3 (XTND XMIT)��I�񂾏ꍇ�Ɏg�p����܂��B

+Apop (0 @ 0|1)
APOP���g�p���邩�ǂ����B


===Receive�Z�N�V����
��M�Ɋւ����ʓI�Ȑݒ�����܂��B

+Host
��M�p�T�[�o�̃z�X�g���B


+Port
��M�p�T�[�o�̃|�[�g�B


+Log (0 @ 0|1)
���O����邩�ǂ����B


+Secure (0 @ 0|1|2)
SSL�̐ݒ�B

:0
  SSL���g�p���Ȃ�
:1
  SSL���g�p����
:2
  STARTTLS���g�p����


+SyncFilterName
�����t�B���^�̖��O�B


+Type
��M�v���g�R���B


+UserName
��M�p�T�[�o�̃��[�U���B


===Send�Z�N�V����
���M�Ɋւ����ʓI�Ȑݒ�����܂��B

+Host
���M�p�T�[�o�̃z�X�g���B


+Port
���M�p�T�[�o�̃|�[�g�B


+Log (0 @ 0|1)
���O����邩�ǂ����B


+Secure (0 @ 0|1|2)
SSL�̐ݒ�B�w��ł���l�́AReceive/Secure�Ɠ����B


+Type
���M�v���g�R���B


+UserName
���M�p�T�[�o�̃��[�U���B


===Smtp�Z�N�V����
SMTP�Ɋւ���ݒ�����܂��B���M�v���g�R���Ƃ���SMTP��I�񂾏ꍇ�Ɏg�p����܂��B

+AuthMethods
SMTP�F�؂Ŏg�p��������F�ؕ����B


+EnvelopeFrom
EnvelopeFrom�Ƃ��Ďg�����[���A�h���X�B�w�肵�Ȃ��ꍇ�ɂ�From�܂���Sender�̃A�h���X���g�p�����B


+LocalHost
EHLO�܂���HELO�ő�����z�X�g���B�w�肵�Ȃ��ꍇ�ɂ́A���݂̃z�X�g�̖��O�B

+PopBeforeSmtp (0 @ 0|1)
POP before SMTP���g�����ǂ����B


+PopBeforeSmtpWait
POP before SMTP��POP3�ŔF�؂������SMTP�ő��M����܂ł̑҂����ԁB


+PopBeforeSmtpCustom (0 @ 0|1)
POP before SMTP�ŃJ�X�^���ݒ���g�����ǂ����B


+PopBeforeSmtpProtocol (pop3 @ pop3|imap4)
POP before SMTP�ŃJ�X�^���ݒ�̎��̃v���g�R���B

+PopBeforeSmtpHost
POP before SMTP�ŃJ�X�^���ݒ�̎��̃z�X�g���܂���IP�A�h���X�B


+PopBeforeSmtpPort
POP before SMTP�ŃJ�X�^���ݒ�̎��̃|�[�g�B


+PopBeforeSmtpSecure (0 @ 0|1|2)
POP before SMTP�ŃJ�X�^���ݒ�̎���SSL���g�����ǂ����B�w��ł���l�́AReceive/Secure�Ɠ����B

+PopBeforeSmtpApop (0 @ 0|1)
POP before SMTP�ŃJ�X�^���ݒ�̎���APOP���g�����ǂ����B�v���g�R����pop3�̏ꍇ�̂݁B


===UI�Z�N�V����

+FolderTo
���b�Z�[�W�̈ړ��_�C�A���O�Ńf�t�H���g�őI�������t�H���_�B

=end

=begin
=�R�}���h���C��

QMAIL3�Ɏw��\�ȃR�}���h���C���͈ȉ��̒ʂ�ł��B

-d��-p�͏�Ɏw��\�ł��B-g, -s, -a, -c, -r, -i�͔r���ł��B�������A-s��-a�͑g�ݍ��킹�Ďw�肷�邱�Ƃ��ł��܂��B-q�́A-g, -c, -r, -i�Ƒg�ݍ��킹�Ďw�肷�邱�Ƃ��ł��܂��B

�R�}���h���C���I�v�V�����̈����ɋ󔒕������܂܂��ꍇ�ɂ�""�Ŋ���܂��B^��u���Ǝ��̕������G�X�P�[�v����ē���ȈӖ��������܂��B�Ⴆ�΁A

 -d "C:\Documents and Settings\test\"
 -i "MessageCreate ^"Test Template^""

�̂悤�Ɏg�p���܂��B


==-d <���[���{�b�N�X�f�B���N�g��>

===����
���[���{�b�N�X�f�B���N�g�����w�肵�܂��B�ʏ�A���[���{�b�N�X�f�B���N�g���̃p�X�̓��W�X�g������擾���܂����A���̃I�v�V�������w�肷��Ǝw�肳�ꂽ�p�X�����[���{�b�N�X�f�B���N�g���Ƃ��Ďg�p���܂��B�قȂ郁�[���{�b�N�X�f�B���N�g�����w�肷���QMAIL3�𕡐��N�����邱�Ƃ��ł��܂��iWindows�ł̂݁j�B���[���{�b�N�X�f�B���N�g�����w�肵���ꍇ�A-p�Ńv���t�@�C�������w�肵�Ȃ����背�W�X�g������v���t�@�C������ǂݍ��݂܂���B

===��
 -d C:\mail


==-p <�v���t�@�C����>

===����
�v���t�@�C�������w�肵�܂��B�ʏ�A�v���t�@�C�����̓��W�X�g������擾���܂����A���̃I�v�V�������g�p����Ǝw�肳�ꂽ�v���t�@�C�������g�p���܂��B((<�v���t�@�C��|URL:Profile.html>))���Q�Ƃ��Ă��������B


===��
 -p mobile


==-g <����R�[�X>

===����
�N������Ɏw�肳�ꂽ�R�[�X�ŏ��񂵂܂��B�w�肳�ꂽ����R�[�X��������Ȃ��ꍇ�ɂ͂��ׂẴA�J�E���g�̂��ׂẴt�H���_�𑗎�M���܂��B����ɂ��ẮA((<����|URL:GoRound.html>))���Q�Ƃ��Ă��������B

===��
 -g
 -g "All Inboxes"


==-s <URL>

===����
�w�肳�ꂽURL���J���܂��BURL��mailto URL�A�܂���feed URL�ł���K�v������܂��B�Ⴆ�΁Amailto:info@example.org��n�����ꍇ�Ainfo@example.org���Ẵ��[���쐬��ʂ��J����܂��B

���̃I�v�V�������g�p���Ċ֘A�t�����s�����ƂŁA�u���E�U�Ȃǂ�mailto URL��feed URL���N���b�N�����Ƃ���QMAIL3���N�������邱�Ƃ��ł���悤�ɂȂ�܂��B

��{�I�ɂ́A���ݑI������Ă���A�J�E���g���g�p����܂��B�������A���ݑI�����Ă���A�J�E���g�����[���A�J�E���g�łȂ������ꍇ�ɂ́A�ȉ��̂悤�Ɏg�p����A�J�E���g�����肵�܂��B

(1)mailto URL�̏ꍇ
   (1)((<qmail.xml|URL:QmailXml.html>))��Global/DefaultMailAccount�Ŏw�肳�ꂽ�A�J�E���g
   (2)�w�肳��Ă��Ȃ������ꍇ�A��ԏ�ɂ��郁�[���A�J�E���g
(2)feed URL�̏ꍇ
   (1)((<qmail.xml|URL:QmailXml.html>))��Global/DefaultRssAccount�Ŏw�肳�ꂽ�A�J�E���g
   (2)�w�肳��Ă��Ȃ������ꍇ�A��ԏ�ɂ���RSS�A�J�E���g

���̂悤�ȃA�J�E���g�����݂��Ȃ��ꍇ�ɂ͉������܂���B

===��
 -s mailto:info@example.org
 -s feed://www.example.org/index.rdf


==-a <�t�@�C����>

===����
�w�肳�ꂽ�t�@�C����Y�t�t�@�C���Ƃ��ēY�t������ԂŃG�f�B�b�g�r���[���J���܂��B-s�ł�mailto URL�̎w��Ɠ����Ɏw��ł��܂��B

===��
 -a C:\temp\test.png
 -s test@example.org -a "C:\Data Files\Test.doc"


==-c [<�t�@�C����>]

===����
�w�肳�ꂽ�t�@�C����ǂݍ���Ń��[�����쐬���܂��B�ǂݍ��܂��t�@�C���́A�v���b�g�t�H�[���̃f�t�H���g�̃G���R�[�f�B���O�ŃG���R�[�h����Ă���K�v������܂��B�܂��ARFC2822�Ɋ�Â��`���ɂȂ��Ă���K�v������܂����A�w�b�_�̕������RFC2047��RFC2231�Ɋ�Â��ăG���R�[�h����Ă���K�v�͂���܂���i�G���R�[�h����Ă��Ă��\���܂���j�B�Ⴆ�΁A�ȉ��̂悤�ȓ��e�Ńt�@�C�����쐬���܂��B

 To: foo@example.com
 Subject: ����̓e�X�g�ł�
 
 �����ɖ{��������܂��B
 �w�b�_�Ɩ{���̊Ԃɂ͋�s���K�v�ł��B

�t�@�C�������ȗ������ꍇ�ɂ́A�N���b�v�{�[�h����擾������������g�p���ă��[�����쐬���܂��B

�O���G�f�B�^�Ȃǂ��烁�[�����쐬����ꍇ�Ɏg�p���܂��B

===��
 -c
 -c "C:\Temp\mail.txt"


==-r [<�t�@�C����>]

===����
-c�Ɠ��l�ł����A���e�Ƃ��ă��[�����쐬���܂��B

===��
 -r
 -r "C:\Temp\mail.txt"


==-i <�A�N�V����>

===����
�w�肳�ꂽ�A�N�V���������s���܂��B�A�N�V�����̎w��́A((<ToolInvokeAction�A�N�V����|URL:ToolInvokeActionAction.html>))�̈����̎w��Ɠ��l�ɍs���܂��B

===��
 -i "MessageCreate new"
 -i "MessageCreate ^"Test Template^""


==-q

===����
�P�ƂŁA�܂���-g, -c, -r, -i�Ƌ��ɑg�ݍ��킹�Ďw�肵�܂��B����QMAIL3���N�����Ă���ꍇ�ɁA-q��P�ƂŎw�肷��Ɖ����N����܂���B�g�ݍ��킹�Ďw�肵���ꍇ�ɂ́AQMAIL3���őO�ʂɈړ����邱�ƂȂ�������s���܂��B�܂��AQMAIL3���N�����Ă��Ȃ��ꍇ�ɂ́A�^�X�N�g���C�Ɋi�[���ꂽ��ԂŋN�����܂��iPocket PC�łł͍Ŕw�ʂŋN�����܂��j�B

-i�Ƒg�ݍ��킹��ꍇ�ɂ͒��ӂ��K�v�ł��B�ꕔ�̃A�N�V�����̓^�X�N�g���C�Ɋi�[����Ă��Ȃ����Ƃ��O��̂��߁A���̂悤�ȃA�N�V������-i�Ŏw�肵��-q�Ƒg�ݍ��킹��Ɨ\�����Ȃ���������邱�ƂɂȂ�܂��B

===��
 -g "All Inboxes" -q
 -c "C:\Temp\mail.txt" -q
 -r -q

=end

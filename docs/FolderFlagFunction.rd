=begin
=@FolderFlag

 Boolean @FolderFlag(String folder, Number flag)


==����
folder�Ŏw�肳�ꂽ�t�H���_��flag�Ŏw�肵���t���O�������Ă��邩�ǂ�����Ԃ��܂��B

folder�ɂ̓t���O�𒲂ׂ����t�H���_�̊��S�����w�肵�܂��B�w�肳�ꂽ�t�H���_��������Ȃ��ƃG���[�ɂȂ�܂��B

flag�ɂ͒��ׂ����t���O���w�肵�܂��B�w��ł���͈̂ȉ��̂����ꂩ�ł��B

::FF-NOSELECT
  �I���\���ǂ����B�I���\�ȏꍇ�ɂ�False���A����ȊO�̏ꍇ�ɂ�True���Ԃ���܂��BIMAP4�A�J�E���g�ł̓��b�Z�[�W�������Ȃ��t�H���_�ł͂��̃t���O��True�ɂȂ�܂��B
::FF-NOINFERIORS
  �q�t�H���_�������Ƃ��ł��邩�ǂ����B�q�t�H���_�������Ƃ��ł���ꍇ�ɂ�False���A����ȊO�̏ꍇ�ɂ�True���Ԃ���܂��BIMAP4�A�J�E���g�ł͎q�t�H���_���쐬���邱�Ƃ��ł��Ȃ��t�H���_�ł͂��̃t���O��True�ɂȂ�܂��B
::FF-CUSTOMFLAGS
  �J�X�^���t���O���T�|�[�g���邩�ǂ����BIMAP4�A�J�E���g�ł̓T�[�o��ɔC�ӂ̃t���O��ۑ��ł���ꍇ�ɂ�True���A����ȊO�̏ꍇ�ɂ�False���Ԃ���܂��B
::FF-NORENAME
  ���O��ύX���邱�Ƃ��ł��邩�ǂ����B���O��ύX�ł���ꍇ�ɂ�False���A����ȊO�̏ꍇ�ɂ�True���Ԃ���܂��B
::FF-LOCAL
  ���[�J���t�H���_���ǂ����B���[�J���t�H���_�̏ꍇ�ɂ�True���A����ȊO�̏ꍇ�ɂ�False���Ԃ���܂��B
::FF-HIDE
  �B����Ă��邩�ǂ����B�B����Ă���ꍇ�ɂ�True���A����ȊO�̏ꍇ�ɂ�False���Ԃ���܂��B
::FF-CHILDOFROOT
  IMAP4�A�J�E���g�Ŏw�肳�ꂽ���[�g�t�H���_�̎q�t�H���_�̏ꍇ�ɂ�True���A����ȊO�̏ꍇ�ɂ�False���Ԃ���܂��B
::FF-IGNOREUNSEEN
  ���ǂ𖳎����邩�ǂ����B���ǂ𖳎�����ꍇ�ɂ�True���A����ȊO�̏ꍇ�ɂ�False���Ԃ���܂��B
::FF-INBOX
  ��M�����ǂ����B
::FF-OUTBOX
  ���M�����ǂ����B
::FF-SENTBOX
  ���M�T�����ǂ����B
::FF-TRASHBOX
  �S�~�����ǂ����B
::FF-DRAFTBOX
  ���e�����ǂ����B
::FF-SEARCHBOX
  �������ǂ����B
::FF-JUNKBOX
  �X�p�����ǂ����B
::FF-SYNCABLE
  �����\���ǂ����B
::FF-SYNCWHENOPEN
  �J�����Ƃ��ɓ������邩�ǂ����B
::FF-CACHEWHENREAD
  �{����ǂ񂾂Ƃ��ɃL���b�V�����邩�ǂ����B


==����
:String folder
  �t�H���_��
:Number flag
  ���ׂ�t���O


==�G���[
*�����̐��������Ă��Ȃ��ꍇ
*�w�肳�ꂽ�t�H���_��������Ȃ��ꍇ


==����
�Ȃ�


==��
 # �I������Ă��郁�b�Z�[�W���i�[����Ă���t�H���_����M�����ǂ����𒲂ׂ�
 @FolderFlag(@Folder(), :FF-INBOX)
 
 # ���݂̃t�H���_�������\���ǂ������ׂ�
 @FolderFlag(@Folder(:FN-FULLNAME, :FT-CURRENT), :FF-SYNCABLE)

=end

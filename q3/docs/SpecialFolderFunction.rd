=begin
=@SpecialFolder

 String @SpecialFolder(Number type, String account?)


==����
����t�H���_�̊��S�����擾���܂��B

type�ɂ͎擾����������t�H���_�̃^�C�v���w�肵�܂��B�ȉ��̂����ꂩ���w��ł��܂��B

::SF-INBOX
  ��M��
::SF-OUTBOX
  ���M��
::SF-SENTBOX
  ���M�ς�
::SF-TRASHBOX
  �S�~��
::SF-DRAFTBOX
  ���e��
::SF-SEARCHBOX
  ����
::SF-JUNKBOX
  �X�p��

account�ɃA�J�E���g�����w�肷��Ǝw�肵���A�J�E���g�̓���t�H���_���擾���܂��B����ȊO�̏ꍇ�ɂ̓R���e�L�X�g�A�J�E���g�̓���t�H���_���擾���܂��B


==����
:Number type
  �擾�������t�H���_�̃^�C�v
:String account
  �A�J�E���g


==�G���[
*�����̐��������Ă��Ȃ��ꍇ
*�R���e�L�X�g�A�J�E���g���Ȃ��ꍇ�i�A�J�E���g���w�肵�Ȃ������ꍇ�j
*�w�肳�ꂽ�A�J�E���g��������Ȃ��ꍇ�i�A�J�E���g���w�肵���ꍇ�j
*�w�肳�ꂽ�^�C�v���s���ȏꍇ


==����
�Ȃ�


==��
 # ��M���̃t�H���_�����擾
 @Folder(:SF-INBOX)
 
 # Test�Ƃ������O�̃A�J�E���g�̃X�p���t�H���_�̖��O���擾
 @Folder(:SF-JUNKBOX, 'Test')

=end

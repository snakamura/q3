=begin
=@AccountDirectory

 String @AccountDirectory(String account?)


==����
account�Ŏw�肳�ꂽ�A�J�E���g�̃f�B���N�g����Ԃ��܂��B�w�肳�ꂽ���O�̃A�J�E���g��������Ȃ��ꍇ�ɂ̓G���[�ɂȂ�܂��B�������ȗ����ꂽ�ꍇ�ɂ́A�R���e�L�X�g�A�J�E���g�̃f�B���N�g����Ԃ��܂��B�R���e�L�X�g�A�J�E���g���Ȃ��ꍇ�ɂ̓G���[�ɂȂ�܂��B

�A�J�E���g�̃f�B���N�g���Ƃ́A���̃A�J�E���g�Ɋւ���t�@�C�����ۑ������f�B���N�g���ł��B


==����
:String account
  �A�J�E���g��


==�G���[
*�����̐��������Ă��Ȃ��ꍇ
*�w�肳�ꂽ���O�̃A�J�E���g��������Ȃ��ꍇ
*�R���e�L�X�g�A�J�E���g���Ȃ��ꍇ


==����
�Ȃ�


==��
 # �R���e�L�X�g�A�J�E���g�̃f�B���N�g�����擾
 # -> C:\Documents and Settings\username\Application Data\QMAIL3\accounts\Sub
 @AccountDirectory()
 
 # Main�Ƃ������O�̃A�J�E���g�̃f�B���N�g�����擾
 # -> C:\Documents and Settings\username\Application Data\QMAIL3\accounts\Main
 @AccountDirectory('Main')

=end

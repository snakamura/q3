=begin
=@InputBox

 String @InputBox(String message, Boolean multiline?, String default?)


==����
���[�U�ɓ��͂����߂邽�߂̃_�C�A���O��\�����A���͂��ꂽ�������Ԃ��܂��Bmessage�ɂ͕\�����郁�b�Z�[�W���w�肵�܂��Bdefault���w�肷��ƃ_�C�A���O���J�����Ƃ��ɂ��̕����񂪓��͗��ɕ\������܂��B

multiline�ɂ̓_�C�A���O�̃^�C�v���w�肵�܂��B�w��ł���͈̂ȉ��̂Ƃ���ł��B

::INPUT-SINGLELINE
  �P���s�̓��͂����߂�_�C�A���O
  
  ((<�P���s���̓_�C�A���O|"IMG:images/SingleLineInputBoxDialog.png">))
::INPUT-MULTILINE
  �����s�̓��͂����߂�_�C�A���O
  
  ((<�����s���̓_�C�A���O|"IMG:images/MultiLineInputBoxDialog.png">))

�w�肵�Ȃ��ꍇ�ɂ́A:INPUT-SINGLELINE���w�肵���̂Ɠ����ɂȂ�܂��B


==����
:String message
  �\�����郁�b�Z�[�W
:Boolean multiline
  �_�C�A���O�̃^�C�v
:String default
  �f�t�H���g�̓��͕�����


==�G���[
*�����̐��������Ă��Ȃ��ꍇ
*UI���Ȃ��ꍇ

==����
*UI���K�v


==��
 # �P���s
 @InputBox('���͂��Ă�������')
 
 # �P���s�Ńf�t�H���g���w��
 @InputBox('���͂��Ă�������', :INPUT-SINGLELINE, '�e�X�g')
 
 # �����s
 @InputBox('���͂��Ă�������', :INPUT-MULTILINE)

=end

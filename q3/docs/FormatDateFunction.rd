=begin
=@FormatDate

 String @FormatDate(Time date, String format, Number timezone?)


==����
date�Ŏw�肳�ꂽ���Ԃ�format�Ŏw�肳�ꂽ�����Ńt�H�[�}�b�g���܂��B

�����ɂ͔C�ӂ̕�������w��ł��܂����A%�ɑ��������񂪎��Ԃ̈ꕔ�ɒu���������܂��B

:%Y2
  �񌅂̔N�i��:06�j
:%Y4
  �l���̔N�i��:2006�j
:%M0
  �񌅂̌��i��:05�j
:%M1
  3�����̌��̖��O�i��:Mar�j
:%M2
  �������̖��O�i��:January�j
:%D
  �񌅂̓��i��:29�j
:%W
  3�����̗j���i��:Mon�j�i���̎w��͌Â��`���ł��B�����%W1���g�p���Ă��������j
:%W0
  �Z���j���i��:M, Tu�j
:%W1
  3�����̗j���i��:Mon�j
:%W2
  �����j���i��:Monday�j
:%h
  24���Ԑ��ł̓񌅂̎��i��:19�j
:%m
  �񌅂̕��i��:34�j
:%s
  �񌅂̕b�i��:02�j
:%z
  +��-�̌��4���̃^�C���]�[���i��:+0900�j
:%Z
  +��-�̌��:�ŋ�؂�ꂽ4���̃^�C���]�[���i��:-07:00�j
:%%
  %����

���Ƃ��΁A�u%Y4/%M0/%D %h:%m:%s�v���w�肷��ƁA�u2006/05/07 21:56:32�v�̂悤�Ƀt�H�[�}�b�g����܂��B

timezone�ɂ͈ȉ��̒萔���w��ł��܂��B

::TZ-UTC
  UTC�Ńt�H�[�}�b�g
::TZ-LOCAL
  ���[�J���̃^�C���]�[���Ńt�H�[�}�b�g
::TZ-ORIGINAL
  �I���W�i���̃^�C���]�[���Ńt�H�[�}�b�g

�w�肵�Ȃ��ꍇ�ɂ́A:TZ-LOCAL���w�肵���̂Ɠ����ɂȂ�܂��B


==����
:Time date
  ����
:String format
  ����
:Number timezone
  �ǂ̃^�C���]�[���Ńt�H�[�}�b�g���邩


==�G���[
*�����̐��������Ă��Ȃ��ꍇ
*�����̌^�������Ă��Ȃ��ꍇ
*format�̕����񂪕s���ȏꍇ
*timezone�̒l���s���ȏꍇ



==����
�Ȃ�


==��
 # Date�w�b�_�̎��Ԃ����[�J���^�C���]�[���Ńt�H�[�}�b�g
 @FormatDate(@Date(Date), '%Y4/%M0/%D %h:%m:%s')
 
 # ���݂̎��Ԃ�UTC�Ńt�H�[�}�b�g
 @FormatDate(@Date(), '%Y4/%M0/%d', :TZ-UTC)

=end

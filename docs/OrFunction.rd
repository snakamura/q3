=begin
=@Or

 Boolean @Or(Boolean arg+)


==����
arg�Ŏw�肳�ꂽ�^�U�l�̘_���a��Ԃ��܂��B�܂�A�S�Ă̒l��False�Ȃ��False��Ԃ��A����ȊO�̏ꍇ�ɂ�True��Ԃ��܂��B1�ȏ�̔C�ӂ̐��̈�����n���܂��B


==����
:Boolean arg
  �^�U�l


==�G���[
*�����̐��������Ă��Ȃ��ꍇ


==����
�Ȃ�


==��
 # True��False�̘_���a
 # -> True
 @And(@True(), @False())
 
 # ���ǂ܂��̓}�[�N����Ă���
 @Or(@Not(@Seen()), @Marked())

=end
=begin
=@If

 Value @If(Boolean condition, Value value, Value otherwise)


==����
condition��]������True�ɂȂ����Ƃ��ɂ�value��Ԃ��܂��BFalse�ɂȂ����Ƃ��ɂ�otherwise��Ԃ��܂��B3�ȏ�̔C�ӂ̊�̈�����n���܂��B���̏ꍇ�A1�Ԗڂ̈�����True�ɂȂ����ꍇ�ɂ�2�Ԗڂ̈������A3�Ԗڂ̈�����True�ɂȂ����ꍇ�ɂ�4�Ԗڂ̈������A�Ƃ����悤�ɕ]�����Ă����A�ǂ̏����ɂ����v���Ȃ������ꍇ�ɂ͍Ō�̈�����Ԃ��܂��B


==����
:Boolean condition
  ����
:Value value
  �l
:Value otherwise
  �ǂ̏����ɂ����v���Ȃ������Ƃ��̒l


==�G���[
*�����̐��������Ă��Ȃ��ꍇ


==����
�Ȃ�


==��
 # To��example.org���܂�ł�����1�A����ȊO�Ȃ�0
 @If(@Contain(To, 'example.org'), 1, 0)
 
 # To��example.org���܂�ł�����1�Aexample���܂�ł�����2�A����ȊO�Ȃ�0
 @If(@Contain(To, 'example.org'), 1,
     @Contain(To, 'example'), 2, 0)

=end

=begin
=@Contain

 Boolean @Contain(String s1, String s2, Boolean case?)


==����
s1��s2���܂܂�Ă���ꍇ�ɂ�True�A����ȊO�̏ꍇ�ɂ�False��Ԃ��܂��Bcase��True���w�肷��Ƒ啶���Ə���������ʂ��AFalse�̏ꍇ��ȗ����ꂽ�ꍇ�ɂ͋�ʂ��܂���B


==����
:String s1
  ������
:String s2
  ������
:Boolean case
  �啶���Ə���������ʂ���ꍇ�ɂ�True�A����ȊO�̏ꍇ�ɂ�False


==�G���[
*�����̐��������Ă��Ȃ��ꍇ


==����
�Ȃ�


==��
 # %Subject��Qs���܂܂��i�啶���Ə���������ʁj
 @Contain(%Subject, 'Qs', @True())
 
 # From��@example.org���܂܂��
 @Contain(From, '@example.org')

=end

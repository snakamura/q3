=begin
=@Equal

 Boolean @Equal(Value v1, Value v2, Boolean case?)


==����
v1��v2���������ꍇ�ɂ�True�A����ȊO�̏ꍇ�ɂ�False��Ԃ��܂��Bv1��v2���Ƃ��ɐ^�U�l�̏ꍇ�ɂ͐^�U�l�Ƃ��āAv1��v2���Ƃ��ɐ��l�̏ꍇ�ɂ͐��l�Ƃ��āA����ȊO�̏ꍇ�ɂ͕�����Ƃ��Ĕ�r���܂��B������Ƃ��Ĕ�r����ꍇ�ɂ́Acase��True���w�肷��Ƒ啶���Ə���������ʂ��AFalse�̏ꍇ��ȗ����ꂽ�ꍇ�ɂ͋�ʂ��܂���B


==����
:Value v1
  �l
:Value v2
  ������
:Boolean case
  �啶���Ə���������ʂ���ꍇ�ɂ�True�A����ȊO�̏ꍇ�ɂ�False


==�G���[
*�����̐��������Ă��Ȃ��ꍇ


==����
�Ȃ�


==��
  # List-Id��<ml@example.org>
  @Equal(List-Id, '<ml@example.org>')
  
  # ID��1234
  @Equal(@Id(), 1234)

=end
=begin
=@SubstringAfter

 String @SubstringAfter(String s, String separator, Boolean case?)


==����
s�Ŏw�肳�ꂽ���������separator�Ŏw�肵�����������̕������Ԃ��܂��B���̂悤�ȕ����񂪌�����Ȃ��ꍇ�ɂ͋󕶎����Ԃ��܂��Bcase��True���w�肷��Ƒ啶���Ə���������ʂ��AFalse�̏ꍇ��ȗ����ꂽ�ꍇ�ɂ͋�ʂ��܂���B


==����
:String s
  ������
:String separator
  �������镶����
:Boolean case
  �啶���Ə���������ʂ���ꍇ�ɂ�True�A����ȊO�̏ꍇ�ɂ�False


==�G���[
*�����̐��������Ă��Ȃ��ꍇ


==����
�Ȃ�


==��
 # c������擾
 # -> def
 @SubstringAfter('abcdef', 'c')
 
 # �啶������������ʂ���XYZ������擾
 # -> ZZ
 @SubstringAfter('wxyzXYZZZ', 'XYZ', @True())

=end

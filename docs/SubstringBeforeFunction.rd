=begin
=@SubstringBefore

 String @SubstringBefore(String s, String separator, Boolean case?)


==����
s�Ŏw�肳�ꂽ���������separator�Ŏw�肵����������O�̕������Ԃ��܂��B���̂悤�ȕ����񂪌�����Ȃ��ꍇ�ɂ͋󕶎����Ԃ��܂��Bcase��True���w�肷��Ƒ啶���Ə���������ʂ��AFalse�̏ꍇ��ȗ����ꂽ�ꍇ�ɂ͋�ʂ��܂���B


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
 # c���O���擾
 # -> ab
 @SubstringBefore('abcdef', 'c')
 
 # �啶������������ʂ���XYZ���O���擾
 # -> wxyz
 @SubstringBefore('wxyzXYZZZ', 'XYZ', @True())

=end

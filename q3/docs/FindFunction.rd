=begin
=@Find

 Number @Find(String s1, String s2, Number index?, Boolean case?)


==����
s1�̒��ōŏ���s2�������C���f�b�N�X��Ԃ��܂��Bindex���w�肳�ꂽ�ꍇ�ɂ́A�w�肳�ꂽ�������ڂ��猟�����n�߂܂��Bcase��True���w�肷��Ƒ啶���Ə���������ʂ��AFalse�̏ꍇ��ȗ����ꂽ�ꍇ�ɂ͋�ʂ��܂���B�C���f�b�N�X��0�x�[�X�ł��B


==����
:String s1
  ������
:String s2
  �������镶����
:Number index
  �������n�߂�C���f�b�N�X
:Boolean case
  �啶���Ə���������ʂ���ꍇ�ɂ�True�A����ȊO�̏ꍇ�ɂ�False


==�G���[
*�����̐��������Ă��Ȃ��ꍇ


==����
�Ȃ�


==��
 # ����������
 # -> 2
 @Find('abcabcabc', 'c')
 
 # �C���f�b�N�X���w��
 # -> 5
 @Find('abcabcabc', 'Ca', 3)

=end

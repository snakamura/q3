=begin
=@Set

 Value @Set(String name, Value value, Boolean global?)


==����
name�Ŏw�肳�ꂽ���O�̕ϐ���value�������܂��Bglobal��:GLOBAL���w�肷��ƃO���[�o���ϐ��ɂȂ�܂��B�w�肳��Ȃ������ꍇ�ɂ̓��[�J���ϐ��ɂȂ�܂��Bvalue�����̂܂ܕԂ��܂��B


==����
:String name
  �ϐ���
:Value value
  �l
:Boolean global
  �O���[�o���ϐ����ǂ���


==�G���[
*�����̐��������Ă��Ȃ��ꍇ


==����
�Ȃ�


==��
 # test�Ƃ������O�̃��[�J���ϐ���@Address(To)�̌��ʂ���
 @Set('test', @Address(To))
 
 # bcc�Ƃ������O�̃O���[�o���ϐ���@Profile('', 'Global', 'Bcc', '1')�̌��ʂ���
 @Set('bcc', @Profile('', 'Global', 'Bcc', '1'), :GLOBAL)

=end

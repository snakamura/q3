=begin
=@Script

 Value @Script(String script, String lang, Value args+)


==����
�w�肳�ꂽ�X�N���v�g�����s���܂��B


==����
:String script
  ���s����X�N���v�g
:String lang
  �X�N���v�g����̖��O
:Value args
  �X�N���v�g�ɓn������


==�G���[
*�����̐��������Ă��Ȃ��ꍇ
*�X�N���v�g�̃p�[�X�E���s�Ɏ��s�����ꍇ


==����
�Ȃ�


==��
 # �X�N���v�g�����s
 @Script('result.value = 1 + 2', 'JScript')
 
 # �X�N���v�g�Ɉ�����n���Ď��s
 @Script('result.value = arguments(0) + arguments(1)', 'JScript', 1, 2)

=end

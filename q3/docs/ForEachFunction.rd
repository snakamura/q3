=begin
=@ForEach

 Boolean @ForEach(MessageList messages, Expr expr)


==����
messages�Ŏw�肳�ꂽ�e���b�Z�[�W���R���e�L�X�g���b�Z�[�W�Ƃ��āAexpr�Ŏw�肳�ꂽ�����J��Ԃ��]�����܂��B���True��Ԃ��܂��B


==����
:MessageList messages
  �Ώۂ̃��b�Z�[�W���X�g
:Expr expr
  �]�����鎮


==�G���[
*�����̐��������Ă��Ȃ��ꍇ
*�����̌^�������Ă��Ȃ��ꍇ
*����]�����ɃG���[�����������ꍇ


==����
�Ȃ�


==��
 # ��M�����̂��ׂẴ��b�Z�[�W�����ǂɂ���
 @ForEach(@Messages('��M��'), @Seen(@True()))
 
 # �R���e�L�X�g���b�Z�[�W��������X���b�h�̃��b�Z�[�W�S�Ă��}�[�N����
 @ForEach(@Thread(), @Marked(@True()))

=end

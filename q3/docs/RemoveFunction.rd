=begin
=@Remove

 String @Remove(String address, String remove+)


==����
address�Ŏw�肳�ꂽ�A�h���X�̃��X�g����Aremove�Ŏw�肳�ꂽ�A�h���X���폜�������ʂ̃A�h���X�̃��X�g��Ԃ��܂��B


==����
:String address
  �A�h���X�̃��X�g
:String remove
  �폜����A�h���X


==�G���[
*�����̐��������Ă��Ȃ��ꍇ
*�A�h���X�̃p�[�X�Ɏ��s�����ꍇ


==����
�Ȃ�


==��
 # Cc�Ɏw�肳�ꂽ�A�h���X���玩���̃A�h���X���폜
 @Remove(Cc, @Address(@I()))
 
 # To�Ɏw�肳�ꂽ�A�h���X���玩���̃A�h���X��From�Ɏw�肳�ꂽ�A�h���X���폜
 @Remove(To, @Address(@I()), @Address(From))

=end

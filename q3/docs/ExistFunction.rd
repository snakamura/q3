=begin
=@Exist

 Boolean @Exist(String name)


==����
�w�b�_�Ɏw�肳�ꂽ���O�̃t�B�[���h�����邩�ǂ�����Ԃ��܂��B

�t�B�[���h��Boolean�ɕϊ�����ƃt�B�[���h�̑��݂����ׂ���̂ňȉ��̎O�͓����Ӗ��ɂȂ�܂��B

 @If(@Exist('X-ML-Name'), 1, 2)
 @If(X-ML-Name, 1, 2)
 @If(@Field('X-ML-Name'), 1, 2)


==����
:String name
  �t�B�[���h��


==�G���[
*�����̐��������Ă��Ȃ��ꍇ
*�R���e�L�X�g���b�Z�[�W���Ȃ��ꍇ
*���b�Z�[�W�̎擾�Ɏ��s�����ꍇ


==����
�Ȃ�


==��
 # X-ML-Name�����邩�ǂ������ׂ�
 @Exist('X-ML-Name')

=end

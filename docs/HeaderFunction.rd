=begin
=@Header

 String @Header(String remove?, Part part?)


==����
�R���e�L�X�g���b�Z�[�W�̃w�b�_��Ԃ��܂��Bpart���w�肳�ꂽ�ꍇ�ɂ͂��̃p�[�g�̃w�b�_��Ԃ��܂��B

remove�ɂ͕Ԃ����w�b�_���珜�O�������w�b�_�̖��O���u,�v��؂�Ŏw�肵�܂��B


==����
:String remove
  ���O����w�b�_�̃��X�g
:Part part
  �p�[�g


==�G���[
*�����̐��������Ă��Ȃ��ꍇ
*�R���e�L�X�g���b�Z�[�W���Ȃ��ꍇ
*���b�Z�[�W�̎擾�Ɏ��s�����ꍇ
*�w�肵���p�[�g���Ȃ��ꍇ�ipart���w�肵���ꍇ�j


==����
�Ȃ�


==��
 # �w�b�_��S�Ď擾
 @Header()
 
 # To��Message-Id�����O���ăw�b�_���擾
 @Header('To,Message-Id')
 
 # �}���`�p�[�g���b�Z�[�W�ł͂��߂̃p�[�g�̃w�b�_���擾
 @Header('', @Part(0))

=end

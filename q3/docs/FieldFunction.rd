=begin
=@Field

 Field @Field(String name, Part part?)


==����
�R���e�L�X�g���b�Z�[�W�̎w�肳�ꂽ���O�̃w�b�_��Ԃ��܂��Bpart���w�肳�ꂽ�ꍇ�ɂ͂��̃p�[�g�̎w�肵�����O�̃w�b�_��Ԃ��܂��B


==����
:String name
  �w�b�_��
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
 # To���擾
 @Field('To')
 
 # �}���`�p�[�g���b�Z�[�W�ł͂��߂̃p�[�g��Content-ID���擾
 @Field('Content-ID', @Part(0))

=end

=begin
=@Part

 Part @Part(Number index, Part part?)


==����
�R���e�L�X�g���b�Z�[�W�̎w�肳�ꂽ�C���f�b�N�X�̃p�[�g��Ԃ��܂��Bpart���w�肳�ꂽ�ꍇ�ɂ͂��̃p�[�g�̎q�p�[�g�̎w�肳�ꂽ�C���f�b�N�X�Ԗڂ�Ԃ��܂��B�C���f�b�N�X��0�x�[�X�ł��B

�p�[�g�����݂��Ȃ��C���f�b�N�X���w�肵���ꍇ�ɂ��G���[�ɂȂ�܂��񂪁A�Ԃ��ꂽPart��((<@Body|URL:BodyFunction.html>))�Ȃǂ̃p�[�g�������Ɏw��ł���֐��ɓn���ƃG���[�ɂȂ�܂��B�p�[�g�����݂��邩�ǂ����͕Ԃ��ꂽPart��Boolean�ɕϊ����邱�ƂŒ��ׂ��܂��B

==����
:Number index
  �p�[�g�̃C���f�b�N�X
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
 # �}���`�p�[�g�Ȃ�ŏ��̃p�[�g�̖{�����擾
 @If(@Multipart(),
     @If(@Part(0),
         @Body('', :BODY-INLINE, @Part(0)),
         ''),
     @Body('', :BODY-INLINE))

=end

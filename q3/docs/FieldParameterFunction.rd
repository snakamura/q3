=begin
=@FieldParameter

 String @FieldParameter(String name, String paramName?, Part part?)


==����
�R���e�L�X�g���b�Z�[�W�̎w�肳�ꂽ���O�̃w�b�_�̎w�肳�ꂽ���O�̃p�����[�^�̒l��Ԃ��܂��Bpart���w�肳�ꂽ�ꍇ�ɂ͂��̃p�[�g�̎w�肵�����O�̃w�b�_�̎w�肳�ꂽ���O�̃p�����[�^�̒l��Ԃ��܂��B�w�肳�ꂽ���O�̃w�b�_��p�����[�^��������Ȃ��ꍇ�ɂ͋󕶎����Ԃ��܂��B�w�肳�ꂽ���O�̃w�b�_����������ꍇ�ɂ͐擪�̃w�b�_���g�p����܂��B

paramName���󕶎��񂩏ȗ����ꂽ�ꍇ�ɂ́A�p�����[�^�łȂ�������Ԃ��܂��B���Ƃ��΁A

 Content-Type: text/plain; charset=iso-2022-jp

�Ƃ����悤�ȃw�b�_�̏ꍇ�A@FieldParameter('Content-Type')�́utext/plain�v���A@FieldParameter('Content-Type', 'charset')�́uiso-2022-jp�v��Ԃ��܂��B


==����
:String name
  �w�b�_��
:String paramName
  �p�����[�^��
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
 # Content-Type�̒l���擾
 @FieldParameter('Content-Type')
 
 # Content-Type��charset�l���擾
 @FieldParameter('Content-Type', 'charset')
 
 # �}���`�p�[�g���b�Z�[�W�ł͂��߂̃p�[�g��Content-Disposition��filename�p�����[�^���擾
 @Field('Content-Disposition', 'filename', @Part(0))

=end

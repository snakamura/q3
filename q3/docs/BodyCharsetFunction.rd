=begin
=@BodyCharset

 String @BodyCharset(Number type?, Part part?)


==����
�R���e�L�X�g���b�Z�[�W�̖{���̃G���R�[�f�B���O��Ԃ��܂��Bpart���w�肳�ꂽ�ꍇ�ɂ͂��̃p�[�g�̖{���̃G���R�[�f�B���O��Ԃ��܂��B���̊֐��́A@Body���g�p�����Ƃ��Ɏg�p�����G���R�[�f�B���O��Ԃ��܂��B�G���R�[�f�B���O�𒼐ڎw�肵�Ă���ꍇ�ɂ͂��̃G���R�[�f�B���O���A����ȊO�̏ꍇ�ɂ͎������肳�ꂽ�G���R�[�f�B���O��Ԃ��܂��B�}���`�p�[�g�̏ꍇ�Ɋe�p�[�g�̃G���R�[�f�B���O���قȂ�ꍇ�ɂ�utf-8��Ԃ��܂��B

type�ɂ͖{���̃t�H�[�}�b�g���@���w�肵�܂��B((<@Body|URL:BodyFunction.html>))��type�����Ɠ����l���w��ł��܂��B�������A�������ȗ����ꂽ�ꍇ�ɂ�:BODY-INLINE���w�肵���̂Ɠ����ɂȂ�܂��B



==����
:Number type
  �t�H�[�}�b�g���@
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
 # �{����S�Ď擾
 @Body()
 
 # �C�����C���̖{�����u> �v�ň��p���Ď擾
 @Body('> ', :BODY-INLINE)
 
 # �C�����C���̖{�����擾�i�������Amessage/rfc822�̃p�[�g�͕K���擾�j
 @Body('', :BODY-RFC822INLINE)
 
 # �}���`�p�[�g���b�Z�[�W�ł͂��߂̃p�[�g�̖{�����擾
 @Body('', :BODY-ALL, @Part(0))

=end

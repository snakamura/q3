=begin
=@Body

 String @Body(String quote?, Number type?, Part part?)


==����
�R���e�L�X�g���b�Z�[�W�̖{����Ԃ��܂��Bpart���w�肳�ꂽ�ꍇ�ɂ͂��̃p�[�g�̖{����Ԃ��܂��B

quote�ɂ͈��p�����w�肵�܂��B�󕶎���ȊO���w�肷��Ǝw�肳�ꂽ���p���ň��p����܂��B

type�ɂ͖{���̃t�H�[�}�b�g���@���w�肵�܂��B�w��ł���͈̂ȉ��̂����ꂩ�ł��B

::BODY-ALL
  �{���S�Ă�Ԃ��܂��B�}���`�p�[�g�̏ꍇ�ɂ̓p�[�g��W�J���ĕԂ��܂��B
::BODY-RFC822INLINE
  :BODY-INLINE�Ɠ����ł����Amessage/rfc822�̃p�[�g��Content-Disposition�Ɋւ�炸�Ԃ���܂��B
::BODY-INLINE
  �C�����C���̖{����Ԃ��܂��B�C�����C���̖{���Ƃ́AContent-Disposition���w�肳��Ă��Ȃ����܂���inline���AContent-Type��text/*�܂���message/rfc822�̂��̂ł��B

�������ȗ����ꂽ�ꍇ�ɂ�:BODY-ALL���w�肵���̂Ɠ����ɂȂ�܂��B

==����
:String quote
  ���p��
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
 # �{���̃G���R�[�f�B���O���擾
 @BodyCharset()
 
 # �C�����C���̖{���̃G���R�[�f�B���O���擾
 @BodyCharset(:BODY-INLINE)

=end

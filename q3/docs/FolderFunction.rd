=begin
=@Folder

 String @Folder(Boolean full?, Boolean current?)


==����
�R���e�L�X�g���b�Z�[�W���ۑ�����Ă���t�H���_�A�܂��̓R���e�L�X�g�t�H���_�̖��O��Ԃ��܂��B

full�ɂ͊��S����Ԃ����P�ꖼ��Ԃ������w�肵�܂��B�w��ł���͈̂ȉ��̂����ꂩ�ł��B

::FN-FULLNAME
  ���S����Ԃ��܂��B
::FN-NAME
  �P�ꖼ��Ԃ��܂��B

�������ȗ����ꂽ�ꍇ�ɂ�:FN-FULLNAME���w�肵���̂Ɠ����ɂȂ�܂��B

�Ⴆ�΁A�u��M���v�̉��ɂ���u�e�X�g�v�t�H���_�Ƀ��b�Z�[�W���ۑ�����Ă���ꍇ�ɂ́A���S���́u��M��/�e�X�g�v�ɂȂ�A�P�ꖼ�́u�e�X�g�v�ɂȂ�܂��B

current�ɂ̓t�H���_���̎擾���@���w�肵�܂��B�w��ł���͈̂ȉ��̂����ꂩ�ł��B

::FT-MESSAGE
  �R���e�L�X�g���b�Z�[�W���i�[����Ă���t�H���_��Ԃ��܂��B
::FT-CURRENT
  �R���e�L�X�g�t�H���_��Ԃ��܂��B

�������ȗ����ꂽ�ꍇ�ɂ�:FT-MESSAGE���w�肵���̂Ɠ����ɂȂ�܂��B

�Ⴆ�΁A�����t�H���_���J���Ă����ꍇ�A:FT-MESSAGE���w�肷��ƃ��b�Z�[�W���i�[����Ă���t�H���_�̖��O���Ԃ���A:FT-CURRENT���w�肷��ƌ����t�H���_�̖��O���Ԃ���܂��B

:FT-CURRENT���w�肵���ꍇ�ɃR���e�L�X�g�t�H���_���Ȃ��i�A�J�E���g���I������Ă���j�Ƃ��ɂ͋󕶎����Ԃ��܂��B


==����
:Boolean full
  ���S�����擾���邩�ǂ���
:Boolean current
  �t�H���_���̎擾���@


==�G���[
*�����̐��������Ă��Ȃ��ꍇ
*�R���e�L�X�g���b�Z�[�W���Ȃ��ꍇ�i:FT-MESSAGE���w�肵���ꍇ�j


==����
�Ȃ�


==��
 # �t�H���_�������S���Ŏ擾
 @Folder()
 
 # �P�ꖼ�Ŏ擾
 @Folder(:FN-NAME)
 
 # ���݂̃t�H���_�������S���Ŏ擾
 @Folder(:FN-FULLNAME, :FT-CURRENT)

=end

=begin
=@Length

 Number @Length(String s, Boolean byte?)


==����
�w�肳�ꂽ������̒�����Ԃ��܂��Bbyte��True���w�肷��ƁA�v���b�g�t�H�[���̃G���R�[�f�B���O((-���{��v���b�g�t�H�[���̏ꍇ�ɂ�CP932-))�ɕϊ������Ƃ��̃o�C�g����Ԃ��܂��B


==����
:String s
  ������
:Boolean byte
  �o�C�g��̒�����Ԃ����ǂ���


==�G���[
*�����̐��������Ă��Ȃ��ꍇ


==����
�Ȃ�


==��
 # ������̒������擾
 # -> 3
 @Length('abc')
 
 # �o�C�g�����擾
 # -> 6�i���{����̏ꍇ�j
 @Length('���{��', @True())

=end

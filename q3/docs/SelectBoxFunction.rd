=begin
=@SelectBox

 String @SelectBox(String message, String candidates, Number type?, String default?)


==����
���[�U�ɑI�������߂邽�߂̃_�C�A���O��\�����A�I�����ꂽ�������Ԃ��܂��Bmessage�ɂ͕\�����郁�b�Z�[�W���w�肵�܂��Bcandidates�ɂ̓��X�g�Ɍ��Ƃ��ĕ\�����镶��������s��؂�Ŏw�肵�܂��B���Ƃ��΁Afoo, bar, baz�����ɂ���ꍇ�ɂ́A'foo\nbar\nbaz'�̂悤�Ɏw�肵�܂��B

type�ɂ̓��X�g�̃^�C�v���w�肵�܂��B�w��ł���͈̂ȉ��̂Ƃ���ł��B

::SELECT-LIST
  ���X�g�\��
  
  ((<���X�g�I���_�C�A���O|"IMG:images/ListSelectBoxDialog.png">))

::SELECT-DROPDOWNLIST
  �h���b�v�_�E�����X�g�\��
  
  ((<�h���b�v�_�E�����X�g�I���_�C�A���O|"IMG:images/DropDownListSelectBoxDialog.png">))

::SELECT-DROPDOWN
  �h���b�v�_�E���\��
  
  ((<�h���b�v�_�E���I���_�C�A���O|"IMG:images/DropDownSelectBoxDialog.png">))

�w�肵�Ȃ��ꍇ�ɂ́ASELECT-LIST���w�肵���̂Ɠ����ɂȂ�܂��B

default���w�肷��ƃf�t�H���g�ł��̌�₪�I������܂��B���ɂȂ��l���w�肷��ƁA�w�肵�Ȃ��̂Ɠ����ɂȂ�܂����Atype��:SELECT-DROPDOWN�̏ꍇ�ɂ͂��̒l�����͂��ꂽ��ԂɂȂ�܂��B�w�肵�Ȃ��ꍇ�ɂ͐擪�̌�₪�I������܂��B



==����
:String message
  �\�����郁�b�Z�[�W
:String candidates
  ���X�g�ɕ\��������
:Number type
  ���X�g�̃^�C�v
:String default
  �f�t�H���g�̓��͕�����


==�G���[
*�����̐��������Ă��Ȃ��ꍇ
*UI���Ȃ��ꍇ

==����
*UI���K�v


==��
 # foo, bar, baz���烊�X�g�őI��
 @SelectBox('�I�����Ă�������', 'foo\nbar\nbaz')
 
 # us-ascii, iso-2022-jp, utf-8����h���b�v�_�E���őI���i�f�t�H���g��iso-2022-jp�j
 @SelectBox('�G���R�[�f�B���O', 'us-ascii\niso-2022-jp\nutf-8', :SELECT-DROPDOWN, 'iso-2022-jp')

=end

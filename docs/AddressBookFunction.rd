=begin
=@AddressBook

 String @AddressBook(String to?, String cc?, String bcc?)


==����
((<[�A�h���X�̑I��]�_�C�A���O|URL:SelectAddressDialog.html>))���J���܂��Bto, cc, bcc�̊e������RFC2822�`���̃A�h���X���X�g���w�肷��Ƃ����̃A�h���X���I�����ꂽ��ԂŃ_�C�A���O���J���܂��B�A�h���X�I���_�C�A���O��[OK]���N���b�N���ĕ���ƁA�I�����ꂽ�A�h���X��RFC2822�̃w�b�_�`���ŕԂ���܂��B


==����
:String to
  To�̃A�h���X
:String cc
  Cc�̃A�h���X
:String bcc
  Bcc�̃A�h���X


==�G���[
*�����̐��������Ă��Ȃ��ꍇ
*UI�X���b�h�ȊO����Ăяo�����ꍇ
*UI���Ȃ��ꍇ

==����
*UI�X���b�h����̂݌Ăяo���\
*UI���K�v


==��
 # [�A�h���X�̑I��]�_�C�A���O���J��
 @AddressBook()
 
 # Bcc�Ɏ����̃A�h���X��I��������Ԃ�[�A�h���X�̑I��]�_�C�A���O���J��
 @AddressBook('', '', @I())

=end

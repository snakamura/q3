=begin
=@LookupAddressBook

 String @LookupAddressBook(String address, Number type)


==����
�w�肳�ꂽ�A�h���X���A�h���X������t�������܂��B

type�ɂ͋t��������l���w�肵�܂��B�ȉ��̂����ꂩ���w��ł��܂��B

::ADDRESS-NAME
  ���O
::ADDRESS-SORTKEY
  �\�[�g�L�[
::ADDRESS-ADDRESS
  �A�h���X
::ADDRESS-ALIAS
  �G�C���A�X
::ADDRESS-CATEGORY
  �J�e�S��
::ADDRESS-COMMENT
  �R�����g

�w�肵�Ȃ������ꍇ�ɂ́A:ADDRESS-NAME���w�肵���̂Ɠ����ɂȂ�܂��B�w�肵���A�h���X�̃G���g����������Ȃ������ꍇ�ɂ͋󕶎����Ԃ��܂��B

:ADDRESS-ADDRESS�͎w�肵���A�h���X�Ɠ����l��Ԃ��܂��i�啶���E�������͕ς��\��������܂��j�B���̂��߁A:ADDRESS-ADDRESS���w�肵�ċt�������ċ󕶎��񂩂ǂ������ׂ邱�ƂŃA�h���X���ɃG���g�������邩�ǂ����𒲂ׂ邱�Ƃ��ł��܂��B


==����
:Number ltype
  �t�����^�C�v


==�G���[
*�����̐��������Ă��Ȃ��ꍇ


==����
�Ȃ�


==��
 # ���O���t����
 @LookupAddressBook($address)
 
 # �R�����g���t����
 @LookupAddressBook($address, :ADDRESS-COMMENT)
 
 # �A�h���X���ɃG���g�������邩�ǂ����𒲂ׂ�
 @If(@LookupAddressBook($address, :ADDRESS-ADDRESS),
     '�A�h���X���ɃG���g������',
     '�A�h���X���ɃG���g���Ȃ�')
=end

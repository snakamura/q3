=begin
=@FindEach

 Value @FindEach(MessageList messages, Expr condition, Expr expr?)


==����
messages�Ŏw�肳�ꂽ�e���b�Z�[�W���R���e�L�X�g���b�Z�[�W�Ƃ��āAcondition�Ŏw�肳�ꂽ����]�����A���ʂ�True�ɂȂ������b�Z�[�W���R���e�L�X�g���b�Z�[�W�Ƃ���expr��]�����ĕԂ��܂��Bexpr���w�肳��Ȃ������ꍇ�ɂ͍Ō�ɕ]������condition�̌��ʂ����̂܂ܕԂ��܂��B�S�Ẵ��b�Z�[�W�ɑ΂���condition��]�������l��False�ɂȂ����ꍇ�ɂ́AFalse��Ԃ��܂��B


==����
:MessageList messages
  �Ώۂ̃��b�Z�[�W���X�g
:Expr condition
  ������
:Expr expr
  �]�����鎮


==�G���[
*�����̐��������Ă��Ȃ��ꍇ
*�����̌^�������Ă��Ȃ��ꍇ
*����]�����ɃG���[�����������ꍇ


==����
�Ȃ�


==��
 # �R���e�L�X�g���b�Z�[�W����������X���b�h�̐擪�̃��b�Z�[�W��Message-Id���擾
 @FindEach(@Thread(), @True(), Message-Id)
 
 # �R���e�L�X�g���b�Z�[�W����������X���b�h�Ɏ��������M�������b�Z�[�W�����邩�ǂ����𒲂ׂ�
 @FindEach(@Thread(), @Equal(@Address(From), @Address(@I())))
 
 # �R���e�L�X�g���b�Z�[�W����������X���b�h�̒��Ő擪�̖��ǂ̃��b�Z�[�W��Subject���擾
 @FindEach(@Thread(), @Not(@Seen()), Subject)

=end

=begin
=@Catch

 Value @Catch(Expr expr, Expr handler)


==����
expr��]�������̌��ʂ�Ԃ��܂��B���̊ԂɃG���[�����������ꍇ�ɂ�handler��]�����Ă��̌��ʂ�Ԃ��܂��B


==����
:Expr expr
  �]�����鎮
:Expr handler
  �G���[���N�����Ƃ��ɕ]������鎮


==�G���[
*�����̐��������Ă��Ȃ��ꍇ
*handler��]�����ɃG���[�����������ꍇ


==����
�Ȃ�


==��
 # �t�@�C����ǂݍ���ŕԂ����A�G���[���N������󕶎����Ԃ�
 @Catch(@Load('C:\\Temp\\test.txt'), '')
 
 # �R���e�L�X�g���b�Z�[�W�Ɋ��ǃt���O��ݒ肷�邪�A
 # �R���e�L�X�g���b�Z�[�W���Ȃ������牽�����Ȃ�
 @Catch(@Seen(@True()), @True())

=end

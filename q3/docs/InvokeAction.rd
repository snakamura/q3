=begin
=@InvokeAction

 Boolean @InvokeAction(String name, String arg+)


==����
name�Ŏw�肳�ꂽ�A�N�V�������N�����܂��B����ȍ~�̈����̓A�N�V�����̈����Ƃ��ăA�N�V�����ɓn����܂��B���True��Ԃ��܂��B

���̊֐��͔C�ӂ̃A�N�V�������N�����邱�Ƃ��ł��邽�߁A�g�p����ɂ͒��ӂ��K�v�ł��B���Ƃ��΁AEditDelete�A�N�V�������N�����ď������̃��b�Z�[�W���폜����ƃN���b�V�����邱�Ƃ�����܂��B


==����
:String name
  �A�N�V�����̖��O
:arg
  �A�N�V�����ɓn������


==�G���[
*�����̐��������Ă��Ȃ��ꍇ
*UI�X���b�h�ȊO����Ăяo�����ꍇ
*UI���Ȃ��ꍇ
*�A�N�V�������N���ł��Ȃ��ꍇ


==����
*UI�X���b�h����̂݌Ăяo���\
*UI���K�v


==��
 # MessageCreate�A�N�V������new.template���g���ĐV�K���b�Z�[�W���쐬����
 @InvokeAction('MessageCreate', 'new')

=end

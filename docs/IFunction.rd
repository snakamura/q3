=begin
=@I

 Field @I(String account?, String subaccount?)


==����
����������킷���z�I�ȃw�b�_��Ԃ��܂��B���̃w�b�_�̓A�J�E���g�Ŏw�肵�����O�ƃ��[���A�h���X���g�p���āA

 I: ���O <���[���A�h���X>

�Ƃ����悤�ȃw�b�_������Ɖ��肵�Ď擾���ꂽ�w�b�_�ł��B

account��subaccount���w�肳���Ǝw�肳�ꂽ�A�J�E���g��T�u�A�J�E���g�̎����̖��O��A�h���X���擾���܂��B�w�肳��Ȃ��ꍇ�ɂ́A�R���e�L�X�g�A�J�E���g�ƌ��݂̃T�u�A�J�E���g���g�p���܂��B


==����
:String account
  �A�J�E���g��
:String subaccount
  �T�u�A�J�E���g��


==�G���[
*�����̐��������Ă��Ȃ��ꍇ
*�R���e�L�X�g�A�J�E���g���Ȃ��ꍇ�i�A�J�E���g���w�肵�Ȃ������ꍇ�j
*�w�肳�ꂽ�A�J�E���g��������Ȃ��ꍇ�i�A�J�E���g���w�肵���ꍇ�j
*UI�X���b�h�ȊO����Ăяo�����ꍇ�i�A�J�E���g�ƃT�u�A�J�E���g���w�肵�Ȃ������ꍇ�j


==����
*UI�X���b�h����̂݌Ăяo���\�i�A�J�E���g�ƃT�u�A�J�E���g���w�肵�Ȃ������ꍇ�j


==��
 # �����̖��O�ƃA�h���X����w�b�_���擾
 @I()
 
 # X-QMAIL-SubAccount�Ŏw�肳�ꂽ�T�u�A�J�E���g�̃A�h���X���擾
 @I(@Account(), X-QMAIL-SubAccount)

=end

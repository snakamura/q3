=begin
=@Messages

 MessageList @Messages(String folder?, Number id?)


==����
���b�Z�[�W�̃��X�g��Ԃ��܂��Bfolder���w�肳�ꂽ�ꍇ�ɂ͂��̃t�H���_���̃��b�Z�[�W�����ׂĕԂ��܂��B�����id���w�肳�ꂽ�ꍇ�ɂ͂���ID�̃��b�Z�[�W��Ԃ��܂��B�����w�肳��Ȃ������ꍇ�ɂ̓A�J�E���g���̂��ׂẴ��b�Z�[�W�̃��X�g��Ԃ��܂��B

�Ԃ��ꂽ���b�Z�[�W���X�g��((<@ForEach|URL:ForEachFunction.html>))��((<@FindEach|URL:FindEachFunction.html>))�Ȃǂ��g�p���ď������邱�Ƃ��ł��܂��B


==����
:String folder
  �t�H���_�̊��S��
:Number id
  ���b�Z�[�W��ID


==�G���[
*�����̐��������Ă��Ȃ��ꍇ
*�w�肳�ꂽ�t�H���_�����݂��Ȃ��ꍇ
*�R���e�L�X�g�A�J�E���g���Ȃ��ꍇ�i�t�H���_���w�肵�Ȃ��������A�t�H���_�̎w��ɃA�J�E���g�����܂܂�Ȃ������ꍇ�j
*�w�肳�ꂽ�t�H���_���ʏ�t�H���_�ł͂Ȃ��ꍇ�iid���w�肳�ꂽ�ꍇ�j
*UI�X���b�h�ȊO����Ăяo�����ꍇ�i�w�肵���t�H���_�̃A�J�E���g���R���e�L�X�g�A�J�E���g�ƈقȂ�ꍇ�j


==����
*UI�X���b�h����̂݌Ăяo���\�i�w�肵���t�H���_�̃A�J�E���g���R���e�L�X�g�A�J�E���g�ƈقȂ�ꍇ�j


==��
 # ��M���̂��ׂẴ��b�Z�[�W�����ǂɂ���
 @ForEach(@Messages('��M��'), @Seen(@True()))
 
 # ��M����ID��1000�̃��b�Z�[�W���폜����
 @ForEach(@Messages('��M��', 1000), @Delete())

=end

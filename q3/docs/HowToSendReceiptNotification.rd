=begin
=�J���ʒm�𑗂�ɂ͂ǂ�����Ηǂ��ł���?

�ȉ��̂悤�ȕ��@�ŊJ���ʒm�v�����t���Ă��郁�[���ɊJ���ʒm�𑗂邱�Ƃ��ł��܂��B

�J���ʒm�v�����t���Ă��郁�[���ɂ�Disposition-Notification-To�Ƃ����w�b�_���t���Ă��܂��B���̃w�b�_���t���Ă��郁�[����\�������Ƃ��Ƀ}�N�������s���邱�ƂŊJ���ʒm�𑗂�܂��B���̂Ƃ��A�������[���ɑ΂��ĉ��x���J���ʒm�𑗂��Ă��܂�Ȃ��悤�ɁAUser1�t���O���g���܂��B�܂�A�J���ʒm�𑗂�����User1�t���O�𗧂Ă�悤�ɂ��AUser1�t���O�������Ă��郁�[���ɑ΂��Ă͊J���ʒm�𑗂�Ȃ��悤�ɂ��܂��B

���[����\�������Ƃ��Ƀ}�N�������s�����邽�߂ɂ́A((<�\���p�̃e���v���[�g|URL:ViewTemplate.html>))���g���̂���ʓI�ł����A���������HTML���[���̕\�����ł��Ȃ��Ȃ铙�̐��񂪂��邽�߁A�����ł�((<header.xml|URL:HeaderXml.html>))�̒��Ƀ}�N���𖄂ߍ��ނ��Ƃɂ��܂��B�ȉ��̂悤�ȕ����������A

 <line class="mail|news">
  <static width="auto" style="bold" showAlways="true">����:</static>
  <edit number="6">{@Subject()}</edit>
 </line>

�ȉ��̂悤�ɏ��������܂��B

 <line class="mail|news">
  <static width="auto" style="bold" showAlways="true">����:</static>
  <edit number="6">{
  @Progn(@If(@And(@Not(@User1()), Disposition-Notification-To),
             @Progn(@User1(@True()),
                    @If(@Equal(@MessageBox(@Concat(Disposition-Notification-To,
                                                  '�ɊJ���ʒm�𑗂�܂���?'),
                                           4),
                               6),
                        @InvokeAction('MessageCreate', 'notification', '', @URI()),
                        @True())),
             @True()),
         @Subject())
  }</edit>
 </line>

�}�N���̒���Disposition-Notification-To���t���Ă��邩�ǂ����AUser1�t���O�������Ă��邩�ǂ����𒲂ׁA�J���ʒm�𑗂�K�v������΃��b�Z�[�W�{�b�N�X��\�����Ċm�F���Ă���((<@InvokeAction|URL:InvokeActionFunction.html>))���g�p����notification.template���g���ă��b�Z�[�W���쐬���Ă��܂��B

notification.template�͈ȉ��̂悤�ȃe���v���[�g��p�ӂ��āAtemplates\mail�̉��ɒu���܂��B

 To: {Disposition-Notification-To}
 Subject: �J���ʒm: {Subject}
 X-QMAIL-Account: {@Account()}{
   @If(@Identity(),
       @Concat('\nX-QMAIL-SubAccount: ', @SubAccount()),
       '')
 }
 X-QMAIL-EditMacro: @InvokeAction('FileSendNow')
 
 �ǂ݂܂�����`�B

���b�Z�[�W�̓��e�͓K���ɏ��������Ă��������B����ɂ���āA�J���ʒm�Ƃ��đ��郁�b�Z�[�W���쐬���܂��B�����ŁA((<X-QMAIL-EditMacro|URL:SpecialHeaders.html>))���g���āA�G�f�B�b�g�E�B���h�E���J�����O�Ƀ}�N�������s���Ă��܂��B���̒���((<FileSendNow�A�N�V����|URL:FileSendNowAction.html>))���g���āA�쐬�������b�Z�[�W�����ۂɑ��M���܂��B

=end

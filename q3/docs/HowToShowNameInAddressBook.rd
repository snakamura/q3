=begin
=��M�������[���̑��M�҂��A�h���X���ɓo�^�������O�ŕ\������ɂ͂ǂ�����Ηǂ��ł���?

==���X�g�r���[��[���M�� / ����]�̗��ɃA�h���X���ɓo�^�������O�ŕ\������

(1)���j���[����[�\��]-[�J�������J�X�^�}�C�Y]��I�����A((<[�J�����̃J�X�^�}�C�Y]�_�C�A���O|URL:ViewsDialog.html>))���J���܂��B

(2)���X�g�{�b�N�X��[���M�� / ����]�̍s��I�����A[�ҏW]�{�^�����N���b�N���A((<[�J����]�_�C�A���O|URL:ViewsColumnDialog.html>))���J���܂��B

(3)[�^�C�v]�R���{�{�b�N�X�Łu���̑��v��I�����A[�}�N��]�Ɉȉ��̃}�N�����w�肵�܂�
    @FormatAddress(From, :FORMAT-NAME, :LOOKUP-FORCE)
   ((<�J�����̃J�X�^�}�C�Y|"IMG:images/ShowNameInAddressBook.png">))

(4)[�t���O]��[�L���b�V��]�Ƀ`�F�b�N�������Ă��邱�Ƃ��m�F���܂��B

(5)����[OK]�{�^���������ă_�C�A���O����܂��B

:FORMAT-NAME�̑����:FORMAT-ALL���w�肷�邱�ƂŁA���O�ƃ��[���A�h���X�̗�����\�����邱�Ƃ��ł��܂��B�ڍׂ́A((<@FormatAddress|URL:FormatAddressFunction.html>))���Q�Ƃ��Ă��������B


==�w�b�_�r���[��[���M��]�̗��ɃA�h���X���ɓo�^�������O�ŕ\������

((<header.xml|URL:HeaderXml.html>))��ҏW���܂��B�ȉ��̂Ƃ����T���A

 <line class="mail|news">
  <static width="auto" style="bold" showAlways="true">���M��:</static>
  <edit background="{@If(@Not(@Param('Verify')), '', @Contain(@Param('Verify'), 'AddressMatch'), 'f5f6be', @Contain(@Param('Verify'), 'AddressMismatch'), 'ec7b95', '')}" number="4">{@FormatAddress(From, 3)}</edit>
  <static width="auto" style="bold" align="right" showAlways="true">���t:</static>
  <edit width="10em" number="5">{@FormatDate(@Date(Date), @Profile('', 'Global', 'DefaultTimeFormat'))}</edit>
 </line>

�ȉ��̂悤�ɏ��������܂��B

 <line class="mail|news">
  <static width="auto" style="bold" showAlways="true">���M��:</static>
  <edit background="{@If(@Not(@Param('Verify')), '', @Contain(@Param('Verify'), 'AddressMatch'), 'f5f6be', @Contain(@Param('Verify'), 'AddressMismatch'), 'ec7b95', '')}" number="4">{@FormatAddress(From, :FORMAT-ALL, :LOOKUP-FORCE)}</edit>
  <static width="auto" style="bold" align="right" showAlways="true">���t:</static>
  <edit width="10em" number="5">{@FormatDate(@Date(Date), @Profile('', 'Global', 'DefaultTimeFormat'))}</edit>
 </line>

=end

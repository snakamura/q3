=begin
=�֘A�t���ŃG�f�B�b�g�r���[���J�����Ƃ��̃A�J�E���g���Œ肷��ɂ͂ǂ�����΂悢�ł���?

mailto URL�Ɋ֘A�t��������ƁA�u���E�U�ȂǂŃ��[���A�h���X���N���b�N�����Ƃ��ɁAQMAIL3�̃G�f�B�b�g�r���[���J���܂��B���̂Ƃ��Ɏg����A�J�E���g�́A((<�R�}���h���C��|URL:CommandLine.html>))�Ő�������Ă���悤�Ɍ��܂�܂��B�܂�A

(1)���ݑI������Ă��郁�[���A�J�E���g
(2)�I������Ă���̂����[���A�J�E���g�ł͂Ȃ��ꍇ�ɂ́A((<qmail.xml|URL:QmailXml.html>))��Global/DefaultMailAccount�Ŏw�肳�ꂽ�A�J�E���g
(3)�w�肳��Ă��Ȃ������ꍇ�A��ԏ�ɂ��郁�[���A�J�E���g

�̏��Ɍ�������Č��܂�܂��B

��ɓ���̃A�J�E���g���g�������ꍇ�ɂ́Aurl.template��ҏW���āA

 X-QMAIL-Account: {@Account()}{
   @If(@Identity(),
       @Concat('\nX-QMAIL-SubAccount: ', @SubAccount()),
       '')
 }

�̕������A

 X-QMAIL-Account: Main

�̂悤�ɂ��܂��B�����ŁAMain�͏�Ɏg�p����A�J�E���g�̖��O�ł��B

=end

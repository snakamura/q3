=begin
=@Profile

 String @Profile(String path, String section, String key, String default?)


==����
�v���t�@�C���`����XML�t�@�C������l�����o���܂��B�v���t�@�C���`����((<qmail.xml|URL:QmailXml.html>))�Ɠ��l�̌`���ł��B�ڍׂ́A((<qmail.xml|URL:QmailXml.html>))���Q�Ƃ��Ă��������B

path�ɋ󕶎�����w�肷��ƃf�t�H���g��((<qmail.xml|URL:QmailXml.html>))����l���擾���܂��B���̏ꍇ�A���łɃ��[�h����Ă���qmail.xml����l�����[�h���邽�ߍ����Ƀ��[�h�ł��܂����A�����I�ɏ����������Ă���ꍇ�ɂ͌��݂̃t�@�C���̓��e�ƈقȂ�\��������܂��B����ȊO�̃p�X���w�肷��Ǝw�肳�ꂽ�t�@�C�����v���t�@�C���`����XML�t�@�C���Ƃ��ēǂݍ��݁A�l��Ԃ��܂��B���΃p�X���w�肷��ƃ��[���{�b�N�X�f�B���N�g������̑��΃p�X�Ƃ��ĉ��߂��܂��B

section��key�ɂ̓Z�N�V�����̖��O�ƃL�[�̖��O���w�肵�܂��Bdefault�ɂ͒l�����݂��Ȃ������ꍇ�ɕԂ��l���w�肵�܂��B�w�肵�Ȃ������ꍇ�ɒl�����݂��Ȃ��Ƌ󕶎����Ԃ��܂��B


==����
:String path
  �t�@�C���̃p�X
:String section
  �Z�N�V�����̖��O
:String key
  �L�[�̖��O
:String default
  �l�����݂��Ȃ������Ƃ��ɕԂ��l


==�G���[
*�����̐��������Ă��Ȃ��ꍇ
*�t�@�C���̓ǂݍ��݂Ɏ��s�����ꍇ�ipath�ɋ󕶎���ȊO���w�肵���ꍇ�j


==����
�Ȃ�


==��
 # qmail.xml��Global�Z�N�V������Bcc�̒l���擾�i�f�t�H���g��1�j
 @Profile('', 'Global', 'Bcc', '1')
 
 # C:\test.xml����Test�Z�N�V������Foo�̒l���擾
 @Profile('C:\\test.xml', 'Test', 'Foo')

=end

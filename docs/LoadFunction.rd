=begin
=@Load

 String @Load(String path, Boolean template?, String encoding?)


==����
�w�肳�ꂽ�p�X�̃t�@�C������ǂݍ��񂾓��e��Ԃ��܂��B�p�X�ɑ��΃p�X���w�肳�ꂽ�ꍇ�ɂ̓��[���{�b�N�X�f�B���N�g������̑��΃p�X�ɂȂ�܂��B

template��True���w�肷��Ɠǂݍ��񂾓��e���e���v���[�g�Ƃ��ď������A�������ʂ�Ԃ��܂��B

encoding���w�肷��Ǝw�肳�ꂽ�G���R�[�f�B���O�œǂݍ��݂܂��B�w�肳��Ȃ��ꍇ�ɂ̓V�X�e���̃f�t�H���g�̃G���R�[�f�B���O�œǂݍ��݂܂��B


==����
:String path
  �t�@�C���p�X
:Boolean template
  �e���v���[�g�Ƃ��ēǂݍ��ނ��ǂ���
:String encoding
  �G���R�[�f�B���O


==�G���[
*�����̐��������Ă��Ȃ��ꍇ
*�ǂݍ��݂Ɏ��s�����ꍇ
*�e���v���[�g�̏����Ɏ��s�����ꍇ�i�e���v���[�g�Ƃ��ēǂݍ��񂾏ꍇ�j


==����
�Ȃ�


==��
 # �t�@�C����ǂݍ���
 @Load('C:\\Temp\\Test.txt')
 
 # ���΃p�X�ŃG���R�[�f�B���O���w��
 @Load('profiles/qmail.xml', @False(), 'utf-8')
 
 # �e���v���[�g����������
 @Load('templates/mail/test.template', @True())

=end

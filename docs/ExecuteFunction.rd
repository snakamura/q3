=begin
=@Execute

 String @Execute(String command, String input?)


==����
command�Ŏw�肳�ꂽ�R�}���h�����s���܂��B

command���֘A�t����ꂽ�t�@�C���������ꍇ�ɂ͊֘A�t���Ńt�@�C�����J���܂��B

input���w�肳�ꂽ�ꍇ�ɂ́A�w�肳�ꂽ��������V�X�e���̃G���R�[�f�B���O�Ńo�C�g��ɕϊ��������ʂ��R�}���h�̕W�����͂ɓn���܂��B���̏ꍇ�A�W���o�͂���o�͂��ꂽ�o�C�g����V�X�e���̃G���R�[�f�B���O�ŕ�����ɕϊ��������ʂ�Ԃ��܂��Binput�̎w���Windows�łł̂݉\�ł��B

input���w�肳��Ȃ������ꍇ�ɂ́A�R�}���h�����s���ċ󕶎����Ԃ��܂��B


==����
:String command
  ���s����R�}���h
:String input
  �R�}���h�̕W�����͂ɓn��������


==�G���[
*�����̐��������Ă��Ȃ��ꍇ
*�R�}���h�̎��s�Ɏ��s�����ꍇ


==����
�Ȃ�


==��
 # ���������N��
 @Execute('notepad.exe')
 
 # �󔒕������܂ނƂ��ɂ�""�Ŋ���
 @Execute('"C:\\Program Files\\Test\\test.exe"')
 
 # ������n��
 @Execute('notepad.exe "C:\\Temp\\test.txt"')
 
 # �֘A�t��
 @Execute('C:/Temp/Test.doc')
 
 # �t�B���^
 @Execute('sed.exe -e "s/foo/bar/"', 'foo')

=end

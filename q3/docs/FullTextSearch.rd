=begin
=�S������

�S�������ł�((<Namazu|URL:http://www.namazu.org/>))��((<Hyper Estraier|URL:http://hyperestraier.sourceforge.net/>))���g�p�����S���������s���܂��BWindows�łł̂ݎg�p�ł��܂��B�S�������̃G���W���Ƃ��ĉ����g�����́A((<�����̐ݒ�|URL:OptionSearch.html>))�Ŏw�肵�܂��B�g�p����G���W���́A���炩���߃C���X�g�[�����Ă����K�v������܂��B

�S���������s���ɂ́A((<�A�J�E���g�̍쐬|URL:CreateAccount.html>))����[���b�Z�[�W�{�b�N�X]��[1���b�Z�[�W1�t�@�C��]�ō쐬����K�v������܂��B�܂��A�S�������p�̃C���f�b�N�X�͎����ł͍X�V����܂���̂œK�X�X�V����K�v������܂��B

((<�S������|"IMG:images/FullTextSearchPage.png">))


+[����������]
������������w�肵�܂��B


+[�t�H���_]
�����Ώۂ̃t�H���_���w�肵�܂��B�u(���ׂẴt�H���_)�v��I������ƑS�Ẵt�H���_���猟�����܂��B


+[�T�u�t�H���_������]
�`�F�b�N�����[�t�H���_]�Ŏw�肵���t�H���_�̑S�ẴT�u�t�H���_�������ΏۂɂȂ�܂��B


+[�V���������t�H���_���쐬����]
�ʏ�A�w�肵���������f�t�H���g�̌����t�H���_�ɐݒ肳��A���̌����t�H���_���J�����ƂŌ������ʂ�\�����܂��B���̃`�F�b�N�{�b�N�X�Ƀ`�F�b�N������ƁA�w�肵�������ŐV���Ɍ����t�H���_���쐬���A���̃t�H���_���J���܂��B�����t�H���_�ɂ��ẮA((<�t�H���_|URL:Folder.html>))���Q�Ƃ��Ă��������B


+[�C���f�b�N�X�̍X�V]
�S�������̃C���f�b�N�X���X�V���邽�߂̃R�}���h�����s���܂��B


==�C���f�b�N�X�̍X�V�ɂ���
�S�������̃C���f�b�N�X�͎����ł͍X�V����܂���B[�C���f�b�N�X�̍X�V]�{�^���������ƃC���f�b�N�X���X�V���邽�߂̃R�}���h�����s���܂��B�C���f�b�N�X���X�V���邽�߂̃o�b�`�t�@�C�����쐬���AWindows�̃^�X�N�X�P�W���[���ȂǂŒ���I�ɃC���f�b�N�X���X�V���邱�Ƃ������߂��܂��B

��������уC���f�b�N�X�X�V�̃R�}���h�͈ȉ��̂悤�ɂȂ�܂��B

*Namazu
  :�����R�}���h
    namazu -l -a "$condition" "$index"
  :�C���f�b�N�X�X�V�R�}���h
    mknmz.bat -a -h -O "$index" "$msg"
*Hyper Estraier
  :�����R�}���h
    estcmd search -ic $encoding -vu -sf -max -1 "$index" "$condition"
  :�C���f�b�N�X�X�V�R�}���h
    estcmd gather -cl -fm -cm -sd "$index" "$msg"

$����͂��܂镶����͈ȉ��̂悤�ɒu������܂��B

*�����R�}���h
  :$index
    �C���f�b�N�X�̃p�X
  :$condition
    ��������
  :$encoding
    �V�X�e���̃G���R�[�f�B���O
*�C���f�b�N�X�X�V�R�}���h
  :$msg
    ���b�Z�[�W�{�b�N�X�̃p�X
  :$index
    �C���f�b�N�X�̃p�X
  :$encoding
    �V�X�e���̃G���R�[�f�B���O

=end

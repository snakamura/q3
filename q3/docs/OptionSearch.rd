=begin
=�����̐ݒ�

[�I�v�V����]�_�C�A���O��[����]�p�l���ł͌����֌W�̐ݒ���s���܂��B

((<�����̐ݒ�|"IMG:images/OptionSearch.png">))


====[��{]
�}�N�������̐ݒ���s���܂��B

+[�}�N��]
�}�N�������Ŏg����}�N�����w�肵�܂��B$Condition��$Case�͂��ꂼ�ꌟ��������Ƒ啶������������ʂ��邩�ǂ����̒l�����s���Ɋ��蓖�Ă��܂��B�f�t�H���g�ł́u@Or(@Contain(%Subject, $Search, $Case), @Contain(%From, $Search, $Case), @Contain(%To, $Search, $Case))�v�ł��B

�����Ŏw�肳�ꂽ�}�N����@X()�ƍl����ƁA�������̃}�N���͈ȉ��̂悤�ɂȂ�܂��B

 @Progn(@Set('Search', <����������>),
        @Set('Case', <�啶���Ə���������ʂ���>),
        @X())

����Ɍ������̃I�v�V�����ŁA[���ׂẴw�b�_����������]��[�{������������]�Ƀ`�F�b�N����ꂽ�ꍇ�ɂ́A�ȉ��̂悤�ȃ}�N�����g���܂��B

*���ׂẴw�b�_����������ꍇ
  @Or(@X(), @Contain(@Decode(@Header()), $Search, $Case))
*�{������������ꍇ
  @Or(@X(), @Contain(@Body(), $Search, $Case))
*��L�̗���
  @Or(@X(),
      @Contain(@Decode(@Header()), $Search, $Case),
      @Contain(@Body(), $Search, $Case))


====[�S������]
�S�������Ŏg���G���W�����w�肵�܂��B

+[Namazu]
((<Namazu|URL:http://www.namazu.org/>))���g�p���܂��B

+[Hyper Estraier]
((<Hyper Estraier|URL:http://hyperestraier.sourceforge.net/>))���g�p���܂��B

+[�J�X�^��]
�J�X�^����I�������ꍇ�ɂ́A�����E�X�V���ɌĂяo���R�}���h���w�肵�܂��B�R�}���h�̎w��ł͈ȉ��̒u�������񂪎g�p�ł��܂��B

*������
  :$index
    �C���f�b�N�X�̃p�X
  :$condition
    ��������
  :$encoding
    �V�X�e���̃G���R�[�f�B���O
*�X�V��
  :$msg
    ���b�Z�[�W�{�b�N�X�̃p�X
  :$index
    �C���f�b�N�X�̃p�X
  :$encoding
    �V�X�e���̃G���R�[�f�B���O

�������ɂ͎w�肳�ꂽ�R�}���h�����s���A�W���o�͂��猋�ʂ���荞�݂܂��B�W���o�͂ɏo�͂���錋�ʂ͈ȉ��̂悤�Ȍ`���ł���K�v������܂��B

(1)��s�Ɉ�̃q�b�g�����t�@�C�������o�͂����
(2)��s�̈�ԍŌ�Ɍ����/�̌�낪�t�@�C�����ɂȂ��Ă���

((:(2):))�̏����ɍ��v���Ȃ��s�͖�������܂��B

=begin
=�e���v���[�g

�e���v���[�g�̓��b�Z�[�W���쐬����Ƃ���\������Ƃ��ȂǂɓK�p����܂��B���Ƃ��΁A�V�������b�Z�[�W���쐬����Ƃ��ɋ�̃��b�Z�[�W���쐬������A�ԐM����Ƃ��ɕԐM���̃��b�Z�[�W�����p�������b�Z�[�W���쐬�����肵�܂��B�\���p�̃e���v���[�g���g�p����ƁA��M�������b�Z�[�W�̕\�����@��ʏ�Ƃ͕ς��邱�Ƃ��ł��܂��B������ȂǁA���̑��̏�ʂŎg�p�����e���v���[�g������܂��B�����̃e���v���[�g��ҏW���邱�ƂŁA���[���쐬���ɒ�^����}��������A�������Ƃ��̃t�H�[�}�b�g��ύX���邱�Ƃ��ł��܂��B

�e���v���[�g�̃t�@�C���̓��[���{�b�N�X�f�B���N�g����templates�f�B���N�g���̉��ɒu����Ă��܂��Btemplates�f�B���N�g���ȉ��ɂ̓A�J�E���g�N���X���ƂɃf�B���N�g�������݂��A���̒��Ɋe�e���v���[�g�̃t�@�C�����u����Ă��܂��B�f�t�H���g�ł͈ȉ��̃e���v���[�g���u����Ă��܂��B

:new.template
  �V�K�쐬�p
:reply.template
  �ԐM�p
:reply_all.template
  �S���ɕԐM�p
:forward.template
  �]���p
:edit.template
  �ҏW�p
:print.template
  ����p
:quote.template
  ���p�p
:url.template
  mailto URL�����p

�e�e���v���[�g�̎g�����ɂ��Ă͈ȉ����Q�Ƃ��Ă��������B

*((<�쐬�p�̃e���v���[�g|URL:CreateTemplate.html>))
*((<�\���p�̃e���v���[�g|URL:ViewTemplate.html>))
*((<���̑��̃e���v���[�g|URL:OtherTemplate.html>))

�e���v���[�g�̃t�@�C���̓v���b�g�t�H�[���̃f�t�H���g�̃G���R�[�f�B���O�ŕۑ�����K�v������܂��B


==�e���v���[�g�̌�����
�쐬�p��\���p�̃e���v���[�g�͈ȉ��̏��Ɍ�������܂��B���Ƃ��΁Anew.template����������ꍇ�ɂ́A

(1)<�A�J�E���g�f�B���N�g��>/templates/new_<�t�H���_ID>.template
(2)<�A�J�E���g�f�B���N�g��>/templates/new.template
(3)<���[���{�b�N�X�f�B���N�g��>/templates/<�A�J�E���g�N���X>/new.template

���Ƃ��΁A���[���{�b�N�X�f�B���N�g����C:\Documents and Settings\test\Application Data\QMAIL3�ŁA���O��Main�Ƃ������[���p�̃A�J�E���g�ŁA���݂̃t�H���_����M���iID��1�j�������ꍇ�ɂ͈ȉ��̏��Ɍ�������܂��B

(1)C:\Documents and Settings\test\Application Data\QMAIL3\accounts\Main\templates\new_1.template
(2)C:\Documents and Settings\test\Application Data\QMAIL3\accounts\Main\templates\new.template
(3)C:\Documents and Settings\test\Application Data\QMAIL3\templates\mail\new.template

���̂悤�ɁA����̃A�J�E���g��t�H���_�Ŏg�p����e���v���[�g��؂�ւ������ꍇ�ɂ́A�K�؂ȃf�B���N�g���ɓK�؂Ȗ��O�Ńe���v���[�g��u�����Ƃɂ���ăe���v���[�g��؂�ւ��邱�Ƃ��ł��܂��B


==�e���v���[�g����
�e���v���[�g�̓e���v���[�g�����ɂ��������ċL�q����Ă��܂��B�e���v���[�g���쐬�E�ҏW����ꍇ�ɂ͂��̏����ɂ��������ċL�q����K�v������܂��B�e���v���[�g�����ŏ������e���v���[�g�́A���s���ɕ]������ĕ�����ɂȂ�܂��B

�e���v���[�g�����ł́A�e���v���[�g�̓��e�����ƃ}�N���ɕ������܂��B���e�����͂��̂܂ܕ]�����ʂƂȂ�A�}�N���͂��̃}�N����]���������ʂ��]�����ʂɂȂ�܂��B�}�N����{}�Ŋ����ċL�q���܂��B

���Ƃ��΁AFrom�Ŏw�肳�ꂽ�������To�w�b�_�Ɏw�肵�����ꍇ�ɂ͈ȉ��̂悤�ɏ������ƂɂȂ�܂��B

 To: {From}

�uTo: �v�����e�����ŁuFrom�v���}�N���ł��B�}�N���Ŋ����Ă��܂��B

���e�����̒���}�N���̒���{��}���g�������Ƃ��ɂ�{{��}}�̂悤�ɓ�d�ɂ��܂��B���Ƃ��Έȉ��̂悤�ɂȂ�܂��B

 {{����̓��e����}}
 {@RegexMatch(Date, /\d{{4}}/)}

��s�ڂ́u{����̓��e����}�v�Ƃ���������ɕ]������A��s�ڂł́u@RegexMatch(Date, /\d{4}/)�v�Ƃ����}�N����]���������ʂɕ]������܂��B

�}�N���ɂ��ẮA((<�}�N��|URL:Macro.html>))���Q�Ƃ��Ă��������B

=end

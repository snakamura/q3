=begin
=@FormatAddress

 String @FormatAddress(Field field, Number type?, Number lookup?)


==����
�����Ŏw�肳�ꂽ�t�B�[���h���A�h���X���X�g�Ƃ��ăp�[�X���A���̌��ʂ��t�H�[�}�b�g�����������Ԃ��܂��B�p�[�X�Ɏ��s�����ꍇ�ɂ͋󕶎����Ԃ��܂��B

type�Ɏw�肵���l�Ŋe�A�h���X�̃t�H�[�}�b�g���@������܂��B�ȉ��̂����ꂩ���w��ł��܂��B

::FORMAT-ALL
  ���O <�A�h���X>
::FORMAT-ADDRESS
  �A�h���X
::FORMAT-NAME
  ���O�i���O���w�肳�ꂽ���Ȃ��ꍇ�ɂ̓A�h���X�j
::FORMAT-VIEW
  ���O <�A�h���X>�i�������A���O�������G�X�P�[�v���Ȃ��j

�w�肵�Ȃ������ꍇ�ɂ�:FORMAT-ALL���w�肵���̂Ɠ����ɂȂ�܂��B:FORMAT-ALL��:FORMAT-VIEW�͎��Ă��܂����A�K�v�ȃG�X�P�[�v�����邩�ǂ������قȂ�܂��B���Ƃ��΁A

 To: "Test (Test)" <test@example.org>, "Yamada, Taro" <test2@example.org>

�Ƃ����w�b�_�������b�Z�[�W�ɁA@FormatAddress(To, :FORMAT-ALL)��K�p����Ɓu"Test (Test)" <test@example.org>, "Yamada, Taro" <test2@example.org>�v���Ԃ���܂����A@FormatAddress(To, :FORMAT-VIEW)��K�p����ƁuTest (Test) <test@example.org>, Yamada, Taro <test2@example.org>�v���Ԃ���܂��B���̂悤�ɁA:FORMAT-VIEW���w�肷��ƌ����ڏd���Ńt�H�[�}�b�g����邽�߁A���̕�������p�[�X���邱�Ƃ��ł��Ȃ��Ȃ�\��������܂��B

lookup�Ɏw�肵���l�Ŏg�p���閼�O���A�h���X������t�������邩�ǂ������w�肵�܂��B�ȉ��̂����ꂩ���w��ł��܂��B

::LOOKUP-NONE
  �t���������܂���
::LOOKUP-EMPTY
  ���O���w�肳��Ă��Ȃ��ꍇ�̂݋t���������܂�
::LOOKUP-FORCE
  ��ɋt���������܂�

�t�����������ꍇ�ɂ́A�t�H�[�}�b�g���ɖ��O�̕������A�h���X������t�����������O�Œu�������܂��B�A�h���X���ɋt���������A�h���X���܂܂�Ă��Ȃ��ꍇ�ɂ͒u�������͍s���܂���B�܂��A�����A�h���X�������̃G���g���ɕ\���ꍇ�ɂ͎n�߂Ɍ��������G���g�����g���܂��Blookup���w�肵�Ȃ������ꍇ�ɂ�:LOOKUP-NONE���w�肵���̂Ɠ����ɂȂ�܂��B


==����
:Field field
  �t�H�[�}�b�g����t�B�[���h
:Number type
  �t�H�[�}�b�g�̌`��
:Number lookup
  �A�h���X���̋t�������@


==�G���[
*�����̐��������Ă��Ȃ��ꍇ
*�����̌^�������Ă��Ȃ��ꍇ


==����
�Ȃ�


==��
 # From��\���p�ɋt�������ăt�H�[�}�b�g
 @FormatAddress(From, :FORMAT-VIEW, :LOOKUP-FORCE)
 
 # To���t�H�[�}�b�g
 @FormatAddress(To)

=end

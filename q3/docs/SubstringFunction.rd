=begin
=@Substring

 String @Substring(String s, Number offset, Number length?)


==����
s�Ŏw�肳�ꂽ������́Aoffset�Ŏw�肳�ꂽ�����ڂ���length�Ŏw�肳�ꂽ�����̕�����������擾���܂��Boffset��0�x�[�X�Ŏw�肵�܂��Blength���w�肵�Ȃ��ꍇ�Ⓑ����������̍Ō�𒴂���ꍇ�ɂ͕�����̍Ō�܂ł��擾���܂��Boffset��s�̒��������傫���ꍇ�ɂ͋󕶎����Ԃ��܂��B


==����
:String s
  ������
:Number offset
  0�x�[�X�̃I�t�Z�b�g
:Number length
  ����


==�G���[
*�����̐��������Ă��Ȃ��ꍇ


==����
�Ȃ�


==��
 # 3�����ڂ���4�����擾
 # -> defg
 @Substring('abcdefghij', 3, 4)
 
 # 5�����ڈȍ~���擾
 # -> fghij
 @Substring('abcdefghij', 5)

=end

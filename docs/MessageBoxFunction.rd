=begin
=@MessageBox

 Number @MessageBox(String message, Number type?)


==����
message�Ɏw�肳�ꂽ���b�Z�[�W��\�����郁�b�Z�[�W�{�b�N�X��\�����܂��Btype�ɂ̓��b�Z�[�W�{�b�N�X�̃^�C�v���w�肵�܂��B���b�Z�[�W�{�b�N�X�̃^�C�v�́A((<MessageBox|URL:http://msdn.microsoft.com/library/en-us/winui/winui/windowsuserinterface/windowing/dialogboxes/dialogboxreference/dialogboxfunctions/messagebox.asp?frame=true>)) API��uType�Ɏw��ł���l���w��ł��܂��B�w�肳��Ȃ������ꍇ�ɂ́AMB_OK | MB_ICONINFORMATION���w�肳�ꂽ�̂Ɠ����ɂȂ�܂��B���b�Z�[�W�{�b�N�X��\��������ŁAMessageBox API�̕Ԃ�l��Ԃ��܂��B


==����
:String message
  ���b�Z�[�W
:Number type?
  �^�C�v


==�G���[
*�����̐��������Ă��Ȃ��ꍇ
*UI���Ȃ��ꍇ


==����
*UI���K�v


==��
 # ���b�Z�[�W�{�b�N�X��\��
 @MessageBox('Test')
 
 # ���b�Z�[�W�{�b�N�X�ɖ{����\��
 @MessageBox(@Body(:BODY-INLINE))
 
 # Yes/No��q�˂郁�b�Z�[�W�{�b�N�X��\�����ď����𕪂���
 @If(@Equal(@MessageBox('�����𑱂��܂���?', 292), 6),
     @MessageBox('���ɂ傲�ɂ�'),
     @Exit())

=end

=begin
=@SaveAttachment

 String @SaveAttachment(String path, Expr expr?, Part part?)


==����
�R���e�L�X�g���b�Z�[�W�̓Y�t�t�@�C����path�Ŏw�肳�ꂽ�f�B���N�g���ɕۑ����܂��Bpart���w�肳�ꂽ�ꍇ�ɂ́A���̃p�[�g�̓Y�t�t�@�C����ۑ����܂��B�ۑ���̃t�@�C�������ɑ��݂���ꍇ�͏㏑�����܂��B

expr���w�肳�ꂽ�ꍇ�ɂ́A�e�Y�t�t�@�C����ۑ�����O�Ɏw�肵�������Ăяo����܂��B���̂Ƃ���Ԗڂ̈����ɓY�t�t�@�C�������A��Ԗڂ̈����ɓY�t�t�@�C���̃p�[�g���n����܂��B���̎����Ԃ��������񂪃t�@�C�����ɂȂ�܂��Bexpr���w�肳��Ȃ��ꍇ�ɂ́A�Y�t�t�@�C���������̂܂܃t�@�C�����ɂȂ�܂��B

�Ō�ɕۑ������Y�t�t�@�C���̃p�X��Ԃ��܂��B


==����
:String path
  �ۑ���̃f�B���N�g���̃p�X
:Expr expr
  �t�@�C���������߂鎮
:Part part
  �p�[�g


==�G���[
*�����̐��������Ă��Ȃ��ꍇ
*���b�Z�[�W�̎擾�Ɏ��s�����ꍇ
*�w�肵���p�[�g���Ȃ��ꍇ�ipart���w�肵���ꍇ�j
*�ۑ��Ɏ��s�����ꍇ


==����
�Ȃ�


==��
 # ���ׂĂ̓Y�t�t�@�C����C:\Temp�ɕۑ�����
 @SaveAttachment('C:\\Temp')
 
 # �t�@�C�����̐擪�ɓ��t�����ĕۑ�����
 @SaveAttachment('C:\\Temp',
                 @Concat(@FormatDate(@Date(), '%Y4%M0%D'), '_', $1))

=end

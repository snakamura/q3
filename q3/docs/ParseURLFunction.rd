=begin
=@ParseURL

 String @ParseURL(String url)


==����
url�Ŏw�肳�ꂽmailto URL���p�[�X���ăw�b�_�`���̕�����i�Ƃ���Ζ{���j��Ԃ��܂��B

���Ƃ��΁A<mailto:test@example.org>����́A

 To: test@example.org

���A<mailto:test@example.org?Cc=test2@example.org?Subject=Test&Body=Test%20Body>����́A

 To: test@example.org
 Cc: test2@example.org
 Subject: Test
 
 Test Body

��Ԃ��܂��B���������w�b�_�́ATo, Cc, Subject, In-Reply-To, References��body�ł��BURL���̔�ASCII�������UTF-8�Ƃ��ĉ��߂���܂��B

url�Ŏw�肳�ꂽ������mailto:����n�܂��Ă��Ȃ������ꍇ�ɂ́ATo�Ƃ��Ďw�肳�ꂽ��������������w�b�_�`���̕������Ԃ��܂��B���Ƃ��΁Atest3@example.org�������Ƃ��ēn���ƁA

 To: test3@example.org

��Ԃ��܂��B


==����
:String url
  �p�[�X����mailto URL


==�G���[
*�����̐��������Ă��Ȃ��ꍇ


==����
�Ȃ�


==��
 # url�Ƃ����ϐ��ɐݒ肳�ꂽURL���p�[�X
 @ParseUrl($url)

=end

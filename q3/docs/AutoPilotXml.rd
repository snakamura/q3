=begin
=autopilot.xml

((<��������|URL:AutoPilot.html>))�̐ݒ������XML�t�@�C���ł��B���̃t�@�C���ɂ́A((<��������̐ݒ�|URL:OptionAutoPilot.html>))�Őݒ肵����񂪕ۑ�����܂��B


==����

===autoPilot�G�������g

 <autoPilot>
  <!-- entry -->
 </autoPilot>

autoPilot�G�������g���g�b�v���x���G�������g�ɂȂ�܂��B���̉���0�ȏ��entry�G�������g���������Ƃ��ł��܂��B


===entry�G�������g

 <entry>
  <!-- course, interval -->
 </entry>

entry�G�������g���e����̃R�[�X�ƃC���^�[�o�����w�肵�܂��B���̉��ɁAcourse�G�������g��interval�G�������g���ЂƂ��u����܂��B


===course�G�������g

 <course>
  �R�[�X
 </course>

����R�[�X���w�肵�܂��B


===interval�G�������g

 <interval>
  ����Ԋu�i���P�ʁj
 </interval>

�w�肵���R�[�X�����񂷂�Ԋu�𕪒P�ʂŎw�肵�܂��B


==�T���v��

 <?xml version="1.0" encoding="utf-8"?>
 <autoPilot>
  <entry>
   <course>Inbox</course>
   <interval>5</interval>
  </entry>
  <entry>
   <course>RSS</course>
   <interval>15</interval>
  </entry>
 </autoPilot>


==�X�L�[�}

 element autoPilot {
   element entry {
     ## �R�[�X
     element course {
       xsd:string
     },
     ## �Ԋu�i���P�ʁj
     element interval {
       xsd:int
     }
   }*
 }

=end

=begin
=texts.xml

((<��^��|URL:FixedFormText.html>))�̐ݒ������XML�t�@�C���ł��B���̃t�@�C���ɂ́A((<��^���̐ݒ�|URL:OptionTexts.html>))�Őݒ肵����񂪕ۑ�����܂��B


==����

===texts�G�������g

 <texts>
   <!-- text -->
 </texts>

texts�G�������g���g�b�v���x���G�������g�ɂȂ�܂��B�q�G�������g�Ƃ���0�ȏ��text�G�������g��u�����Ƃ��o���܂��B


===text�G�������g

 <text
  name="���O">
  <!-- �e�L�X�g -->
 </text>

text�G�������g�ɂ͑}�������`�����L�q���܂��Bname�����ɖ��O���w�肵�܂��B


==�T���v��

 <?xml version="1.0" encoding="utf-8"?>
 <texts>
  <text name="hello">����ɂ���</text>
 </texts>


==�X�L�[�}

 element texts {
   element text {
     ## �e�L�X�g
     xsd:string,
     ## ���O
     attribute name {
       xsd:string
     }
   }*
 }

=end

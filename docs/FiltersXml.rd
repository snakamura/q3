=begin
=filters.xml

((<�t�B���^|URL:Filter.html>))�̐ݒ������XML�t�@�C���ł��B���̃t�@�C���ɂ́A((<�t�B���^�̐ݒ�|URL:OptionFilters.html>))�Őݒ肵����񂪕ۑ�����܂��B


==����

===filters�G�������g

 <filters>
  <!-- filter -->
 </filters>

filters�G�������g���g�b�v���x���G�������g�ɂȂ�܂��Bfilters�G�������g�ȉ��ɂ�0�ȏ��filter�G�������g��u�����Ƃ��ł��܂��B


===filter�G�������g

 <filter
  name="���O">
  �}�N��
 </filter>

filter�G�������g�ɂ̓t�B���^�̖��O�ƃt�B���^����̂Ɏg�p����}�N�����w�肵�܂��Bname�����ɂ̓t�B���^�̖��O���w�肵�܂��B


==�T���v��

 <?xml version="1.0" encoding="utf-8"?>
 <filters>
  <filter name="���ǂ̂�">@Not(@Seen())</filter>
  <filter name="10KB�ȏ�">@Greater(@Size(), 10240)</filter>
 </filters>


==�X�L�[�}

 element filters {
   element filter {
     ## �}�N��
     xsd:string,
     ## ���O
     attribute name {
       xsd:string
     }
   }*
 }

=end

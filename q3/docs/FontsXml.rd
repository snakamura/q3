=begin
=fonts.xml

�t�H���g�̐ݒ������XML�t�@�C���ł��B


==����

===fonts�G�������g

 <fonts>
  <!-- group -->
 </fonts>

fonts�G�������g���g�b�v���x���G�������g�ɂȂ�܂��Bfonts�G�������g�ȉ��ɂ�0�ȏ��group�G�������g��u�����Ƃ��ł��܂��B


===group�G�������g

 <group
  name="���O">
  <!-- fontSet -->
 </group>

group�G�������g�̓t�H���g�O���[�v��\���܂��Bname�����ɃO���[�v�����w�肵�܂��Bgroup�G�������g�ȉ���0�ȏ��fontSet�G�������g��u�����Ƃ��ł��܂��B


===fontSet�G�������g

 <fontSet
  match="�}�N��"
  lineSpacing="�s��">
  <!-- font -->
 </fontSet>

fontSet�G�������g�̓t�H���g�Z�b�g��\���܂��B

match�����Ƀ}�N�����w�肵�܂��B�\�����郁�b�Z�[�W�ɑ΂��Ă��̃}�N����]���������ʂ�True�ɂȂ�Ƃ��̃t�H���g�Z�b�g���g���܂��B

lineSpacing�����ɂ͍s�Ԃ��s�N�Z���P�ʂŎw�肵�܂��B


===font�G�������g

 <font
  face="�t�H���g��"
  size="�T�C�Y"
  style="�X�^�C��"
  charset="�����Z�b�g"/>

font�����ɂ͎��ۂɎg����t�H���g���w�肵�܂��Bface�����ɂ̓t�H���g�����Asize�����ɂ̓T�C�Y���|�C���g�P�ʂŎw�肵�܂��Bsize���w�肵�Ȃ��ꍇ�ɂ�9�|�C���g�ɂȂ�܂��Bstyle�����ɂ�bold, italic, underline, strikeout�̑g�ݍ��킹���󔒕�����؂�Ŏw��ł��܂��B�����Z�b�g�ɂ̓t�H���g�̕����Z�b�g���w�肵�܂��B


==�T���v��

 <?xml version="1.0" encoding="utf-8"?>
 <fonts>
  <group name="main">
   <fontSet match="@Progn(@Set('charset', @BodyCharset()),
                          @Or(@BeginWith($charset, 'iso-8859-'),
                              @Equal($charset, 'us-ascii')))">
    <font face="Tahoma" size="9"/>
   </fontSet>
   <fontSet match="@Equal(@Folder(), '�����}�K')">
    <font face="�l�r �S�V�b�N" size="9"/>
   </fontSet>
   <fontSet>
    <font face="�l�r �o�S�V�b�N" size="9"/>
   </fontSet>
  </group>
 </fonts>

==�X�L�[�}

 element fonts {
   element group {
     element fontSet {
       element font {
         ## �t�H���g��
         attribute face {
           xsd:string
         },
         ## �T�C�Y�i�|�C���g�j
         attribute size {
           xsd:float
         }?,
         ## �X�^�C��
         ## bold, italic, underline, strikeout
         attribute style {
           xsd:string
         }?,
         ## �����Z�b�g
         attribute charset {
           xsd:nonNegativeInteger
         }?
       },
       ## �}�N��
       attribute match {
         xsd:string
       }?,
       ## �s��
       attribute lineSpacing {
         xsd:nonNegativeInteger
       }?
     }*,
     ## �O���[�v��
     attribute name {
       xsd:string
     }
   }*
 }

=end

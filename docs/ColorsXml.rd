=begin
=colors.xml

((<���X�g�r���[�̐F|URL:Colors.html>))�̐ݒ������XML�t�@�C���ł��B���̃t�@�C���ɂ́A((<�F�̐ݒ�|URL:OptionColors.html>))�Őݒ肵����񂪕ۑ�����܂��B


==����

===colors�G�������g

 <colors>
  <! -- colorSet -->
 </colors>

colors�G�������g���g�b�v���x���G�������g�ɂȂ�܂��Bcolors�G�������g�ȉ��ɂ́A0�ȏ��colorSet�G�������g��u�����Ƃ��o���܂��B


===colorSet�G�������g

 <colorSet
  account="�A�J�E���g��"
  folder="�t�H���_��">
  <!-- color -->
 </colorSet>

colorSet�G�������g�͐F�������Ɏg�p�����F�Z�b�g���w�肵�܂��Baccount�����ɃA�J�E���g�����Afolder�����Ƀt�H���_�����w�肷�邱�Ƃ��o���܂��B�����Ƃ�//�ň͂ނ��Ƃɂ�萳�K�\�����g�p�ł��܂��B�����̑����͏ȗ��\�ŁA�ȗ����ꂽ�ꍇ�ɂ͂��ꂼ��S�ẴA�J�E���g�E�t�H���_�Ƀ}�b�`���܂��B���Ƃ��΁Aaccount�����݂̂��w�肵��folder�������w�肵�Ȃ��ƁA�w�肵���A�J�E���g�̑S�Ẵt�H���_�Ƀ}�b�`���܂��B


===color�G�������g

 <color
  match="�}�N��"
  description="����">
  <!-- foreground, background, style -->
 </color>

color�G�������g�͐F�ƃt�H���g�̃X�^�C�����w�肵�܂��Bmatch�����ɂ̓}�N�����Adescription�����ɂ͐������w�肵�܂��B


===foreground�G�������g

 <foreground>
  �F
 </foreground>

foreground�G�������g�͕����F���w�肵�܂��B�F��RRGGBB�`���Ŏw�肵�܂��B


===background�G�������g

 <background>
  �F
 </background>

background�G�������g�͔w�i�F���w�肵�܂��B�F��RRGGBB�`���Ŏw�肵�܂��B


===style�G�������g

 <style>
  �t�H���g�̃X�^�C��
 </style>

style�G�������g�ɂ̓t�H���g�̃X�^�C�����w�肷�邱�Ƃ��ł��܂��B�w��\�Ȃ̂́Aregular��bold�ł��B


==�T���v��

 <?xml version="1.0" encoding="utf-8"?>
 <colors>
  <colorSet account="test" folder="Inbox">
   <color match="@Not(@Seen())">
    <foreground>ff0000</foreground>
    <style>bold</style>
   </color>
   <color match="@BeginWith(%Subject, '[Qs:')">
    <foreground>00ff00</foreground>
   </color>
  </colorSet>
 </colors>


==�X�L�[�}

 element colors {
   element colorSet {
     element color {
       ## �����F
       element foreground {
         xsd:string {
           pattern = "[0-9a-fA-F]{6}"
         }
       }?,
       ## �w�i�F
       element background {
         xsd:string {
           pattern = "[0-9a-fA-F]{6}"
         }
       }?,
       ## �t�H���g�X�^�C��
       element style {
         "regular" | "bold"
       }?,
       ## �F���K�p���������i�}�N���j
       attribute match {
         xsd:string
       },
       attribute description {
         xsd:string
       }?
     }*,
     ## �A�J�E���g
     ## �w�肳��Ȃ��ꍇ�A�S�ẴA�J�E���g
     attribute account {
       xsd:string
     }?,
     ## �t�H���_
     ## �w�肳��Ȃ��ꍇ�A�S�Ẵt�H���_
     attribute folder {
       xsd:string
     }?
   }*
 }

=end

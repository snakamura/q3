=begin
=addressbook.xml

((<�A�h���X��|URL:AddressBook.html>))�̏���ۑ�����ݒ������XML�t�@�C���ł��B


==����

===addressBook�G�������g

 <addressBook>
  <!-- entry -->
 </addressBook>

addressBook�G�������g���g�b�v���x���G�������g�ɂȂ�܂��B���̃G�������g�ȉ��ɂ́A0�ȏ��entry�G�������g��u�����Ƃ��o���܂��B


===entry�G�������g

 <entry>
  <!-- name, sortKey, addresses -->
 </entry>

entry�G�������g�͈�̃G���g����\���܂��B�ʏ�G���g���͂ЂƂ�̐l�ɑΉ����܂��B�G���g���ɂ͕����̃A�h���X���܂߂邱�Ƃ��o���܂��B


===name�G�������g

 <name>
  <!-- ���O -->
 </name>

name�G�������g�ɂ̓G���g���̖��O���w�肵�܂��B����͒ʏ킻�̃G���g���̐l�̖��O�ɂȂ�܂��B


===sortKey�G�������g

 <sortKey>
  <!-- �\�[�g�L�[ -->
 </sortKey>

sortKey�G�������g�ɂ̓G���g���̃\�[�g�L�[���w�肵�܂��B����͒ʏ킻�̃G���g���̂ӂ肪�Ȃ��w�肵�܂��BsortKey���w�肵�Ȃ���name�G�������g�̒l���\�[�g�L�[�ɂȂ�܂��B


===addresses�G�������g

 <addresses>
  <!-- address ->
 </addresses>

addresses�G�������g�̓��[���A�h���X�̃��X�g��\���܂��Baddresses�G�������g�́A1�ȏ��address�G�������g���܂݂܂��B


===address�G�������g

 <address
  alias="�G�C���A�X��"
  category="�J�e�S����"
  comment="�R�����g"
  rfc2822="boolean"
  certificate="�ؖ����̖��O">
  <!-- �A�h���X -->
 </address>

address�G�������g�ň�̃��[���A�h���X���w�肵�܂��B

alias�����ŃG�C���A�X�����w�肵�܂��B�����Ŏw�肵���G�C���A�X�����G�f�B�b�g�r���[��To�t�B�[���h�ȂǂɎw�肷��Ǝ����I�ɓW�J����܂��B

category�����ŃJ�e�S�����w�肵�܂��B�J�e�S���́A�u/�v�ŋ�؂邱�ƂŊK�w���ł��܂��B��̃A�h���X�ɕ����̃J�e�S�����w�肷��ꍇ�ɂ́A�u,�v�ŋ�؂�܂��B

comment�����ŃA�h���X�ɃR�����g�����邱�Ƃ��o���܂��B

rfc2822������true���w�肷��ƁA�w�肳�ꂽ���[���A�h���X�͊���RFC2822�`���ł���Ƃ��ď�������܂��B����ȊO�̏ꍇ�ɂ́A�G���g���̖��O�ƃ��[���A�h���X����RFC2822�`���𐶐����܂��B�f�t�H���g��false�ł��B

certificate������S/MIME�Ŏg�p����ؖ����̖��O���w�肵�܂��B�ؖ����́Asecurity/<�w�肵�����O>.pem�Ƃ����t�@�C�����烍�[�h����܂��B


==�T���v��

 <?xml version="1.0" encoding="utf-8"?>
 <addressBook>
  <entry>
   <name>Hogehoge Fuga</name>
   <addresses>
    <address alias="hoge" comment="����">hoge@foo.com</address>
    <address category="�����/�ǂ�����" rfc2822="true"
     >�ق��ق��l &lt;hogefuga@dokosoko.co.jp></address>
   </addresses>
 </addressBook>


==�X�L�[�}

 element addressBook {
   element entry {
     ## ���O
     element name {
       xsd:string
     },
     element sortKey {
       xsd:string
     }?,
     element addresses {
       element address {
         ## �A�h���X
         xsd:string,
         ## �G�C���A�X
         attribute alias {
           xsd:string
         }?,
         ## �J�e�S���i'/'�ŋ�؂��ĊK�w���j
         ## �����w�肷��ꍇ�ɂ�','�ŋ�؂�
         attribute category {
           xsd:string {
             pattern = "([^/,]+(/[^/,]+)*)+(,([^/,]+(/[^/,]+)*))*"
           }
         }?,
         ## �R�����g
         attribute comment {
           xsd:string
         }?,
         ## �A�h���X��RFC2822�`���ɂȂ��Ă��邩�ǂ���
         ## true�̏ꍇ�A�A�h���X�����̂܂܎g�p�����
         ## false�̏ꍇ�A���O�ƃA�h���X����RFC2822�`�������������
         ## �w�肳��Ȃ��ꍇ�Afalse
         attribute rfc2822 {
           xsd:boolean
         }?,
         ## �ؖ����̖��O
         attribute certificate {
           xsd:string
         }?
       }+
     }
   }*
 }

=end

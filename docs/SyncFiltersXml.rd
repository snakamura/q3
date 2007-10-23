=begin
=syncfilters.xml

((<�����t�B���^|URL:SyncFilter.html>))�̐ݒ������XML�t�@�C���ł��B���̃t�@�C���ɂ́A((<�����t�B���^�̐ݒ�|URL:OptionSyncFilters.html>))�Őݒ肵����񂪕ۑ�����܂��B


==����

===filters�G�������g

 <filters>
  <!-- filterSet -->
 </filters>

filters�G�������g���g�b�v���x���G�������g�ɂȂ�܂��B���̉���0�ȏ��filterSet��u�����Ƃ��o���܂��B


===filterSet�G�������g

 <filterSet
  name="���O">
  <!-- filter -->
 </filterSet>

filterSet�G�������g�̓t�B���^�̃Z�b�g���w�肵�܂��Bname�����Ńt�B���^�Z�b�g�̖��O���w�肵�܂��B


===filter�G�������g

 <filter
  folder="�t�H���_��"
  match="�}�N��">
  <!-- action -->
 </filter>

filter�G�������g�Ńt�B���^���w�肵�܂��Bmatch�����Ƀ}�N�����w�肵�܂��B���̃}�N����]���������ʂ��^�ɂȂ�t�B���^�̃A�N�V���������s����܂��Bfolder�������w�肷��ƁA�w�肵���t�H���_�𓯊�����Ƃ��ɂ̂ݎg�p����܂��B//�ň͂ނ��Ƃɂ�萳�K�\�����g�p�ł��܂��B

filter�G�������g�͏ォ�珇�Ԃɕ]������A�ŏ��Ƀ}�N�����^�ɂȂ����t�B���^���g�p����܂��B

���filter�G�������g�̉��ɂ͈�ȏ��action�G�������g��u�����Ƃ��o���܂��B�A�N�V�����G�������g�͏ォ�珇�Ԃɕ]������A���s����܂��B


===action�G�������g

 <action
  name="���O">
  <!-- param -->
 </action>

action�G�������g�̓t�B���^���ǂ̂悤�ȓ��������̂����w�肵�܂��Bname�����ŃA�N�V�����̖��O���w�肵�܂��B�ǂ̂悤�ȃA�N�V����������̂��͔��l���Q�Ƃ��Ă��������B


===param�G�������g

 <param
  name="���O">
  �l
 </param>

param�G�������g�̓A�N�V�����̃p�����[�^���w�肵�܂��Bname�����Ńp�����[�^�̖��O���w�肵�A�q�m�[�h�Ƃ��Ēl���w�肵�܂��B�A�N�V�����ɂǂ̂悤�ȃp�����[�^������̂��͔��l���Q�Ƃ��Ă��������B


==�T���v��

 <?xml version="1.0" encoding="utf-8"?>
 <filters>
  <filterSet name="test_main">
   <filter folder="Inbox" match="@Greater(@Size(),10240)">
    <action name="download">
     <param name="type">header</param>
    </action>
   </filter>
   <filter folder="Inbox" match="@And(@Or(
    @Contain(@Address(To),'foo.com'),
    @Contain(@Address(From),'foo.com')))">
    <action name="download">
     <param name="type">text</param>
    </action>
   </filter>
   <filter folder="Inbox" match="@And(@Or(
    @Contain(@Address(To),'foo.com'),
    @Contain(@Address(From),'foo.com')),
    @Less(@Size(@True()),10240))">
    <action name="download">
     <param name="type">text</param>
    </action>
   </filter>
   <filter folder="Inbox" match="@True()">
    <action name="download">
     <param name="type">header</param>
    </action>
   </filter>
  </filterSet>
  <filterSet name="test2_main">
   <filter match="Greater(@Size(),10240)">
    <action name="download">
     <param name="line">1000</param>
    </action>
   </filter>
  </filterSet>
 </filters>


==�X�L�[�}

 element filters {
   element filterSet {
     element filter {
       element action {
         element param {
           ## �p�����[�^�̒l
           xsd:string,
           ## �p�����[�^�̖��O
           attribute name {
             xsd:string
           }
         }*,
         ## �A�N�V�����̖��O
         attribute name {
           xsd:string
         }
       }+,
       ## �t�B���^���K�p�����t�H���_
       ## �w�肳��Ȃ��ꍇ�A�S�Ẵt�H���_
       attribute folder {
         xsd:string
       }?,
       ## �t�B���^���}�b�`��������i�}�N���j
       attribute match {
         xsd:string
       }
     }*,
     ## �t�B���^�Z�b�g�̖��O
     attribute name {
       xsd:string
     }
   }*
 }


==���l
���ݎw��ł���A�N�V�����͈ȉ��̂Ƃ���ł��B�A�N�V�����̓v���g�R�����ƂɈقȂ�܂��B

===POP3

====download�A�N�V����
���b�Z�[�W���_�E�����[�h���܂��Bline�p�����[�^�ɍő�s�����w�肵�܂��B


====delete�A�N�V����
���b�Z�[�W���T�[�o�ォ��폜���܂��B


====ignore�A�N�V����
���b�Z�[�W���_�E�����[�h���܂���Bignore�A�N�V�������w�肷��ƃ��X�g�ɂ��\������Ȃ��Ȃ�܂��B


===IMAP4

====download�A�N�V����
���b�Z�[�W���_�E�����[�h���܂��Btype�p�����[�^�Ń^�C�v���w�肵�܂��B

�w��ł���^�C�v�͈ȉ��̂Ƃ���ł��B

:all
  �S��
:text
  �e�L�X�g�Ƃ��ĕ\������̂ɕK�v�ȕ����̂�
:html
  HTML���[���Ƃ��ĕ\������̂ɕK�v�ȕ����̂�
:header
  �w�b�_�̂�


====delete�A�N�V����
���b�Z�[�W���T�[�o�ォ��폜���܂��i�폜�t���O�𗧂Ă܂��j�B


===NNTP

====download�A�N�V����
���b�Z�[�W���_�E�����[�h���܂��B


====ignore�A�N�V����
���b�Z�[�W���_�E�����[�h���܂���Bignore�A�N�V�������w�肷��ƃ��X�g�ɂ��\������Ȃ��Ȃ�܂��B

=end

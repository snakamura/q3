=begin
=goround.xml

((<����|URL:GoRound.html>))�̐ݒ������XML�t�@�C���ł��B���̃t�@�C���ɂ́A((<����̐ݒ�|URL:OptionGoRound.html>))�Őݒ肵����񂪕ۑ�����܂��B


==����

===goround�G�������g

 <goround>
  <!-- course -->
 </goround>

goround�G�������g���g�b�v���x���G�������g�ɂȂ�܂��Bgoround�G�������g�ȉ��ɂ́A0�ȏ��course�G�������g��u�����Ƃ��o���܂��B


===course�G�������g

 <course
  name="�R�[�X��"
  confirm="�m�F���邩�ǂ���">
  <!-- dialup, sequential, parallel -->
 </course>

course�G�������g�͈�̏���R�[�X��\���܂��Bname�����ŃR�[�X�����w�肵�܂��Bconfirm������true���Ƃ��̃R�[�X�ŏ��񂷂�O�Ɋm�F�̃_�C�A���O���\������܂��B

course�G�������g�̎q�G�������g�Ƃ��āAdialup�G�������g����сAsequential, parallel�G�������g�̂ǂ��炩��u�����Ƃ��o���܂��B


===dialup�G�������g

 <dialup
  name="�_�C�A���A�b�v�G���g����"
  dialFrom="���M��"
  showDialog="�_�C�A���A�b�v�_�C�A���O��\�����邩�ǂ���"
  disconnectWait="�ؒf�O�ɑҋ@����b��"
  wheneverNotConnected="�l�b�g���[�N�ڑ����Ȃ��Ƃ��ɂ̂ݐڑ����邩"/>

dialup�G�������g��course�G�������g�̍ŏ��̎q�G�������g�Ƃ��Ă������Ƃ��o���܂��B���̃G�������g���Ȃ��ꍇ�ɂ̓_�C�A���A�b�v�͍s���܂���B

name�����Ń_�C�A���A�b�v�ڑ��̃G���g�������w�肵�܂��B�w�肵�Ȃ��ꍇ�ɂ͎��s���ɃG���g����I������_�C�A���O���\������܂��BdialFrom�Ŕ��M�����w�肷�邱�Ƃ��o���܂��B�w�肵�Ȃ��ꍇ�ɂ́A���݂̔��M�����g�p���܂��B

showDialog ������true���w�肷��ƁA�_�C�A���O�Ƀ_�C�A���O��\�����܂��BdisconnectWait�����ɐ������w�肷��ƁA�ؒf�O�Ɏw�肵���b���ҋ@���A���̊Ԃɐؒf���L�����Z�����邱�Ƃ��o���܂��BwheneverNotConnected������true���w�肷��ƃl�b�g���[�N�ڑ������݂��Ȃ��ꍇ�ɂ̂݃_�C�A���A�b�v����悤�ɂȂ�܂��B�Ⴆ�΁ALAN�ɐڑ����Ă���ꍇ�ɂ̓_�C�A���A�b�v���܂���B


===sequential�G�������g

 <sequential>
  <!-- entry -->
 </sequential>

sequential�G�������g���g�p����ƁA�q�G�������g�Ɏw�肵���G���g�������ԂɎ��s���܂��B


===parallel�G�������g

 <parallel>
  <!-- entry -->
 </parallel>

parallel�G�������g���g�p����ƁA�q�G�������g�Ɏw�肵���G���g���𓯎��Ɏ��s���܂��B�Ⴆ�΁A�����Ȑڑ����g�p���Ă���ꍇ�ɁAparallel�G�������g���g�p���ĕ����̃A�J�E���g�𓯎��ɓ������邱�Ƃ��o���܂��B


===entry�G�������g

 <entry
  account="�A�J�E���g��"
  subaccount="�T�u�A�J�E���g��"
  send="���M���邩�ǂ���"
  receive="��M���邩�ǂ���"
  folder="�t�H���_��"
  selectFolder="�t�H���_��I�����邩�ǂ���"
  filter="��M�t�B���^"/>

entry�G�������g�ŃG���g�����w�肵�܂��Baccount�����ŃA�J�E���g�����w�肵�܂��Bsubaccount�������w�肷��ƃT�u�A�J�E���g���w��ł��܂��B�w�肳��Ă��Ȃ��ꍇ�ɂ́A���݂̃T�u�A�J�E���g���g�p����܂��B

send������receive�������w�肷��ƁA���M�E��M�̂ǂ��炩�݂̂��s�����Ƃ��o���܂��B�ǂ�����w�肵�Ȃ��ꍇ�ɂ́A����M���s���܂��B

folder�����Ńt�H���_�����w�肷��ƁA���̃t�H���_�݂̂𓯊����܂��B//�ň͂ނ��Ƃɂ�萳�K�\�����g�p�ł��܂��B�܂��AselectFolder������true�ɂ���ƁA��������O�ɂǂ̃t�H���_�𓯊�����̂���q�˂�_�C�A���O���\������܂��B�w�肵�Ȃ��Ɠ����\�Ȃ��ׂẴt�H���_�𓯊����܂��B

filter�����Ŏg�p����((<�����t�B���^|URL:SyncFilter.html>))���w�肷�邱�Ƃ��o���܂��B�w�肵�Ȃ��ꍇ�ɂ́A�����t�B���^�͎g�p����܂���B


==�T���v��

 <?xml version="1.0" encoding="utf-8"?>
 <goround>
  <course name="all">
   <parallel>
    <entry account="test1"/>
    <entry account="test2" folder="Inbox" receive="true" filter="main"/>
   </parallel>
  </course>
  <course name="ml">
   <parallel>
    <entry account="test3" folder="/ml/.*/" receive="true"/>
   </parallel>
  </course>
 </goround>


==�X�L�[�}

 start = element goround {
   element course {
     element dialup {
       ## �_�C�A���A�b�v��
       attribute name {
         xsd:string
       }?,
       ## ���M��
       attribute dialFrom {
         xsd:string
       }?,
       ## �_�C�A���O��\�����邩�ǂ���
       attribute showDialog {
         xsd:boolean
       }?,
       ## �ؒf����܂ł̑҂�����
       attribute disconnectWait {
         xsd:int
       }?,
       ## �l�b�g���[�N���ڑ�����Ă��Ȃ��Ƃ��̂݃_�C�A���A�b�v���邩�ǂ���
       attribute wheneverNotConnected {
         xsd:boolean
       }
     }?,
     method,
     ## �R�[�X��
     attribute name {
       xsd:string
     },
     ## ���񂷂�O�Ɋm�F���邩�ǂ���
     attribute confirm {
       xsd:boolean
     }?
   }*
 }
 
 method = element sequential {
   entry*
 } |
 element parallel {
   entry*
 }
 
 entry = element entry {
   empty,
   ## �A�J�E���g��
   attribute account {
     xsd:string
   },
   ## �T�u�A�J�E���g��
   attribute subaccount {
     xsd:string
   }?,
   ## ���M���邩�ǂ���
   attribute send {
     xsd:boolean
   }?,
   ## ��M���邩�ǂ���
   attribute receive {
     xsd:boolean
   }?,
   (
     ## ��M����t�H���_
     attribute folder {
       xsd:string
     } |
     ## ��M����t�H���_��I�����邩�ǂ���
     attribute selectFolder {
       xsd:boolean
     }
   )?,
   ## �����t�B���^
   attribute filter {
     xsd:string
   }?
 }

=end

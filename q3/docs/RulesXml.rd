=begin
=rules.xml

((<�U�蕪��|URL:ApplyRules.html>))�̐ݒ������XML�t�@�C���ł��B���̃t�@�C���ɂ́A((<�U�蕪���̐ݒ�|URL:OptionRules.html>))�Őݒ肵����񂪕ۑ�����܂��B


==����

===rules�G�������g

 <rules>
   <!-- ruleSet -->
 </rules>

rules�G�������g�̓g�b�v���x���G�������g�ɂȂ�܂��Brules�G�������g�ȉ��ɂ́A0�ȏ��ruleSet�G�������g��u�����Ƃ��o���܂��B


===ruleSet�G�������g

 <ruleSet
  account="�A�J�E���g��"
  folder="�t�H���_��">
  <!-- rule -->
 </ruleSet>

ruleSet�G�������g�͐U�蕪�����Ɏg�p����郋�[���Z�b�g���w�肵�܂��Baccount�����ɃA�J�E���g�����Afolder�����Ƀt�H���_�����w�肷�邱�Ƃ��o���܂��B�����Ƃ�//�ň͂ނ��Ƃɂ�萳�K�\�����g�p�ł��܂��B�����̑����͏ȗ��\�ŁA�ȗ����ꂽ�ꍇ�ɂ͂��ꂼ��S�ẴA�J�E���g�E�t�H���_�Ƀ}�b�`���܂��B���Ƃ��΁Aaccount�����݂̂��w�肵��folder�������w�肵�Ȃ��ƁA�w�肵���A�J�E���g�̑S�Ẵt�H���_�Ƀ}�b�`���܂��B

�U�蕪�����ɂ́A�U�蕪���Ώۂ̃t�H���_���ƃA�J�E���g�����g�p���āA�ォ�珇�ԂɃ��[���Z�b�g���������A�ŏ��Ƀ}�b�`�������[���Z�b�g���g�p���܂��B


===rule�G�������g

 <rule
  match="�}�N��"
  use="manual|auto|active"
  continue="true|false"
  description="����",
  enabled="true|false">
  <!-- move/copy/delete/label/deleteCache/apply -->
 </rule>

rule�G�������g�́A���[�����̂��w�肵�܂��Bmatch�����ɂ̓}�N�����w�肵�܂��B���[���͏ォ�珇�Ԃɂɕ]�����ꂳ��܂��B���ʂ��ŏ��ɐ^�ɂȂ������[���̃A�N�V���������s���܂��B���s�����A�N�V�����͎q�G�������g�Ƃ��Ďw�肵�܂��B

use�����ɂ�manual, auto, active�̑g�ݍ��킹���w�肵�܂��Bmanual���w�肷��Ǝ蓮�U�蕪�����ɁAauto���w�肷��Ǝ����U�蕪�����ɁAactive���w�肷��ƃA�N�e�B�u�U�蕪�����Ɏg�p����܂��B�����w�肷��ꍇ�ɂ͋󔒕����ŋ�؂�܂��B�w�肵�Ȃ��ꍇ�ɂ�"manual auto"���w�肵���̂Ɠ����ɂȂ�܂��B

continue�����ɂ͂��̃��[���Ƀ}�b�`���ď���������Ō㑱�̃��[�����������邩�ǂ������w�肵�܂��B

description�����ɂ͐������w�肵�܂��B

enabled������false���w�肷��ƃ��[���������ɂȂ�܂��B


===move�G�������g

 <move
  account="�A�J�E���g��"
  folder="�t�H���_��">
  <!-- template -->
 </move>

move�G�������g�́A�ړ��R�}���h���w�肵�܂��B���̃A�N�V������account������folder�����Ŏw�肳�ꂽ�t�H���_�Ƀ��b�Z�[�W���ړ����܂��Baccount�𑮐��͏ȗ��\�ŁA���̏ꍇ�U�蕪���Ώۂ̃t�H���_�Ɠ����A�J�E���g�̃t�H���_���ړ���ɂȂ�܂��B

�q�G�������g�Ƃ���template�G�������g��u���ƁA�e���v���[�g�ŏ������Ă���ړ����邱�ƂɂȂ�܂��B


===copy�G�������g

 <copy
  account="�A�J�E���g��"
  folder="�t�H���_��">
  <!-- template -->
 </copy>

copy�G�������g�́A���b�Z�[�W���ړ������ɃR�s�[����_��������move�G�������g�Ɠ����ł��B


===template�G�������g

 <template
  name="�e���v���[�g��">
  <!-- argument -->
 </template>

template�G�������g��copy��move�̎q�G�������g�Ƃ��Ēu���ƁA�ړ��E�R�s�[���Ƀ��b�Z�[�W���e���v���[�g�ŏ������܂��Bname�����Ńe���v���[�g�����w�肵�܂��B�e���v���[�g�Ɉ�����n���Ƃ��ɂ͎q�G�������g�Ƃ��ĔC�ӂ̐���argument�G�������g�������܂��B


===argument�G�������g

 <argument
  name="������">
  �l
 </argument>

argument�G�������g��template�G�������g�Ŏw�肵���e���v���[�g�Ɉ�����n�����߂Ɏw�肵�܂��Bname�����Ŗ��O���A�e�L�X�g�Ƃ��Ēl���w�肵�܂��B�e���v���[�g�ɓn���������́A�e���v���[�g������}�N���̕ϐ��Ƃ��ăA�N�Z�X�ł��܂��B


===delete�G�������g

 <delete
  direct="�S�~�����g�p���邩�ǂ���"/>

delete�G�������g�́A�폜�R�}���h���w�肵�܂��B���̃A�N�V�����͑Ώۂ̃��b�Z�[�W���폜���܂��Bdirect������true���w�肷��ƃS�~�����g�p�����ɒ��ڃ��b�Z�[�W���폜���܂��B����ȊO�̏ꍇ�ɂ̓S�~���Ƀ��b�Z�[�W���ړ����܂��B


===label�G�������g

 <label
  type="add|remove">
  ���x��
 </label>

label�G�������g�́A���x���ݒ�R�}���h���w�肵�܂��Btype�������w�肳��Ȃ������ꍇ�ɂ͎w�肳�ꂽ���x����ݒ肵�܂��Btype������add���w�肷��Ǝw�肳�ꂽ���x����ǉ����Aremove���w�肷��ƍ폜���܂��B


===deleteCache�G�������g

 <deleteCache/>

deleteCache�G�������g�́A�L���b�V���폜�R�}���h���w�肵�܂��B���̃A�N�V�����̓��b�Z�[�W�̃L���b�V���iIMAP4��NNTP�A�J�E���g�ŃT�[�o����擾�������b�Z�[�W�j���폜���܂��B


===apply�G�������g

 <apply>
  ���s����}�N��
 </apply>

apply�G�������g�̓}�N���̎��s���w�肵�܂��B�Ώۂ̃��b�Z�[�W�ɑ΂��Ďw�肳�ꂽ�}�N�������s���܂��B


==�T���v��

 <?xml version="1.0" encoding="utf-8"?>
 <rules>
  <ruleSet account="test" folder="Inbox">
   <rule match="@BeginWith(%Subject, '[Qs:')">
    <move folder="ml/qs"/>
   </rule>
   <rule match="@Contain(From, '@example.com')">
    <move folder="test"/>
   </rule>
  </ruleSet>
  <ruleSet account="test">
   <rule match="@True()">
    <copy folder="archives">
     <template name="strip_header">
      <argument name="foo">bar</argument>
     </template>
    </copy>
   </rule>
  </ruleSet>
 </rules>


==�X�L�[�}

 start = element rules {
   element ruleSet {
     element rule {
       ## �A�N�V����
       action?,
       ## ���[�����K�p���������i�}�N���j
       attribute match {
         xsd:string
       },
       # ���[�����K�p�������
       # manual�͎蓮�U�蕪��
       # auto�͎����U�蕪��
       # active�̓A�N�e�B�u�U�蕪��
       # �w�肳��Ȃ��ꍇ�ɂ͎蓮�Ǝ���
       attribute use {
         xsd:string
       }?
       ## ���̃��[���ɐi�ނ��ǂ���
       ## �w�肳��Ȃ��ꍇ�ɂ�false
       attribute continue {
         xsd:boolean
       }?,
       ## ����
       attribute description {
         xsd:string
       }?,
       ## �L�����ǂ���
       ## �w�肳��Ȃ��ꍇ�ɂ�true
       attribute enabled {
         xsd:boolean
       }?
     }*,
     ## ���[�����K�p�����A�J�E���g
     ## �w�肳��Ȃ��ꍇ�A�S�ẴA�J�E���g
     attribute account {
       xsd:string
     }?,
     ## ���[�����K�p�����t�H���_
     ## �w�肳��Ȃ��ꍇ�A�S�Ẵt�H���_
     attribute folder {
       xsd:string
     }?
   }*
 }
 
 action = element move {
   template?,
   ## �ړ���A�J�E���g
   attribute account {
     xsd:string
   }?,
   ## �ړ���t�H���_
   attribute folder {
     xsd:string
   }
 } |
 element copy {
   template?,
   ## �R�s�[��A�J�E���g
   attribute account {
     xsd:string
   }?,
   ## �R�s�[��t�H���_
   attribute folder {
     xsd:string
   }
 } |
 element delete {
   attribute direct {
     xsd:boolean
   }?
 } |
 element label {
   ## �^�C�v
   ## �w�肵�Ȃ��ꍇ�ɂ͐ݒ�
   ## add�͒ǉ�
   ## remove�͍폜
   attribute type {
     "add|remove"
   }?,
   ## ���x��
   xsd:string
 } |
 element deleteCache {
   empty
 } |
 element apply {
   ## ���s����}�N��
   xsd:string
 }
 
 template = element template {
   ## ����
   element argument {
     ## �����̒l
     text,
     ## �����̖��O
     attribute name {
       xsd:string
     }
   }*,
   ## �e���v���[�g�̖��O
   attribute name {
     xsd:string
   }
 }

=end

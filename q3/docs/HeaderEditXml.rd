=begin
=headeredit.xml

�w�b�_�G�f�B�b�g�r���[�̐ݒ������XML�t�@�C���ł��B


==����

===headerEdit�G�������g

 <headerEdit>
  <!-- line -->
 </headerEdit>

headerEdit�G�������g���g�b�v���x���G�������g�ɂȂ�܂��BheaderEdit�G�������g�ȉ��ɂ�0�ȏ��line�G�������g��u�����Ƃ��ł��܂��B


===line�G�������g

 <line
  hideIfNoFocus="true|false"
  class="�N���X��">
  <!-- static, edit, address, attachment, account, signature -->
 </line>

line�G�������g�̓w�b�_�G�f�B�b�g�r���[�̈�s��\���܂��B

hideIfNoFocus�����́A�w�b�_�G�f�B�b�g�r���[����t�H�[�J�X���Ȃ��Ȃ����Ƃ��i�Ⴆ�΃G�f�B�b�g�r���[�Ƀt�H�[�J�X���ړ������Ƃ��j�ɂ��̍s���B�����ǂ������w�肵�܂��B�w�肵�Ȃ��ꍇ�ɂ�false���w�肵���̂Ɠ����ɂȂ�܂��B

class�����ɂ͐��K�\�����w�肵�܂��B�w�肵�����K�\���ɃA�J�E���g�N���X���}�b�`����ꍇ�̂ݍs���\������܂��B�Ⴆ�΁A"mail|news"�Ǝw�肷���mail�A�J�E���g��news�A�J�E���g�ł̂ݕ\�������悤�ɂȂ�܂��B�w�肵�Ȃ��ꍇ�ɂ́A�A�J�E���g�N���X�ɂ�����炸��ɕ\������܂��B


===static�G�������g

 <static
  width="��"
  number="�ԍ�"
  initialFocus="true|false"
  style="�t�H���g�X�^�C��"
  align="left|center|right">
  ������
 </static>

static�G�������g�̓X�^�e�B�b�N�R���g���[����\���܂��B�R���e���c�ɕ\�����镶������w�肵�܂��B

width�����ɂ͕����w�肵�܂��B���̎w��ɂ��ẮA((<header.xml|URL:HeaderXml.html>))�̔��l���Q�Ƃ��Ă��������B

number�����ɂ̓R���g���[���̔ԍ����w�肵�܂��B���̔ԍ���((<ViewFocusEditItem�A�N�V����|URL:ViewFocusEditItemAction.html>))�̈����Ɏw�肷�邱�ƂŁA�t�H�[�J�X���ړ����邱�Ƃ��ł��܂��B

initialFocus�����ɂ̓G�f�B�b�g�E�B���h�E���J�����Ƃ��Ƀt�H�[�J�X���󂯎�邩�ǂ������w�肵�܂��B�w�肵�Ȃ��ꍇ�ɂ́Atrue���w�肵���̂Ɠ����ɂȂ�܂��B

style�����ɂ̓t�H���g�̃X�^�C�����w�肵�܂��B�w��ł���̂�bold��italic�̑g�ݍ��킹�ł��B�����w�肷��ꍇ�ɂ�,�ŋ�؂�܂��B�w�肵�Ȃ��ꍇ�ɂ͒ʏ�̃X�^�C���ɂȂ�܂��B

align�����ɂ�left, center, right�̂����ꂩ���w�肵�܂��B���ꂼ��A���񂹁A�����񂹁A�E�񂹂ɂȂ�܂��B�w�肵�Ȃ��ꍇ�ɂ͍��񂹂ɂȂ�܂��B


===edit�G�������g

 <edit
  width="��"
  number="�ԍ�"
  initialFocus="true|false"
  style="�t�H���g�X�^�C��"
  align="left|center|right"
  field="�w�b�_��"
  type="addressList|references|unstructured"/>

edit�G�������g�̓G�f�B�b�g�R���g���[����\���܂��B

width, number, initialFocus, style, align�����ɂ��Ă�static�G�������g���Q�Ƃ��Ă��������B

field�����ɂ͂��̃G�f�B�b�g�R���g���[���ŕҏW����w�b�_�����w�肵�܂��B�܂��Atype�����ɂ͂��̃w�b�_�̌^���w�肵�܂��B�w��ł���̂́AaddressList, references, unstructured�̂����ꂩ�ł��B


===address�G�������g

 <address
  width="��"
  number="�ԍ�"
  initialFocus="true|false"
  style="�t�H���g�X�^�C��"
  align="left|center|right"
  field="�w�b�_��"
  expandAlias="true|false"
  autoComplete="true|false"/>

address�G�������g�̓A�h���X�p�̃G�f�B�b�g�R���g���[����\���܂��B

width, number, initialFocus, style, align�����ɂ��Ă�static�G�������g���Q�Ƃ��Ă��������Bfield�����ɂ��Ă�edit�G�������g���Q�Ƃ��Ă��������B

expandAlias�����ɂ́A((<�A�h���X��|URL:AddressBook.html>))�̕ʖ���W�J���邩�ǂ������w�肵�܂��Btrue���w�肷��ƃt�H�[�J�X���������Ƃ��ɕʖ���W�J���܂��B�w�肵�Ȃ��ꍇ�ɂ�true���w�肵���̂Ɠ����ɂȂ�܂��B

autoComplete�����ɂ́A((<�A�h���X�̎����⊮|URL:AddressAutoComplete.html>))��L���ɂ��邩�ǂ������w�肵�܂��B�w�肵�Ȃ��ꍇ�ɂ�true���w�肵���̂Ɠ����ɂȂ�܂��B


===attachment�G�������g

 <attachment
  width="��"
  number="�ԍ�"
  initialFocus="true|false"/>

attachment�G�������g�͓Y�t�t�@�C���ҏW�R���g���[����\���܂��B

width, number, initialFocus�����ɂ��Ă�static�G�������g���Q�Ƃ��Ă��������B


===account�G�������g

 <account
  width="��"
  number="�ԍ�"
  initialFocus="true|false"
  showFrom="true|false"/>

account�G�������g��((<�A�J�E���g|URL:Account.html>))�I���R���g���[����\���܂��B�I���\�ȃA�J�E���g�A�T�u�A�J�E���g���R���{�{�b�N�X�ŕ\������܂��B

width, number, initialFocus�����ɂ��Ă�static�G�������g���Q�Ƃ��Ă��������B

showFrom�����ɂ̓A�J�E���g���ׂ̗Ɏg�p�����From�A�h���X��\�����邩�ǂ������w�肵�܂��B�w�肵�Ȃ��ꍇ�ɂ�true���w�肵���̂Ɠ����ɂȂ�܂��B


===signature�G�������g

 <signature
  width="��"
  number="�ԍ�"
  initialFocus="true|false"/>

signature�G�������g��((<����|URL:Signature.html>))�I���R���g���[����\���܂��B�I���\�ȏ������R���{�{�b�N�X�ŕ\������܂��B

width, number, initialFocus�����ɂ��Ă�static�G�������g���Q�Ƃ��Ă��������B


==�T���v��

 <?xml version="1.0" encoding="utf-8"?>
 <headerEdit>
  <line class="mail">
   <static width="1max" style="bold" align="right">T&amp;o:</static>
   <address field="To" number="0"/>
  </line>
  <line class="mail" hideIfNoFocus="true">
   <static width="1max" style="bold" align="right">&amp;Cc:</static>
   <address field="Cc" number="1" initialFocus="false"/>
  </line>
  <line class="mail" hideIfNoFocus="true">
   <static width="1max" style="bold" align="right">&amp;Bcc:</static>
   <address field="Bcc" number="2" initialFocus="false"/>
  </line>
  <line class="news">
   <static width="1max" style="bold" align="right">&amp;Newsgroups:</static>
   <edit type="unstructured" field="Newsgroups" number="3"/>
  </line>
  <line class="news" hideIfNoFocus="true">
   <static width="1max" style="bold" align="right">Fo&amp;llowup-To:</static>
   <edit type="unstructured" field="Followup-To" number="4" initialFocus="false"/>
  </line>
  <line hideIfNoFocus="true">
   <static width="1max" style="bold" align="right">&amp;Subject:</static>
   <edit type="unstructured" field="Subject" number="5"/>
  </line>
  <line hideIfNoFocus="true">
   <static width="1max" style="bold" align="right">Attac&amp;hment:</static>
   <attachment number="6"/>
  </line>
  <line hideIfNoFocus="true">
   <static width="1max" style="bold" align="right">Acco&amp;unt:</static>
   <account number="7"/>
   <static width="auto" style="bold" align="right">S&amp;ignature:</static>
   <signature width="5em" number="8"/>
  </line>
 </headerEdit>


==�X�L�[�}

 start = element headerEdit {
   element line {
     (
       element static {
         textitem
       } |
       element edit {
         textitem
       } |
       element address {
         textitem,
         ## �G�C���A�X��W�J���邩�ǂ���
         ## �w�肳��Ȃ��ꍇ�Atrue
         attribute expandAlias {
           xsd:boolean
         }?,
         ## �I�[�g�R���v���[�g���g�p���邩�ǂ���
         ## �w�肳��Ȃ��ꍇ�Atrue
         attribute autoComplete {
           xsd:boolean
         }?
       } |
       element attachment {
         item
       } |
       element account {
         item,
         ## From�̃A�h���X��\�����邩�ǂ���
         ## �w�肳��Ȃ��ꍇ�Atrue
         attribute showFrom {
           xsd:boolean
         }?
       } |
       element signature {
         item
       }
     )*,
     ## �t�H�[�J�X���Ȃ��ꍇ�ɉB�����ǂ���
     ## �w�肳��Ȃ��ꍇ�Afalse
     attribute hideIfNoFocus {
       xsd:boolean
     }?,
     ## �ǂ̃A�J�E���g�N���X�ŕ\�����邩
     ## �w�肳��Ȃ��ꍇ�ɂ͂��ׂẴN���X�ŕ\��
     attribute class {
       xsd:string
     }?
   }*
 }
 
 item = attribute width {
   xsd:string {
     pattern = "auto|[0-9]max|[0-9]min|[0-9]+(px)?|[0-9]+%|[0-9]+(\.[0-9]+)?em"
   }
 }?,
 attribute number {
   xsd:int
 }?,
 attribute initialFocus {
   xsd:boolean
 }?
 
 textitem = item,
 ## �X�^�C��
 ## bold��italic���w��\
 ## �����w�肷��ꍇ�ɂ�,�ŋ�؂�
 attribute style {
   xsd:string
 }?,
 attribute align {
   "left" | "center" | "right"
 }?,
 (
   xsd:string |
   (
     ## �w�b�_��
     attribute field {
       xsd:string
     },
     ## �w�b�_�̃^�C�v
     attribute type {
       "addressList" | "references" | "unstructured"
     }?
   )
 )

=end

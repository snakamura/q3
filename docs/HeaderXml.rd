=begin
=header.xml

�w�b�_�r���[�̐ݒ������XML�t�@�C���ł��B


==����

===header�G�������g

 <header>
  <!-- line -->
 </header>

header�G�������g���g�b�v���x���G�������g�ɂȂ�܂��Bheader�G�������g�ȉ��ɂ�0�ȏ��line�G�������g��u�����Ƃ��ł��܂��B


===line�G�������g

 <line
  hideIfEmpty="�A�C�e����"
  class="�N���X��">
  <!-- static, edit, attachment -->
 </line>

line�G�������g�̓w�b�_�r���[�̈�s��\���܂��B

hideIfEmpty�����ɂ̓A�C�e�������w�肵�܂��B�w�肵���A�C�e������̏ꍇ�ɂ͍s���ƉB����܂��B�w�肵�Ȃ��ꍇ�ɂ́A���̍s�͏�ɕ\������܂��B

class�����ɂ͐��K�\�����w�肵�܂��B�w�肵�����K�\���ɃA�J�E���g�N���X���}�b�`����ꍇ�̂ݍs���\������܂��B�Ⴆ�΁A"mail|news"�Ǝw�肷���mail�A�J�E���g��news�A�J�E���g�ł̂ݕ\�������悤�ɂȂ�܂��B�w�肵�Ȃ��ꍇ�ɂ́A�A�J�E���g�N���X�ɂ�����炸��ɕ\������܂��B


===static�G�������g

 <static
  name="�A�C�e����"
  width="��"
  number="�ԍ�"
  showAlways="true|false"
  background="�w�i�F"
  style="�t�H���g�X�^�C��"
  align="left|center|right">
  �e���v���[�g
 </static>

static�G�������g�̓X�^�e�B�b�N�R���g���[����\���܂��B�R���e���c�Ƀe���v���[�g�����ŕ\�����镶������w�肵�܂��B

name�����ɂ̓A�C�e�������w�肵�܂��Bline�G�������g��hideIfEmpty�����Ɏw�肷��ꍇ�ɂ͂����Ŗ��O���w�肵�Ă����܂��B

width�����ɂ͕����w�肵�܂��B���̎w��ɂ��Ă͔��l���Q�Ƃ��Ă��������B

number�����ɂ̓R���g���[���̔ԍ����w�肵�܂��B���̔ԍ���((<ViewFocusItem�A�N�V����|URL:ViewFocusItemAction.html>))�̈����Ɏw�肷�邱�ƂŁA�t�H�[�J�X���ړ����邱�Ƃ��ł��܂��B

showAlways�����ɂ�true�܂���false���w�肵�܂��Btrue���w�肷��ƃR���e�L�X�g�A�J�E���g���Ȃ��ꍇ�ł���ɃA�C�e���ɕ������\�����܂��B���̏ꍇ�A�e���v���[�g���Ƀ}�N�����������Ƃ͂ł��܂���B�w�肵�Ȃ��ꍇ�ɂ�false���w�肵���ꍇ�Ɠ����ɂȂ�܂��B

background�����ɂ͔w�i�F���e���v���[�g�����Ŏw�肵�܂��B�e���v���[�g��]���������ʂ́Arrggbb�`���̕�����ɂȂ�K�v������܂��B�󕶎����Ԃ��ƃw�b�_�r���[�̔w�i�F�Ɠ����ɂȂ�܂��B�w�肵�Ȃ��ꍇ�ɂ̓w�b�_�r���[�̔w�i�F�Ɠ����ɂȂ�܂��B

style�����ɂ̓t�H���g�̃X�^�C�����w�肵�܂��B�w��ł���̂�bold��italic�̑g�ݍ��킹�ł��B�����w�肷��ꍇ�ɂ�,�ŋ�؂�܂��B�w�肵�Ȃ��ꍇ�ɂ͒ʏ�̃X�^�C���ɂȂ�܂��B

align�����ɂ�left, center, right�̂����ꂩ���w�肵�܂��B���ꂼ��A���񂹁A�����񂹁A�E�񂹂ɂȂ�܂��B�w�肵�Ȃ��ꍇ�ɂ͍��񂹂ɂȂ�܂��B


===edit�G�������g

 <edit
  name="�A�C�e����"
  width="��"
  number="�ԍ�"
  showAlways="true|false"
  background="�w�i�F"
  style="�X�^�C��"
  align="left|center|right"
  multiline="�s��"
  wrap="true|false">
  �e���v���[�g
 </edit>

edit�G�������g�̓G�f�B�b�g�R���g���[����\���܂��B�������ҏW�ł���킯�ł͂Ȃ��A�X�^�e�B�b�N�R���g���[���Ƃ̎�ȈႢ�́A�t�H�[�J�X�����Ă邩�ǂ����Ǝ����X�N���[�����邩�ǂ����ł��B�R���e���c�Ƀe���v���[�g�����ŕ\�����镶������w�肵�܂��B

name, width, number, showAlways, background, style, align�����ɂ��Ă�static�G�������g���Q�Ƃ��Ă��������B

multiline�����ɂ͕����s�ɂȂ����Ƃ��ɍő剽�s�܂ő傫�����邩���w�肵�܂��B-1���w�肷��ƕ����s�ɂ͂Ȃ�܂���B�܂��A0���w�肷��ƕK�v�Ȃ����s���������܂��B�w�肵�Ȃ��ꍇ�ɂ͕����s�ɂȂ�܂���B

wrap�����ɂ̓R���g���[���̕��Ŏ����Ő܂�Ԃ����ǂ������w�肵�܂��B�w�肵�Ȃ��ꍇ�ɂ͐܂�Ԃ��܂���B


===attachment�G�������g

 <attachment
  name="�A�C�e����"
  width="��"
  number="�ԍ�"
  showAlways="true|false"
  background="�w�i�F"/>

attachment�G�������g�͓Y�t�t�@�C���R���g���[����\���܂��B���b�Z�[�W�̓Y�t�t�@�C����\�����܂��B

name, width, number, showAlways, background�����ɂ��Ă�static�G�������g���Q�Ƃ��Ă��������B


==�T���v��

 <?xml version="1.0" encoding="utf-8"?>
 <header>
  <line class="mail">
   <static width="auto" style="bold" showAlways="true">To:</static>
   <edit>{@FormatAddress(To, 3)}</edit>
  </line>
  <line class="mail" hideIfEmpty="cc">
   <static width="auto" style="bold" showAlways="true">Cc:</static>
   <edit name="cc">{@FormatAddress(Cc, 3)}</edit>
  </line>
  <line class="news">
   <static width="auto" style="bold" showAlways="true">Newsgroups:</static>
   <edit>{Newsgroups}</edit>
  </line>
  <line class="news" hideIfEmpty="followup-to">
   <static width="auto" style="bold" showAlways="true">Followup-To:</static>
   <edit name="followup-to">{FollowUp-To}</edit>
  </line>
  <line class="mail|news">
   <static width="auto" style="bold" showAlways="true">From:</static>
   <edit background="{@If(@Not(@Param('Verify')), '', @Contain(@Param('Verify'), 'AddressMatch'), 'f5f6be', @Contain(@Param('Verify'), 'AddressMismatch'), 'ec7b95', '')}">{@FormatAddress(From, 3)}</edit>
   <static width="auto" style="bold" align="right" showAlways="true">Date:</static>
   <edit width="10em">{@FormatDate(@Date(Date), @Profile('', 'Global', 'DefaultTimeFormat'))}</edit>
  </line>
  <line class="mail|news">
   <static width="auto" style="bold" showAlways="true">Subject:</static>
   <edit>{@Subject()}</edit>
  </line>
  <line hideIfEmpty="attachment" class="mail|news">
   <static width="auto" style="bold" showAlways="true">Attachment:</static>
   <attachment name="attachment" background="{@If(@Equal(X-QMAIL-AttachmentDeleted, 1), 'ccc7ba', '')}"/>
  </line>
  <line class="rss">
   <static width="auto" style="bold" showAlways="true">Title:</static>
   <edit>{@Subject()}</edit>
  </line>
  <line class="rss">
   <static width="auto" style="bold" showAlways="true">Date:</static>
   <edit>{@FormatDate(@Date(Date), @Profile('', 'Global', 'DefaultTimeFormat'))}</edit>
  </line>
  <line class="rss" hideIfEmpty="creator">
   <static width="auto" style="bold" showAlways="true">Creator:</static>
   <edit name="creator">{X-RSS-Creator}</edit>
  </line>
  <line class="rss" hideIfEmpty="category">
   <static width="auto" style="bold" showAlways="true">Category:</static>
   <edit name="category">{X-RSS-Category}</edit>
  </line>
  <line class="rss" hideIfEmpty="subject">
   <static width="auto" style="bold" showAlways="true">Subject:</static>
   <edit name="subject">{X-RSS-Subject}</edit>
  </line>
  <line class="rss">
   <static width="auto" style="bold" showAlways="true">URL:</static>
   <edit>{X-RSS-Link}</edit>
  </line>
  <line hideIfEmpty="label">
   <static width="auto" style="bold" showAlways="true">Label:</static>
   <edit name="label">{@Label()}</edit>
  </line>
  <line class="mail|news" hideIfEmpty="sign">
   <static width="auto" style="bold" showAlways="true">Signed by:</static>
   <edit name="sign" background="{@If(@Not(@Param('Verify')), '', @Contain(@Param('Verify'), 'Verified'), 'f5f6be', @Contain(@Param('Verify'), 'VerifyFailed'), 'ec7b95', '')}">{@Param('SignedBy')}</edit>
  </line>
 </header>


==�X�L�[�}

 start = element header {
   element line {
     (
       element static {
         textitem
       } |
       element edit {
         textitem,
         attribute multiline {
           xsd:int
         }?,
         attribute wrap {
           xsd:boolean
         }?
       } |
       element attachment {
         item
       }
     )*,
     attribute hideIfEmpty {
       xsd:string
     }?,
     attribute class {
       xsd:string
     }?
   }*
 }
 
 item = attribute name {
   xsd:string
 }?,
 attribute width {
   xsd:string {
     pattern = "auto|[0-9]max|[0-9]min|[0-9]+(px)?|[0-9]+%|[0-9]+(\.[0-9]+)?em"
   }
 }?,
 attribute showAlways {
   xsd:boolean
 }?,
 attribute background {
   xsd:string
 }?
 
 textitem = xsd:string,
 item,
 attribute style {
   xsd:string
 }?,
 attribute align {
   "left" | "center" | "right"
 }?


==���l

width�����ł̕��̎w��ł͈ȉ��̂悤�Ȏw�肪�ł��܂��B

:�w��Ȃ�
  �����w�肵�܂���B
:auto
  �K�v�ɉ����ĕ������߂܂��B
:max
  1max����9max�܂Ŏw��ł��A�����ԍ�������ꂽ�R���g���[���ōő�̃R���g���[���̕��ɍ��킳��܂��B�R���g���[���̕���auto���w�肵���̂Ɠ��l�Ɍ��߂��܂��B
:min
  1min����9min�܂Ŏw��ł��A�����ԍ�������ꂽ�R���g���[���ōŏ��̃R���g���[���̕��ɍ��킳��܂��B�R���g���[���̕���auto���w�肵���̂Ɠ��l�Ɍ��߂��܂��B
:px
  �s�N�Z���P�ʂŎw�肵�܂��B�����̂ݎw��ł��܂��B
:%
  �p�[�Z���g�Ŏw�肵�܂��B�����̂ݎw��ł��܂��B
:em
  �t�H���g�̍�������Ɏw�肵�܂��B�������w��ł��܂��B

max��min�͕ʂ̍s�ɂ���A�C�e���̕��𑵂������ꍇ�Ɏg�p���܂��B�Ⴆ�΁A�e�s�̍����Ƀ��x����u���ă��x���̕������킹�����ꍇ�ɁA����炷�ׂẴ��x����width������1max�Ǝw�肵�Ă����ƁA�ł����̍L�����x���̕��ɂ��ׂẴ��x���̕������킳��܂��B

�s���̔z�u�����߂�Ƃ��ɂ͈ȉ��̂悤�Ɍ��߂܂��B

(1)auto, max, min���w�肳�ꂽ�R���g���[���̕����v�Z���A�e�R���g���[�����K�v�Ƃ��镝�����߂�
(2)px, em���w�肳�ꂽ�R���g���[���̕������߂�
(3)(1), (2)�Ō��܂����R���g���[���̕������ׂđ���
(4)(3)�Ōv�Z���������w�b�_�r���[�̕����������ꍇ�ɂ́A�c��̕���%�Ŏw�肳�ꂽ�R���g���[���Ɋ���U��
(5)%�Ŏw�肳�ꂽ�R���g���[���̕��̍��v��100%�𒴂��Ă��Ȃ��ꍇ�ɂ́A�c��̕��𕝂��w�肵�Ȃ������R���g���[���ŋϓ��Ɋ���
(6)%�Ŏw�肳�ꂽ�R���g���[���̕��̍��v��100%�𒴂��Ă����ꍇ�ɂ́A�����w�肵�Ȃ������R���g���[���̕���0�ɂ���
(7)(3)�Ōv�Z���������w�b�_�r���[�̕������L���ꍇ�ɂ́A%�Ŏw�肳�ꂽ�R���g���[���ƕ����w�肵�Ȃ������R���g���[���̕���0�ɂ���

=end

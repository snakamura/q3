=begin
=signatures.xml

((<����|URL:Signature.html>))�̐ݒ������XML�t�@�C���ł��B���̃t�@�C���ɂ́A((<�����̐ݒ�|URL:OptionSignatures.html>))�Őݒ肵����񂪕ۑ�����܂��B


==����

===signatures�G�������g

 <signatures>
  <!-- signature -->
 </signatures>

signatures�G�������g���g�b�v���x���G�������g�ɂȂ�܂��B0�ȏ��signature�G�������g���q�G�������g�Ƃ��Ă������Ƃ��o���܂��B


===signature�G�������g

 <signature
  account="�A�J�E���g��"
  name="���O"
  default="�f�t�H���g�ɂ��邩�ǂ���">
  <!-- ���� -->
 </signature>

signature�G�������g�Ŏ��ۂ̏������w�肵�܂��Baccount�����ɃA�J�E���g�����w�肷��ƁA���̃A�J�E���g�݂̂Ŏg�p�����悤�ɂȂ�܂��B�w�肵�Ȃ��ꍇ�ɂ͑S�ẴA�J�E���g�Ŏg�p����܂��B//�ň͂ނ��Ƃɂ�萳�K�\�����g�p�ł��܂��B

name�����ɂ͖��O���w�肵�܂��B�����Ŏw�肵�����O���G�f�B�b�g�r���[�Ŏg�p����܂��Bdefault������true���w�肷��ƁA�f�t�H���g�ɂȂ邱�Ƃ��o���܂��B�f�t�H���g�ɂȂ�ƃG�f�B�b�g�r���[�ŊJ�����Ƃ��ɍŏ��ɑI������܂��B�����̃f�t�H���g������ꍇ�ɂ́A�ŏ��Ɍ����������̂��I������܂��B


==�T���v��

 <?xml version="1.0" encoding="utf-8"?>
 <signatures>
  <signature account="test1" name="Default"
   default="true">���̏����ł�</signature>
  <signature account="/test.*/" name="�e�X�g"><![CDATA[
 -- 
 Hogehoge Fugafuga <hoge@fuga.com>
 ]]></signature>
 </signatures>



==�X�L�[�}

 element signatures {
   element signature {
     ## �V�O�j�`��
     xsd:string,
     ## ���O
     attribute name {
       xsd:string
     },
     ## ���̃V�O�j�`�����g�p����A�J�E���g
     ## �w�肳��Ȃ��ꍇ�A�S�ẴA�J�E���g
     attribute account {
       xsd:string
     }?,
     ## �f�t�H���g�ɂȂ邩�ǂ���
     ## �w�肳��Ȃ��ꍇ�Afalse
     attribute default {
       xsd:boolean
     }?
   }*
 }

=end

=begin
=�X�y���`�F�b�N�͂ł��܂���?

QMAIL3�ɂ̓X�y���`�F�b�N�@�\�͂���܂��񂪁A((<aspell|URL:http://aspell.net/>))�Ȃǂ̃X�y���`�F�b�J���Ăяo�����ƂŃX�y���`�F�b�N���s�����Ƃ��ł��܂��B�����ł́Aaspell���g�����@��������܂��B

�܂��Aaspell�ƕK�v�Ȏ������C���X�g�[�����A�P�̂Ő��������삷�邱�Ƃ��m�F���܂��B

���ɁA�ȉ��̂悤�ȃX�N���v�g��p�ӂ��A���[���{�b�N�X�f�B���N�g����scripts�f�B���N�g���̉���"Spell Check.js"�Ƃ����t�@�C�����ŕۑ����܂��B

 var aspell = "\"C:\\Program Files\\aspell\\bin\\aspell.exe\"";
 var tempFile = "C:\\Documents and Settings\\username\\Application Data\\QMAIL3\\temp\\spellcheck.txt";
 
 // �e�L�X�g�����ׂđI�����ăN���b�v�{�[�h�ɂ���A
 // �N���b�v�{�[�h�o�R�Ńe�L�X�g���擾����
 editFrameWindow.invokeAction("EditSelectAll");
 editFrameWindow.invokeAction("EditCopy");
 var getClipboardMacro = macroParser.parse("@Clipboard()");
 var inText = getClipboardMacro.evaluate(application.nothing, document.accounts(0));
 
 // �擾�����e�L�X�g���ꎞ�t�@�C���ɏ�������
 var fs = new ActiveXObject("Scripting.FileSystemObject");
 try {
   var inFile = fs.OpenTextFile(tempFile, 2, true);
   try {
     inFile.Write(inText);
   }
   finally {
     inFile.Close();
   }
 
   // aspell���N�����ďI������܂ő҂�
   var shell = new ActiveXObject("WScript.Shell");
   shell.Run(aspell + " -c \"" + tempFile + "\"", 5, true);
 
   // aspell���C�������t�@�C����ǂݍ���
   var outText = null;
   var outFile = fs.OpenTextFile(tempFile, 1);
   try {
     outText = outFile.ReadAll();
   }
   finally {
     outFile.Close();
   }
 
   // �N���b�v�{�[�h���o�R���ăG�f�B�b�g�r���[�ɓ\��t��
   var setClipboardMacro = macroParser.parse("@Clipboard($value)");
   setClipboardMacro.setVariable("value", outText);
   setClipboardMacro.evaluate(application.nothing, document.accounts(0));
   editFrameWindow.invokeAction("EditPaste");
 }
 finally {
   fs.DeleteFile(tempFile);
 }

aspell��tempFile�Ŏw�肵�Ă���p�X�͓K�X���������Ă��������B

���ۂɎg���Ƃ��ɂ́A�G�f�B�b�g�r���[�̃��j���[����[�X�N���v�g]-[Spell Check]��I�����܂��B����ƁAaspell���N�����ăX�y���`�F�b�N���s���A�I������Ƃ��̌��ʂ��G�f�B�b�g�r���[�Ɏ�荞�܂�܂��B

((<�L�[�{�[�h�V���[�g�J�b�g�̃J�X�^�}�C�Y|URL:CustomizeAccelerators.html>))���s�����ƂŁA�V���[�g�J�b�g�L�[�����蓖�Ă邱�Ƃ��ł��܂��B


=end

=begin
=���X�g�r���[�Ƀ��b�Z�[�W�̈�s�ڂ�\�����邱�Ƃ͂ł��܂���?

ML�̃A�[�J�C�u�ȂǂŃ��X�g�\���E�X���b�h�\��������Ƃ��ɁASubject�̑ւ��ɖ{���̈�s�ڂ�\������ꍇ������܂��BML�Ȃǂ̏ꍇ�ASubject�́uRe: ...�v�̂悤�Ȋ����ŏ��ʂ����Ȃ����Ƃ������̂ŁA����͂Ȃ��Ȃ��֗��ł��B���������AQMAIL3�̃��X�g�r���[�ł���ɂ͈ȉ��̂悤�ɂ��܂��B

���j�Ƃ��ẮA�r���[�̃J�X�^�}�C�Y�ň�s�ڂ����o���}�N�����w�肷��Ƃ������j�ł��B��s�ڂ����o���Ƃ����Ă��A���p�∥�A�Ȃǂ͔�΂������̂ŁA������}�N���ł��̂͂ł��Ȃ��͂���܂��񂪁A������ƍ��ł��B�����ŁA@Script���g���Ĉ�s�ڂ̎��o����JScript�ŏ������Ƃɂ��܂����B�ł����̂͂���Ȋ����ł��B

 @If(@Contain(%Subject, 'Re:'), @Script('
 
 function getDescription(text) {
   var maxline = 20; // �w�肵���s���ȓ��Ɍ�����Ȃ������炠����߂�
   var minchar = 5;  // ������Z���s�͖���
   var regexes = [   // �����̐��K�\���Ƀ}�b�`�����疳��
     /^\\s+$/,                            // �󔒂����̍s
     /^[>|#���b��]/,                      // ���p�����ۂ���������n�܂��Ă���
     /^In (message|article)\\s/i,         // ���p�̐������ۂ�
     /[+-]\\d{4},?\\s*$/,                 // �^�C���]�[���ŏI����Ă���̂����p�̐������ۂ�
     /(writes|wrote|����)[:�F]\\s*$/i,    // ��������p�̐������ۂ�
     /����ɂ���|����ɂ���|����΂��/,  // ���A
     /(�\���܂�|�ł�)�B?$/,               // ���O���q�ׂĂ�����
     /���b/,                              // �u�����b�ɂȂ�܂��v�Ƃ�
     /(.)\\1{4}/,                         // ����������5�����ȏ㑱���̂͋�؂���ۂ�
     /^[\\w-]+:/                          // �w�b�_�̈��p���ۂ�
   ];
   var lines = text.split(/\\n/, maxline);
   for (n in lines) {
     var line = lines[n];
     if (line.length < minchar)
       continue;
     var match = false;
     for (r in regexes) {
       var regex = regexes[r];
       match = line.search(regex) != -1;
       if (match)
         break;
     }
     if (!match)
       return line;
   }
   return lines.length > 0 ? lines[0] : \"\";
 }
 result.value = getDescription(arguments.item(0)).replace(/^[\\s�@]*/, \"\");
 
 ', 'JScript', @Body('', @True())), %Subject)

�g�������t�H���_���J���āA((<[�\��]-[�J�������J�X�^�}�C�Y]|URL:ConfigViewsAction.html>))��I�сA�_�C�A���O��[����]��I���[�ҏW]���N���b�N���܂��B���̃_�C�A���O��[�^�C�v]��[���̑�]���w�肵�āA[�}�N��]�ɏ�L�̃}�N�����w�肵�A[�L���b�V��]�Ƀ`�F�b�N������[OK]���N���b�N���܂��B

��L�̃_�C�A���O�ł̓}�N�����w�肷��Ƃ���Ɉ�s����������Ȃ��̂ŁA��̃}�N������s�ɂ������̂��ȉ��ł��B

@If(@Contain(%Subject, 'Re:'), @Script('function getDescription(text) { var maxline = 20; var minchar = 5; var regexes = [ /^\\s+$/, /^[>|#���b��]/, /^In (message|article)\\s/i, /[+-]\\d{4},?\\s*$/, /(writes|wrote|����)[:�F]\\s*$/i, /����ɂ���|����ɂ���|����΂��/, /(�\���܂�|�ł�)�B?$/, /���b/, /(.)\\1{4}/, /^[\\w-]+:/ ]; var lines = text.split(/\\n/, maxline); for (n in lines) { var line = lines[n]; if (line.length < minchar) continue; var match = false; for (r in regexes) { var regex = regexes[r]; match = line.search(regex) != -1; if (match) break; } if (!match) return line; } return lines.length > 0 ? lines[0] : \"\"; } result.value = getDescription(arguments.item(0)).replace(/^[\\s�@]*/, \"\");', 'JScript', @Body('', @True())), %Subject)

=end

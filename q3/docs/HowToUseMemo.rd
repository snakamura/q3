=begin
=���b�Z�[�W�Ƀ�����t���邱�Ƃ͂ł��܂���?

�Z�������ł���΁A((<���x��|URL:Label.html>))���g�����Ƃ��ł��܂��B�����ƒ���������t�������ꍇ�ɂ́A�ȉ��̂悤�Ƀ}�N���Ȃǂ�g�ݍ��킹�邱�ƂŁA���b�Z�[�W�Ƀ��������邱�Ƃ��ł��܂��B

�e���b�Z�[�W�ɕt����ꂽ��������̃t�@�C���Ƃ��ĕۑ�����Ƃ������@�����܂��B�܂��A������ۑ�����f�B���N�g���Ƃ��āA���[���{�b�N�X�t�H���_��memo�Ƃ����f�B���N�g�����쐬���Ă����܂��B

���ꂩ��A���[���{�b�N�X�f�B���N�g���̉���macro�Ƃ����f�B���N�g�����쐬���A�ȉ��̃}�N����memo.macro�Ƃ����t�@�C�����ŕۑ����܂��B

 @Progn(# �����̃t�@�C�������擾���܂��B
        @Defun('GetFileName',
               @Concat('memo/',
                       @RegexReplace(Message-Id,
                                     /[^A-Za-z0-9_.!@#$%^&-]/,
                                     '',
                                     @True()))),
        # ������ǂݍ���ŕԂ��܂��B�ǂݍ��߂Ȃ��ꍇ�ɂ͋󕶎����Ԃ��܂��B
        @Defun('LoadMemo',
               @If(@CanMemo(),
                   @Catch(@Load(@GetFileName()), ''),
                   '')),
        # ������ۑ����܂��B��Ԗڂ̈����ɕۑ����郁�����w�肵�܂��B
        @Defun('SaveMemo',
               @If(@CanMemo(),
                   @Save(@GetFileName(), $1),
                   @False())),
        # ���������邩�ǂ������ׂ܂��BMessage-Id�������ƃ����͎��܂���B
        @Defun('CanMemo',
               Message-Id),
        # ���̓_�C�A���O���J���ă�������͂��ۑ����܂��B
        @Defun('InputMemo',
               @If(@CanMemo(),
                   @Catch(@SaveMemo(@InputBox('��������͂��Ă�������',
                                              :INPUT-MULTILINE,
                                              @LoadMemo())),
                          @MessageBox('�����̕ۑ��Ɏ��s���܂���')),
                   @MessageBox('Message-Id���Ȃ��̂Ń��������܂���'))))

�����āA((<menus.xml|URL:MenusXml.html>))��ҏW���A���j���[��\���������Ƃ���Ɉȉ��̂悤�ȃG���g����ǉ����܂��B

 <menuitem text="Mem&amp;o..."
           action="MessageMacro"
           param="@Progn(@Include('macro/memo.macro'), @InputMemo())"/>

�K�v�ɉ����āA((<toolbars.xml|URL:ToolbarsXml.html>))�ɃG���g����ǉ����ăc�[���o�[�̃{�^���ɂ��邱�Ƃ��ł��܂��B

����ɁA((<header.xml|URL:HeaderXml.html>))��ҏW���ă��b�Z�[�W�ɕt����������\������悤�ɂ��܂��B�ȉ��̂悤�ȃG���g����������\���������Ƃ���ɒǉ����܂��B

 <line hideIfEmpty="memo">
   <static width="auto" style="bold" showAlways="true">Memo:</static>
   <edit name="memo" multiline="4" wrap="true">{@Progn(@Include('macro/memo.macro'), @LoadMemo())}</edit>
 </line>

�������A���̕��@�ł͈ȉ��̂悤�Ȑ���������܂��B

*��������͂�������̓r���[���X�V����Ȃ��̂Ńw�b�_�r���[�Ƀ������\������Ȃ�
*�������폜���Ă��t�@�C�����폜����Ȃ�
*IMAP4�A�J�E���g�ł��T�[�o���Ƀ��������킯�ł͂Ȃ��̂ŕʂ�PC�ƃ��������L�ł��Ȃ�

=end

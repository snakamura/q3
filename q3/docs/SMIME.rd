=begin
=S/MIME

QMAIL3�ł�S/MIME�ɂ��Í����Ə������T�|�[�g���Ă��܂��B


==�K�v�ȃ��C�u����
SSL���g�p����ɂ́AOpenSSL�̃��C�u�������K�v�ł��B�C���X�g�[���ŃC���X�g�[�������ꍇ�A[SSL, S/MIME]�Ƀ`�F�b�N�����Ă����OpenSSL�̃��C�u�����͊��ɃC���X�g�[������Ă��܂��B���̑��̏ꍇ�ɂ́A�_�E�����[�h�y�[�W���烉�C�u�������_�E�����[�h���Alibeay32.dll��libssl32.dll��q3u.exe�Ɠ����f�B���N�g���ɒu���܂��B

�܂��Aqscryptou.dll���Ȃ��ꍇ�ɂ́A�z�z�t�@�C������C���X�g�[������K�v������܂��B


==�ؖ���
S/MIME�Ŏg�p���邽�߂̃��[�g�ؖ����̓f�t�H���g�ŃV�X�e���̏ؖ����X�g�A���烍�[�h����܂��B�ڍׂɂ��ẮA((<���[�g�ؖ���|URL:RootCertificate.html>))���Q�Ƃ��Ă��������B


==�����̔閧���Əؖ���
S/MIME�ŏ�����������Í�������đ����Ă������[���𕜍�������ɂ͎����̔閧���Əؖ������C���X�g�[������K�v������܂��B�����̔閧����PEM�`���ŃA�J�E���g�̃t�H���_�iaccounts/<�A�J�E���g��>�j��key.pem�Ƃ������O�Œu���Ă��������B�ؖ����͓����t�H���_�ɓ�����PEM�`����cert.pem�Ƃ������O�Œu���Ă��������B

�������A�T�u�A�J�E���g��Identity���g���Ă���ꍇ�ɂ́A�t�@�C�����͂��ꂼ��key_<Identity��>.pem��cert_<Identity��>.pem�ɂȂ�܂��B


==���̐l�̏ؖ���
�Í������ꂽ���[���𑗐M����ɂ͎�M�҂̏ؖ������C���X�g�[������K�v������܂��B�ؖ�����PEM�`���ɂ��ĔC�ӂ̃t�@�C�����Ń��[���{�b�N�X�f�B���N�g���ȉ���security�f�B���N�g���ɂ����Ă��������B�g���q��.pem�ɂ��܂��B����ɁA((<�A�h���X��|URL:AddressBook.html>))�ł��̏ؖ������w�肵�܂��B

���Ƃ��΁Afoo@example.com�Ƃ����A�h���X�̐l�̏ؖ������C���X�g�[������ɂ́Asecurity/foo.pem�ɏؖ�����PEM�`���ł����A�A�h���X���̏ؖ����̎w��Łufoo�v�Ǝw�肵�܂��B


==�����Ə����̌���
�����⏐���̌��؂��s���ɂ́A((<"[�\��]-[���[�h]-[S/MIME]"|URL:ViewSMIMEModeAction.html>))�Ƀ`�F�b�N������S/MIME���[�h��On�ɂ��܂��BS/MIME���[�h��On�ɂ���ƃ��b�Z�[�W��ǂݍ��ނƂ���S/MIME�̕����⏐���̌��؂������I�ɍs���܂��B��������((<�p�X���[�h|URL:Password.html>))���K�v�ȏꍇ�ɂ́A[�p�X���[�h]�_�C�A���O���J���܂��B

���b�Z�[�W�𕜍������菐�������؂���ƃX�e�[�^�X�o�[�ɃA�C�R�����\������܂��B

// TODO �摜

���������ꍇ�ɂ͌��}�[�N���A���������؂����ꍇ�ɂ̓`�F�b�N�}�[�N���A���؂Ɏ��s�����ꍇ�ɂ́~�}�[�N���\������܂��B�`�F�b�N�}�[�N�܂��́~�}�[�N���N���b�N����ƁA���������Ƃ��Ɏg�p�����ؖ������\������܂��B

// TODO �摜

�܂��A�����̌��؂ɐ��������ꍇ�ɂ́A�w�b�_�r���[��From�̍s�̔w�i�F���������F�ɕς��܂��B�܂��ASigned by�̍s�ɁA��������̂Ɏg�p���ꂽ�ؖ�����DN���\������܂��B

// TODO �摜


==�Í����Ə���
���b�Z�[�W���Í�������ɂ́A�G�f�B�b�g�E�B���h�E�ŁA((<"[�c�[��]-[S/MIME]-[�Í���]"|URL:ToolSMIMEEncryptAction.html>))�Ƀ`�F�b�N�����ĈÍ�������悤�ɐݒ肵�܂��B���l�ɏ�������ɂ́A((<"[�c�[��]-[S/MIME]-[����]"|URL:ToolSMIMESignAction.html>))�Ƀ`�F�b�N�����ď�������悤�ɐݒ肵�܂��B

�����̃f�t�H���g�l�́A((<�G�f�B�b�g�r���[2�̐ݒ�|URL:OptionEdit2.html>))�Ŏw�肷�邱�Ƃ��ł��܂��B


==�ݒ�
�����̌`���Ƃ���application/pkcs7-mime�`�����g�p���邩multipart/signed�`�����g�p���邩�A�Í�������Ƃ��Ɏ����̌��ł��Í������邩�Ƃ�����S/MIME�̐ݒ�́A((<�G�f�B�b�g�r���[2�̐ݒ�|URL:OptionEdit2.html>))�ōs���܂��B


==�ؖ����⌮�t�@�C���̍���
�V�X�e���̏ؖ����X�g�A����PKCS#12�`���ŃG�N�X�|�[�g�����t�@�C����OpenSSL�̃R�}���h���g�p����PEM�`���ɂ��邱�Ƃ��ł��܂��B

 #CA�̏ؖ����̎擾
 openssl pkcs12 -in example.p12 -nokeys -cacerts -out ca.pem
 # �����̏ؖ����̎擾
 openssl pkcs12 -in example.p12 -nokeys -clcerts -out cert.pem
 # �����̔閧���̎擾
 openssl pkcs12 -in example.p12 -nocerts -nodes -out key.pem

=end

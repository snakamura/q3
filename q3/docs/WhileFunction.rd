=begin
=@While

 Value @While(Boolean condition, Value value)


==����
condition�̒l��True�̊ԁAvalue�̒l���J��Ԃ��]�����܂��Bcondition������]������܂��B�Ō�ɕ]�����ꂽcondition���Ԃ���܂��B

�g���������Ɩ������[�v���܂��̂Œ��ӂ��Ă��������B


==����
:Boolean condition
  �J��Ԃ��̏���
:Value value
  �]������l


==�G���[
*�����̐��������Ă��Ȃ��ꍇ


==����
�Ȃ�


==��
 # �ϐ��ɒl�������Ȃ���10�񃋁[�v����
 @While(@Less(@Variable('n', 0), 10),
        @Progn(@MessageBox($n),
               @Set('n', @Add($n, 1))))

=end

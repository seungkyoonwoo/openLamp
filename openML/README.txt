* ȯ�溯�� ����
- MOBILE_LITE_HOME : openML ��� ����
    ex) $ setenv MOBILE_LITE_HOME /home/openML
- MOBILE_LITE_CONFIG : DB file �� openml.cfg ��� ����
    ex) $ setenv MOBILE_LITE_CONFIG $MOBILE_LITE_HOME/dbspace
- LD_LIBRARY_PATH ����
    ex) $ setenv LD_LIBRARY_PATH ${LD_LIBRARY_PATH}:$MOBILE_LITE_HOME/lib

* config.mk
- openML�� compile option�� �����ϴ� file
- OPTION : �����Ǵ� define option�� �߰� ����
- ENABLE_YACC : make�� parser compile ���� ���� 
    1 : parser compile ����
    0 : parser compile ������
- JNI : openML JNI library ���� ����
    1 : JNI library compile ����
    0 : JNI libaray compile ������
- HAVE_READLINE : isql tool�� ���� interactive prompt ��� ����
    1 : interactive prompt ��� ����
    0 : interactive prompt ��� ������
- COMPILE_MODE : compile mode ����
    Default : mode32 (Makefile ����)
- COMPILER : compiler ���� 
    Default : gcc (Makefile ����)

* make
- openML compile�� config.mk�� ������ �������� Makefile�� ����
- Debug(default)/Release mode ���ڷ� ����
    $ make release
- COMPILE_MODE, COMPILER�� ���ڷ� ����. ������ config.mk�� ���� �����
    $ make mode64
    $ make arm-gcc-android
    $ make
- lib ������ libopenml.a�� libopenml.so�� ������
- bison, flex ��� ����
    bison 2.5, flex 2.5.34 �̻� ��� �ǰ�

* JNI
- compile
a. config.mk�� 'JNI=0'�� 'JNI=1'�� ����
b. ȯ�溯�� JAVA_HOME�� ��ġ�� java directory�� ����
    ex) $ setenv JAVA_HOME /usr/lib/jvm/java
c. make
- ����
a. openML�� include ������ �ִ� JEDB.java�� �۾� ���������� com.openml�� ����
b. openML�� lib ������ ������ JNI library�� libjedb.so�� ��ũ �ϵ��� �ڵ� �߰�
c. example.java �ۼ� (�ڵ� �ۼ��� JEDB.java�� ���ǵ� �Լ����� �̿�)

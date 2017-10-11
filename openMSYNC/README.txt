****************************************************************************
*** Linux �󿡼��� build ���
****************************************************************************
* ȯ�溯�� ����
- openML build ȯ�� ���࿡�� �����ϴ� ȯ�� ������ ���. 
- ������: openMSYNC ���� ������ openML�� ������ �ξ�� �Ѵ�.

* config.mk
- openMSYNC�� compile option�� �����ϴ� file
- OPTION : �����Ǵ� define option�� �߰� ����
- JNI : openMSYNC JNI library ���� ����
    1 : JNI library compile ����
    0 : JNI libaray compile ������
- COMPILE_MODE : compile mode ����
    Default : mode64 (Makefile ����)
- COMPILER : compiler ���� 
    Default : gcc (Makefile ����)

* make
- openMSYNC compile�� config.mk�� ������ �������� Makefile�� ����
- Debug(default)/Release mode ���ڷ� ����
    $ make release
- COMPILE_MODE, COMPILER�� ���ڷ� ����. ������ config.mk�� ���� �����
    $ make mode64
    $ make arm-gcc-android
    $ make
- lib ������ libmSyncClient.a�� libmSyncClient.so�� ������
  
* JNI
- compile
a. config.mk�� 'JNI=0'�� 'JNI=1'�� ����
b. ȯ�溯�� JAVA_HOME�� ��ġ�� java directory�� ����
    ex) $ setenv JAVA_HOME /usr/lib/jvm/java
c. make
- lib ������ libmSyncClientJNI.so�� ������


- ����
a. openMSYNC/client/mSyncClientJNI/com ������ �۾� ���������� ����
b. openMSYNC�� lib ������ ������ JNI library�� libmSyncClientJNI.so�� ��ũ �ϵ��� �ڵ� �߰�
c. example.java �ۼ� (�ڵ� �ۼ��� mSyncClientJNI.java�� ���ǵ� �Լ����� �̿�)
d. openML�� ������ ��� 
  d-1. openML�� include ������ �ִ� JEDB.java�� �۾� ���������� com/openml�� ����
  d-2. openML�� lib ������ ������ JNI library�� libjedb.so�� ��ũ �ϵ��� �ڵ� �߰�
  d-3. example.java �ۼ� (�ڵ� �ۼ��� JEDB.java�� ���ǵ� �Լ����� �̿�)

****************************************************************************
*** Windows �󿡼��� build ���
****************************************************************************
* �����Ϸ��� VC 6.0�� �������� �Ǿ� ����.
* Workspace �ε� �� ������ ����
   (������: openMSYNC ���� ������ openML�� ������ �ξ�� �Ѵ�.)
a. openMSYNC/openMSYNC.dsw �� loading
b. "Project setting"���� mSyncClientJNI ������Ʈ�� �����ϰ�  "C/C++" ���� Category�� "Preprocessor"��
   ��ȯ�� �� include directories���� JAVA JDK�� include ��θ� ������ �ش�.
   
   ���� �⺻������ ������ ���� ��θ� �����ϵ��� �ϰ� ������ �����ϰ� �����ؼ� ����Ѵ�.
   C:\Program Files\Java\jdk1.7.0_05\include,C:\Program Files\Java\jdk1.7.0_05\include\win32
c. Admin -> mSyncDBCTL -> mSync -> mSyncClient -> mSyncClientJNI ������ ���� �ϵ��� �Ѵ�.  



/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class com_openMSYNC_mSyncClientJNI */

#ifndef _Included_com_openMSYNC_mSyncClientJNI
#define _Included_com_openMSYNC_mSyncClientJNI
#ifdef __cplusplus
extern "C" {
#endif
#undef com_openMSYNC_mSyncClientJNI_UPDATE_FLAG
#define com_openMSYNC_mSyncClientJNI_UPDATE_FLAG 85L
#undef com_openMSYNC_mSyncClientJNI_DELETE_FLAG
#define com_openMSYNC_mSyncClientJNI_DELETE_FLAG 68L
#undef com_openMSYNC_mSyncClientJNI_INSERT_FLAG
#define com_openMSYNC_mSyncClientJNI_INSERT_FLAG 73L
#undef com_openMSYNC_mSyncClientJNI_DOWNLOAD_FLAG
#define com_openMSYNC_mSyncClientJNI_DOWNLOAD_FLAG 70L
#undef com_openMSYNC_mSyncClientJNI_DOWNLOAD_DELETE_FLAG
#define com_openMSYNC_mSyncClientJNI_DOWNLOAD_DELETE_FLAG 82L
#undef com_openMSYNC_mSyncClientJNI_END_FLAG
#define com_openMSYNC_mSyncClientJNI_END_FLAG 69L
#undef com_openMSYNC_mSyncClientJNI_OK_FLAG
#define com_openMSYNC_mSyncClientJNI_OK_FLAG 75L
#undef com_openMSYNC_mSyncClientJNI_TABLE_FLAG
#define com_openMSYNC_mSyncClientJNI_TABLE_FLAG 77L
#undef com_openMSYNC_mSyncClientJNI_APPLICATION_FLAG
#define com_openMSYNC_mSyncClientJNI_APPLICATION_FLAG 83L
#undef com_openMSYNC_mSyncClientJNI_ACK_FLAG
#define com_openMSYNC_mSyncClientJNI_ACK_FLAG 65L
#undef com_openMSYNC_mSyncClientJNI_NACK_FLAG
#define com_openMSYNC_mSyncClientJNI_NACK_FLAG 78L
#undef com_openMSYNC_mSyncClientJNI_XACK_FLAG
#define com_openMSYNC_mSyncClientJNI_XACK_FLAG 88L
#undef com_openMSYNC_mSyncClientJNI_UPGRADE_FLAG
#define com_openMSYNC_mSyncClientJNI_UPGRADE_FLAG 86L
#undef com_openMSYNC_mSyncClientJNI_END_OF_TRANSACTION
#define com_openMSYNC_mSyncClientJNI_END_OF_TRANSACTION 84L
#undef com_openMSYNC_mSyncClientJNI_NO_SCRIPT_FLAG
#define com_openMSYNC_mSyncClientJNI_NO_SCRIPT_FLAG 66L
#undef com_openMSYNC_mSyncClientJNI_ALL_SYNC
#define com_openMSYNC_mSyncClientJNI_ALL_SYNC 1L
#undef com_openMSYNC_mSyncClientJNI_MOD_SYNC
#define com_openMSYNC_mSyncClientJNI_MOD_SYNC 2L
#undef com_openMSYNC_mSyncClientJNI_DBSYNC
#define com_openMSYNC_mSyncClientJNI_DBSYNC 1L
#undef com_openMSYNC_mSyncClientJNI_FIELD_DEL
#define com_openMSYNC_mSyncClientJNI_FIELD_DEL 8L
#undef com_openMSYNC_mSyncClientJNI_RECORD_DEL
#define com_openMSYNC_mSyncClientJNI_RECORD_DEL 127L
#undef com_openMSYNC_mSyncClientJNI_NORMAL
#define com_openMSYNC_mSyncClientJNI_NORMAL 1L
#undef com_openMSYNC_mSyncClientJNI_ABNORMAL
#define com_openMSYNC_mSyncClientJNI_ABNORMAL 2L
#undef com_openMSYNC_mSyncClientJNI_HUGE_STRING_LENGTH
#define com_openMSYNC_mSyncClientJNI_HUGE_STRING_LENGTH 8156L
/*
 * Class:     com_openMSYNC_mSyncClientJNI
 * Method:    mSync_InitializeClientSession
 * Signature: (Ljava/lang/String;I)V
 */
JNIEXPORT void JNICALL Java_com_openMSYNC_mSyncClientJNI_mSync_1InitializeClientSession
  (JNIEnv *, jclass, jstring, jint);

/*
 * Class:     com_openMSYNC_mSyncClientJNI
 * Method:    mSync_ConnectToSyncServer
 * Signature: (Ljava/lang/String;I)I
 */
JNIEXPORT jint JNICALL Java_com_openMSYNC_mSyncClientJNI_mSync_1ConnectToSyncServer
  (JNIEnv *, jclass, jstring, jint);

/*
 * Class:     com_openMSYNC_mSyncClientJNI
 * Method:    mSync_AuthRequest
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;I)I
 */
JNIEXPORT jint JNICALL Java_com_openMSYNC_mSyncClientJNI_mSync_1AuthRequest
  (JNIEnv *, jclass, jstring, jstring, jstring, jint);

/*
 * Class:     com_openMSYNC_mSyncClientJNI
 * Method:    mSync_SendDSN
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_openMSYNC_mSyncClientJNI_mSync_1SendDSN
  (JNIEnv *, jclass, jstring);

/*
 * Class:     com_openMSYNC_mSyncClientJNI
 * Method:    mSync_SendTableName
 * Signature: (ILjava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_openMSYNC_mSyncClientJNI_mSync_1SendTableName
  (JNIEnv *, jclass, jint, jstring);

/*
 * Class:     com_openMSYNC_mSyncClientJNI
 * Method:    mSync_SendUploadData
 * Signature: (Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_openMSYNC_mSyncClientJNI_mSync_1SendUploadData
  (JNIEnv *, jclass, jstring, jstring);

/*
 * Class:     com_openMSYNC_mSyncClientJNI
 * Method:    mSync_UploadOK
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_openMSYNC_mSyncClientJNI_mSync_1UploadOK
  (JNIEnv *, jclass, jstring);

/*
 * Class:     com_openMSYNC_mSyncClientJNI
 * Method:    mSync_ReceiveDownloadData
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_openMSYNC_mSyncClientJNI_mSync_1ReceiveDownloadData
  (JNIEnv *, jclass);

/*
 * Class:     com_openMSYNC_mSyncClientJNI
 * Method:    mSync_Disconnect
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_openMSYNC_mSyncClientJNI_mSync_1Disconnect
  (JNIEnv *, jclass, jint);

/*
 * Class:     com_openMSYNC_mSyncClientJNI
 * Method:    mSync_ClientLog
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_openMSYNC_mSyncClientJNI_mSync_1ClientLog
  (JNIEnv *, jclass, jstring);

/*
 * Class:     com_openMSYNC_mSyncClientJNI
 * Method:    mSync_GetSyncError
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_openMSYNC_mSyncClientJNI_mSync_1GetSyncError
  (JNIEnv *, jclass);

/*
 * Class:     com_openMSYNC_mSyncClientJNI
 * Method:    mSync_SendUploadData2
 * Signature: (Ljava/lang/String;[B)I
 */
JNIEXPORT jint JNICALL Java_com_openMSYNC_mSyncClientJNI_mSync_1SendUploadData2
  (JNIEnv *, jclass, jstring, jbyteArray);

/*
 * Class:     com_openMSYNC_mSyncClientJNI
 * Method:    mSync_ReceiveDownloadData2
 * Signature: ()[B
 */
JNIEXPORT jbyteArray JNICALL Java_com_openMSYNC_mSyncClientJNI_mSync_1ReceiveDownloadData2
  (JNIEnv *, jclass);

#ifdef __cplusplus
}
#endif
#endif

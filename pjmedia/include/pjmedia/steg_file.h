/*
* provide interface for jni!
* do something to send file or msg by myself
* added by MuBonan on 2018/4/12
*/
#include <pj/os.h>
#include <pj/file_io.h>

struct FileCallback{
	void (*on_file_receive_progress)(const pj_str_t *fName, unsigned int len, float p);
	void (*on_file_receive_result)(const pj_str_t *fName, int status);
	void (*on_file_send_progress)(const pj_str_t *fName, unsigned int len, float p);
	void (*on_file_send_result)(const pj_str_t *fName, int status);
	void (*on_msg_receive_result)(const pj_str_t *msg);
	void (*on_msg_send_result)(int status);
};
#ifndef LAST_TAG_FILE_CANCEL
#define LAST_TAG_FILE_CANCEL ">>1"
#endif // !LAST_TAG_FILE_CANCEL

#ifndef LAST_TAG_FILE_RECEIVED_REPLY
#define LAST_TAG_FILE_RECEIVED_REPLY ">>2"
#endif // !LAST_TAG_FILE_RECEIVED_REPLY

#ifndef LAST_TAG_FILE_SENDED_REQUEST
#define LAST_TAG_FILE_SENDED_REQUEST ">>3"
#endif // !LAST_TAG_FILE_SENDED_REQUEST

#ifndef LAST_END_STR
#define LAST_END_STR '\0'
#endif // !LAST_END_STR



class StegFile
{
public:
	pj_thread_t *thread_file_steg, *thread_file_steg_send, *thread_file_steg_receive;
	StegFile();
	~StegFile();

	void initStegFile(pj_pool_t* pool);

	//发送文件
	pj_status_t sendFile(pj_str_t *filepath, pj_str_t *filename);
	pj_status_t isFileSending();
	pj_status_t isFileReceiving();
	pj_status_t cancelFileTranslating(pj_str_t *fileName);
	//发送消息
	pj_status_t sendMsg(pj_str_t *strmsg);
	//SIA层数据发送完毕
	pj_status_t OnSIAClear(pj_pool_factory *pf, pj_pool_t *pool, int type);
	//有新的SIA层数据到达
	pj_status_t OnSIArrive(pj_pool_factory *pf, pj_pool_t *pool, int type);
	void OnSenderTimer();
	void OnReceiverTimer();
	pj_status_t thread_start(pj_str_t *receiveFileRoot);
	pj_status_t thread_stop(pj_str_t *receiveFileRoot);
	pj_size_t getMaxSIADU();
	void set_file_cb(FileCallback cb);
private:
	pj_str_t * thread_id_RFRoot = NULL;  //作为thread启动id，同时是接收文件根路径
	pj_status_t m_SFStep, m_RFStep; //发送和接收文件的阶段
	pj_str_t *m_SFPath = NULL, *m_SFName = NULL, *m_RFName = NULL;  //发送和接收文件路径
	pj_str_t* m_RFPath = NULL;  //接收文件全路径

	pj_oshandle_t m_fileS=NULL, m_fileR=NULL;  //发送和接收文件
	unsigned int m_SLength= -1, m_SndLength= -1, m_RLength= -1, m_RcvLength= -1;  //发送和接收文件长度
	pj_str_t m_SFP, m_RFP;  //发送和接收提示字符串，在java层回调时使用
	pj_pool_t *pool_file_steg;
	FileCallback file_cb;
	pj_str_t *tag_receiver_file_cancel = NULL, *tag_receiver_file_success_request = NULL, *tag_receiver_file_success_reply = NULL;
	pj_str_t *tag_sender_file_cancel = NULL, *tag_sender_file_success_request = NULL, *tag_sender_file_success_reply = NULL;
	int count_receive = -1, MAX_COUNT_RECEIVE = 0, count_send_wait_reply=-1, MAX_COUNT_SEND_WAIT_REPLY=0;
	void releaseCaching();
};

#include <pjmedia/steg_file.h>
#include <StegSuit.h>
#include <pj/string.h>
#include <pj/pool.h>
#include <pj/log.h>

/**
* steg_file.cpp
* do some thing like ConDlg.cpp of microsip in pjproject-2.4
* added by MuBonan on 2018/4/12
*/
extern CStegSuit m_pSteg;
#ifndef PJMEDIA_FILE_SIZE
#   define PJMEDIA_FILE_SIZE	1000
#endif

#ifndef PJMEDIA_FILE_INC
#   define PJMEDIA_FILE_INC	1000
#endif

#ifndef PJMEDIA_FILE_THREAD_DURATION
#   define PJMEDIA_FILE_THREAD_DURATION	100 // sleep time duaring steg_thread run, min=10ms; Notice: should not more than 16*20/2=160ms
#endif
#ifndef PJMEDIA_FILE_THREAD_DURATION_SEND
#define PJMEDIA_FILE_THREAD_DURATION_SEND 330  //send sleep time should more than 320ms, for put SD after send rtp success;
#endif // !PJMEDIA_FILE_THREAD_DURATION_SEND
#ifndef PJMEDIA_FILE_THREAD_DURATION_RECEIVE
#define PJMEDIA_FILE_THREAD_DURATION_RECEIVE 20  //receive sleep time should smaller than 160ms and not lower than 20ms, for receive lost frame from rtp, and case of using lost of cpus;
#endif // !PJMEDIA_FILE_THREAD_DURATION_RECEIVE


#define THIS_FILE "steg_file.cpp"


/*
* Clock thread
*/
static int steg_thread(void *arg)
{
	while (true)
	{
		StegFile *stegFile = (StegFile*)arg;
		if (stegFile&&stegFile->thread_file_steg) {
			if (m_pSteg.bMessageSent)
			{
				stegFile->OnSIAClear(NULL, NULL, 1);
			}
			if (m_pSteg.bMessageArrived)
			{
				stegFile->OnSIArrive(NULL, NULL, 1);
			}
			if (m_pSteg.bFileSent)
			{
				stegFile->OnSIAClear(NULL, NULL, 2);
			}
			if (m_pSteg.bFileArrived)
			{
				stegFile->OnSIArrive(NULL, NULL, 2);
			}
			pj_thread_sleep(PJMEDIA_FILE_THREAD_DURATION);
		}
		else {
			break;
		}
	}
	return 0;
}

static int steg_thread_send(void *arg)
{
	while (true)
	{
		StegFile *stegFile = (StegFile*)arg;
		if (stegFile&&stegFile->thread_file_steg_send) {
			stegFile->OnSenderTimer();
			if (m_pSteg.bMessageSent)
			{
				stegFile->OnSIAClear(NULL, NULL, 1);
			}
			if (m_pSteg.bFileSent)
			{
				stegFile->OnSIAClear(NULL, NULL, 2);
			}
			pj_thread_sleep(PJMEDIA_FILE_THREAD_DURATION_SEND);
		}
		else {
			break;
		}
	}
	return 0;
}

static int steg_thread_receive(void *arg)
{
	while (true)
	{
		StegFile *stegFile = (StegFile*)arg;
		if (stegFile&&stegFile->thread_file_steg_receive) {
			stegFile->OnReceiverTimer();
			if (m_pSteg.bMessageArrived)
			{
				stegFile->OnSIArrive(NULL, NULL, 1);
			}
			if (m_pSteg.bFileArrived)
			{
				stegFile->OnSIArrive(NULL, NULL, 2);
			}
			pj_thread_sleep(PJMEDIA_FILE_THREAD_DURATION_RECEIVE);
		}
		else {
			break;
		}
	}
	return 0;
}

StegFile::StegFile()
{
	m_SFStep = 0;
	m_RFStep = 0;
	m_SLength = 0;
	m_SndLength = 0;
	m_RLength = 0;
	m_RcvLength = 0;
	count_receive = -1;
	count_send_wait_reply = -1;
}

StegFile::~StegFile()
{
	releaseCaching();
}

void StegFile::initStegFile(pj_pool_t* pool)
{
	m_SFStep = 0;
	m_RFStep = 0;
	m_SLength = 0;
	m_SndLength = 0;
	m_RLength = 0;
	m_RcvLength = 0;
	count_receive = -1;
	count_send_wait_reply = -1;
	pool_file_steg = pool;
	if (thread_id_RFRoot!=NULL)
	{
		thread_start(thread_id_RFRoot);
	}
}

#ifdef WIN32
#include <fstream>
UINT file_size(char * filename) {
	std::ifstream fsRead;
	fsRead.open(filename, std::ios::in | std::ios::binary);
	if (!fsRead) {
		return -3;
	}
	fsRead.seekg(0, fsRead.end);
	UINT srcSize = fsRead.tellg();
	if (!srcSize) {
		fsRead.close();
	}
	return srcSize;
}
#else
#include <sys/stat.h>  
UINT file_size(char* filename)
{
	struct stat statbuf;
	stat(filename, &statbuf);
	UINT size = statbuf.st_size;
	return size;
}
#endif // WIN32

pj_status_t StegFile::sendFile(pj_str_t *filepath, pj_str_t *filename)
{
	if (m_SFPath != NULL)
	{
		if (m_SFPath->ptr != NULL)
		{
			delete[] m_SFPath->ptr;
			m_SFPath->ptr = NULL;
			m_SFPath->slen = 0;
		}
		delete[] m_SFPath;
		m_SFPath = NULL;
	}
	m_SFPath = new pj_str_t;
	m_SFPath->ptr = new char[filepath->slen+1];
	strcpy(m_SFPath->ptr, filepath->ptr);
	m_SFPath->slen = filepath->slen;
//	PJ_LOG(4, (THIS_FILE, "sendFile():before copy file dir:%s!", filepath->ptr));
//	PJ_LOG(4, (THIS_FILE, "sendFile():after copy file dir:%s!", m_SFPath->ptr));
	if (m_SFName != NULL)
	{
		if (m_SFName->ptr != NULL)
		{
			delete[] m_SFName->ptr;
			m_SFName->ptr = NULL;
			m_SFName->slen = 0;
		}
		delete[] m_SFName;
		m_SFName = NULL;
	}
	m_SFName = new pj_str_t;
	m_SFName->ptr = new char[filename->slen - 1];
	pj_memset(m_SFName->ptr, 0, (filename->slen - 1) * sizeof(char));
	pj_memcpy(m_SFName->ptr, filename->ptr + 1 * sizeof(char), (filename->slen - 2) * sizeof(char));
	m_SFName->slen = filename->slen - 2;
	PJ_LOG(4, (THIS_FILE, "sendFile():send file name:%s, len=%d!", m_SFName->ptr, m_SFName->slen));

	m_SFStep = 1;
	pj_status_t status=-1;
	if (filename->slen <= 0)
	{
		printf("empty file name!!\r\n");
		m_SFStep = 0;
		status = -2;
	}
	else if (filename->slen + 2 >= m_pSteg.SIADU || filename->slen + 1 > m_pSteg.SIADU / 2 / COUNT_WINDOW_CACHE) {
		printf("too long file name!!\r\n");
		m_SFStep = 0;
		status = -3;
	}
	else {
		if (m_pSteg.SLock) {
			m_pSteg.lock();
			status = m_pSteg.Send((void *)filename->ptr, filename->slen + 1, 2);
			m_pSteg.unlock();
		}

		//todo: new file tags on sender
		if (tag_file_cancel != NULL)
		{
			delete[] tag_file_cancel->ptr;
			tag_file_cancel->ptr = NULL;
			delete[] tag_file_cancel;
			tag_file_cancel = NULL;
		}
		tag_file_cancel = new pj_str_t;
		tag_file_cancel->ptr = new char[m_SFName->slen + 3 + 1];
		pj_strcpy(tag_file_cancel, m_SFName);
		pj_strcat2(tag_file_cancel, LAST_TAG_FILE_CANCEL);
		tag_file_cancel->ptr[tag_file_cancel->slen] = LAST_END_STR;

		if (tag_file_sended != NULL)
		{
			delete[] tag_file_sended->ptr;
			tag_file_sended->ptr = NULL;
			delete[] tag_file_sended;
			tag_file_sended = NULL;
		}
		tag_file_sended = new pj_str_t;
		tag_file_sended->ptr = new char[m_SFName->slen + 3 + 1];
		pj_strcpy(tag_file_sended, m_SFName);
		pj_strcat2(tag_file_sended, LAST_TAG_FILE_SENDED_REQUEST);
		tag_file_sended->ptr[tag_file_sended->slen] = LAST_END_STR;

		if (tag_file_received != NULL)
		{
			delete[] tag_file_received->ptr;
			tag_file_received->ptr = NULL;
			delete[] tag_file_received;
			tag_file_received = NULL;
		}
		tag_file_received = new pj_str_t;
		tag_file_received->ptr = new char[m_SFName->slen + 3 + 1];
		pj_strcpy(tag_file_received, m_SFName);
		pj_strcat2(tag_file_received, LAST_TAG_FILE_RECEIVED_REPLY);
		tag_file_received->ptr[tag_file_received->slen] = LAST_END_STR;
	}
	return status;
}

pj_status_t StegFile::isFileSending() {
	return m_SFStep;
}

pj_status_t StegFile::isFileReceiving() {
	return m_RFStep;
}

pj_status_t StegFile::cancelFileTranslating(pj_str_t *filename) {
	//todo: new file end tag
	pj_str_t * file_end_tag = new pj_str_t;
	file_end_tag->ptr = new char[filename->slen + 3 + 1];
	pj_strcpy(file_end_tag, filename);
	pj_strcat2(file_end_tag, LAST_TAG_FILE_CANCEL);
	file_end_tag->ptr[file_end_tag->slen] = LAST_END_STR;
	sendMsg(file_end_tag);
	delete[] file_end_tag->ptr;
	file_end_tag->ptr = NULL;
	file_end_tag->slen = 0;
	delete[] file_end_tag;
	file_end_tag = NULL;

	if (m_SFStep>0)
	{
		m_SFStep = 10;
		if (m_pSteg.SLock) {
			m_pSteg.lock();
			m_pSteg.bFileSent = true;
			m_pSteg.unlock();
		}else{
			m_pSteg.bFileSent = true;
		}
		return 1;
	}
	if (m_RFStep>0)
	{
		m_RFStep = 10;
		if (m_pSteg.SLock) {
			m_pSteg.lock();
			m_pSteg.bFileArrived = true;
			m_pSteg.unlock();
		}else{
			m_pSteg.bFileArrived = true;
		}
		return 2;
	}
	return 3;
}

pj_status_t StegFile::sendMsg(pj_str_t *strmsg)
{
	pj_status_t state=-1;
	pj_str_t *tmp=new pj_str_t;
	tmp->ptr = new char[strmsg->slen + 1];
	pj_strcpy(tmp, strmsg);
	if (tmp->slen <= 0)
	{
		printf("empty msg!!\r\n");
		state = -2;
	}
	else if (tmp->slen + 1 > m_pSteg.SIADU || tmp->slen + 1 > m_pSteg.SIADU/2/COUNT_WINDOW_CACHE) {
		printf("too long msg!!\r\n");
		state = -2;
	}
	else {
		PJ_LOG(4, (THIS_FILE, "sendMsg:slock=%d", m_pSteg.SLock != NULL));
		if (m_pSteg.SLock) {
			m_pSteg.lock();
			state = m_pSteg.Send((void *)tmp->ptr, tmp->slen, 1);
			m_pSteg.unlock();
		}
	}
	delete[] tmp->ptr;
	delete[] tmp;
	return state;
}

pj_size_t StegFile::getMaxSIADU() {
	return m_pSteg.SIADU;
}

pj_status_t StegFile::OnSIAClear(pj_pool_factory *pf, pj_pool_t *pool, int type) {
	PJ_LOG(4, ("steg_threadTAG", "--------steg_thread():file send before run! type=%d, send sleep time=%d(ms)", type, PJMEDIA_FILE_THREAD_DURATION_SEND));
	//如果是1，消息体
	if (type == 1)
	{
		if (m_pSteg.SLock) {
			m_pSteg.lock();
			m_pSteg.bMessageSent = false;
			m_pSteg.unlock();
		}
		file_cb.on_msg_send_result(PJ_SUCCESS);
//		PJ_LOG(4, ("steg_threadTAG", "++++++++++++++++++++steg_thread():1 file send after run, ready to sleep!"));
		return 1;
	}
	//如果是2则是二进制文件
	else if (type = 2)
	{
		//TODO; 2018/5/31 by BobMu 突然中断通话时直接跳显示接收失败
		if (!m_pSteg.SLock) {
			m_SFStep = 10;
		}
	
		pj_status_t status;
		if (m_SFStep == 1)
		{
			PJ_LOG(4, (THIS_FILE, "OnSIAClear():create file end tag str=%s, slen=%d, received str=%s, slen=%d, sended str=%s, slen=%d!", tag_file_cancel->ptr, tag_file_cancel->slen, tag_file_received->ptr, tag_file_received->slen, tag_file_sended->ptr, tag_file_sended->slen));

			//第一步：获取文件长度
			if (pool == NULL)
			{
				pool = pool_file_steg;
			}
			if (pool == NULL)
			{
				m_SFStep = 0;
				(file_cb.on_file_send_result)(m_SFName, -1);
				PJ_LOG(4, ("steg_threadTAG", "++++++++++++++++++++steg_thread():-1 file send after run, ready to sleep!"));
				return -1;
			}
			char* filePath = m_SFPath->ptr;
			m_SLength = 0;
			m_SndLength = 0;
			status = pj_file_open(pool, filePath, PJ_O_RDONLY, &m_fileS);
			pj_off_t off=0;
			pj_file_setpos(m_fileS, off, PJ_SEEK_END);
			PJ_LOG(4, (THIS_FILE, "OnSIAClear():~~~~~~~~~~~~~~~~~~~~~~after set pos end, file_length=%d, pj_get_length=%d!", m_SLength, (unsigned int)off));
			pj_file_getpos(m_fileS, &off);
			PJ_LOG(4, (THIS_FILE, "OnSIAClear():~~~~~~~~~~~~~~~~~~~~~~after get pos, file_length=%d, pj_get_length=%d!", m_SLength, (unsigned int)off));
			PJ_LOG(4, (THIS_FILE, "OnSIAClear():create send file:%s, status=%d!", filePath, status));
			m_SLength = off;
			if (m_SLength<=0)
			{
				m_SLength = file_size(filePath);
			}
			m_pSteg.lock();
			m_pSteg.bFileSent = false;
			m_pSteg.Send((void *)&m_SLength, sizeof(UINT), 2);
			m_pSteg.unlock();
			pj_file_setpos(m_fileS, 0, PJ_SEEK_SET);
			if (status != PJ_SUCCESS)
			{
				m_SFStep = 0;
				(file_cb.on_file_send_result)(m_SFName, -1);
//				PJ_LOG(4, ("steg_threadTAG", "++++++++++++++++++++steg_thread():-1 file send after run, ready to sleep!"));
				return -1;
			}
			m_SFStep = 2;
		}
		else if (m_SFStep == 2)
		{
			//第二步：传文件
			BYTE * tempread = new BYTE[m_pSteg.SIADU];
			pj_ssize_t length = m_pSteg.SIADU *sizeof(BYTE);
			pj_file_read(m_fileS, tempread, &length);
			m_pSteg.lock();
			m_pSteg.bFileSent = false;
			m_pSteg.Send(tempread, length, 2);
			m_pSteg.unlock();
			delete[] tempread;
			m_SndLength += length;
			PJ_LOG(4, (THIS_FILE, "OnSIAClear():step2 send file this len=%d, have sended length=%d, all length=%d!", length, m_SndLength, m_SLength));
			(file_cb.on_file_send_progress)(m_SFName, m_SLength, float(m_SndLength) / float(m_SLength));
			if (m_SndLength == m_SLength) {

				pj_str_t * file_sended_tag = new pj_str_t;
				file_sended_tag->ptr = new char[m_SFName->slen + 3 + 1];
				pj_strcpy(file_sended_tag, m_SFName);
				pj_strcat2(file_sended_tag, LAST_TAG_FILE_SENDED_REQUEST);
				file_sended_tag->ptr[file_sended_tag->slen] = LAST_END_STR;
				sendMsg(file_sended_tag);
				delete[] file_sended_tag->ptr;
				file_sended_tag->ptr = NULL;
				file_sended_tag->slen = 0;
				delete[] file_sended_tag;
				file_sended_tag = NULL;
				m_SFStep = 3;
			}
		}
		else if (m_SFStep == 3)
		{
			//第三步：发送完成，等待接收方接收完成
			pj_str_t * file_sended_tag = new pj_str_t;
			file_sended_tag->ptr = new char[m_SFName->slen + 3 + 1];
			pj_strcpy(file_sended_tag, m_SFName);
			pj_strcat2(file_sended_tag, LAST_TAG_FILE_SENDED_REQUEST);
			file_sended_tag->ptr[file_sended_tag->slen] = LAST_END_STR;
			sendMsg(file_sended_tag);
			delete[] file_sended_tag->ptr;
			file_sended_tag->ptr = NULL;
			file_sended_tag->slen = 0;
			delete[] file_sended_tag;
			if (count_send_wait_reply<0)
			{
				MAX_COUNT_SEND_WAIT_REPLY = 100;
				count_send_wait_reply = 0;
			}
		}
		else if (m_SFStep == 4)
		{
			//第四步，发送方等待完成，接收回应返回给发送方显示发送成功
			m_pSteg.lock();
			m_pSteg.bFileSent = false;
			m_pSteg.unlock();
			if (m_fileS)
			{
				pj_file_close(m_fileS);
				m_fileS = NULL;
			}
			PJ_LOG(4, (THIS_FILE, "OnSIAClear():success send file %s, all length=%d!", m_SFName->ptr, m_SLength));
			(file_cb.on_file_send_result)(m_SFName, PJ_SUCCESS);

			if (m_SFName != NULL)
			{
				delete[] m_SFName->ptr;
				m_SFName->ptr = NULL;
				delete[] m_SFName;
				m_SFName = NULL;
			}
			if (tag_file_cancel != NULL)
			{
				delete[] tag_file_cancel->ptr;
				tag_file_cancel->ptr = NULL;
				delete[] tag_file_cancel;
				tag_file_cancel = NULL;
			}
			if (tag_file_received != NULL)
			{
				delete[] tag_file_received->ptr;
				tag_file_received->ptr = NULL;
				delete[] tag_file_received;
				tag_file_received = NULL;
			}
			if (tag_file_sended != NULL)
			{
				delete[] tag_file_sended->ptr;
				tag_file_sended->ptr = NULL;
				delete[] tag_file_sended;
				tag_file_sended = NULL;
			}
			m_SFStep = 0;
		}
		else if (m_SFStep == 10)
		{
			m_SFStep = 0;
			m_pSteg.Configure();
			if (m_SFName&&m_SFName->slen > 0 && m_SFName->ptr)
			{
				PJ_LOG(4, (THIS_FILE, "OnSIAClear():fail send file %s, len=%d, in all length=%d!", m_SFName->ptr, m_SndLength, m_SLength));
				if (m_SLength > 0 && m_SndLength >= m_SLength)
				{
					if (m_fileS)
					{
						pj_file_close(m_fileS);
						m_fileS = NULL;
					}
					(file_cb.on_file_send_result)(m_SFName, -11);
				}
				else
				{
					if (m_fileS)
					{
						pj_file_close(m_fileS);
						m_fileS = NULL;
					}
					(file_cb.on_file_send_result)(m_SFName, -10);
				}
				if (m_SFName != NULL)
				{
					if (m_SFName->ptr != NULL)
					{
						delete[] m_SFName->ptr;
						m_SFName->ptr = NULL;
						m_SFName->slen = 0;
					}
					delete[] m_SFName;
					m_SFName = NULL;
				}
			}

			if (tag_file_cancel != NULL)
			{
				delete[] tag_file_cancel->ptr;
				tag_file_cancel->ptr = NULL;
				delete[] tag_file_cancel;
				tag_file_cancel = NULL;
			}
			if (tag_file_received != NULL)
			{
				delete[] tag_file_received->ptr;
				tag_file_received->ptr = NULL;
				delete[] tag_file_received;
				tag_file_received = NULL;
			}
			if (tag_file_sended != NULL)
			{
				delete[] tag_file_sended->ptr;
				tag_file_sended->ptr = NULL;
				delete[] tag_file_sended;
				tag_file_sended = NULL;
			}
		}
//		PJ_LOG(4, ("steg_threadTAG", "++++++++++++++++++++steg_thread():2 file send after run, ready to sleep!"));
		return 2;
	}
//	PJ_LOG(4, ("steg_threadTAG", "++++++++++++++++++++steg_thread():0 file send after run, ready to sleep!"));
	return 0;
}

pj_status_t StegFile::OnSIArrive(pj_pool_factory *pf, pj_pool_t *pool, int type)
{
	PJ_LOG(4, ("steg_threadTAG", "--------steg_thread():file arrived before run! type=%d, receive sleep time=%d(ms)", type, PJMEDIA_FILE_THREAD_DURATION_RECEIVE));
	unsigned char *Msg = new unsigned char[m_pSteg.SIADU+1];
	pj_memset(Msg, 0, sizeof(char) *m_pSteg.SIADU);
	if (type == 1)
	{
		if (m_pSteg.SLock == NULL)
		{
			return -1;
		}
		m_pSteg.lock();
		m_pSteg.bMessageArrived = false;
		UINT length = m_pSteg.Receive((void *)Msg, m_pSteg.SIADU, 1);
		PJ_LOG(4, (THIS_FILE, "OnSIArrive():receive msg len=%d", length));
		if (length > 0)
		{
			pj_str_t *msg=new pj_str_t;
			msg->ptr = new char[m_pSteg.SIADU + 1];
			pj_memset(msg->ptr, 0, sizeof(char) * m_pSteg.SIADU);
			pj_strcpy2(msg, (char *)Msg);
			PJ_LOG(4, (THIS_FILE, "OnSIArrive():receive msg, str=%s, len=%d", msg->ptr, msg->slen));
			if (tag_file_cancel&& tag_file_cancel->slen>0|| tag_file_received && tag_file_received->slen>0||tag_file_sended&&tag_file_sended->slen>0)
			{
				if (tag_file_cancel && pj_strcmp(msg, tag_file_cancel) == 0)
				{
					// case1: sender cancel or receiver cancel
					// case2: receiver not all receive for fail sending msg to sender 
					PJ_LOG(4, (THIS_FILE, "OnSIArrive():compare tag_file_cancel str=%s, len=%d", tag_file_cancel->ptr, tag_file_cancel->slen));
					if (m_RFStep>0)
					{
						m_RFStep = 10;
						m_pSteg.bFileArrived = true;
					}
					if (m_SFStep>0) {
						m_SFStep = 10;
						m_pSteg.bFileSent = true;
					}

					delete[] tag_file_cancel->ptr;
					tag_file_cancel->ptr = NULL;
					delete[] tag_file_cancel;
					tag_file_cancel = NULL;
				}
				else if (tag_file_received && pj_strcmp(msg, tag_file_received) == 0)
				{
					//receiver success for sending msg to sender
					PJ_LOG(4, (THIS_FILE, "OnSIArrive():compare tag_file_received str=%s, len=%d", tag_file_received->ptr, tag_file_received->slen));
					if (m_SFStep == 3) m_SFStep = 4;
					else m_SFStep = 10;
					m_pSteg.bFileSent = true;

					delete[] tag_file_received->ptr;
					tag_file_received->ptr = NULL;
					delete[] tag_file_received;
					tag_file_received = NULL;
				}
				else if (tag_file_sended &&pj_strcmp(msg, tag_file_sended)==0) {
					//sender success for sending msg to receiver
					if (count_receive < 0) {

						MAX_COUNT_RECEIVE =
							(PJMEDIA_FILE_THREAD_DURATION_SEND * (PJMEDIA_FILE_THREAD_DURATION_SEND >= 320)
								+ 320 * (PJMEDIA_FILE_THREAD_DURATION_SEND < 320))
							/ (m_pSteg.SIADU / m_pSteg.Harves)
							/ (PJMEDIA_FILE_THREAD_DURATION_RECEIVE * (PJMEDIA_FILE_THREAD_DURATION_RECEIVE >= 20)
								+ 20 * (PJMEDIA_FILE_THREAD_DURATION_RECEIVE < 20))
							+ 1;
						MAX_COUNT_RECEIVE *= 2;
						count_receive = 0;
					}
				}
				else {
					file_cb.on_msg_receive_result(msg);
				}
			}
			else {
				file_cb.on_msg_receive_result(msg);
			}
//			file_cb.on_msg_receive_result(msg);
			m_pSteg.length = 0;
			delete[] msg->ptr;
			delete[] msg;
		}
		m_pSteg.unlock();
		delete[]Msg;
//		PJ_LOG(4, ("steg_threadTAG", "++++++++++++++++++++steg_thread():1 file arrived after run, ready to sleep!"));
		return 1;
	}
	else if (type == 2) {
		pj_status_t status;
		if (pool == NULL)
		{
			pool = pool_file_steg;
		}
		//TODO; 2018/5/31 by BobMu 突然中断通话时直接跳显示接收失败
		if (m_pSteg.SLock == NULL)
		{
			m_RFStep = 10;
		}
		if (m_RFStep == 0)
		{
			//Step1：receive file name
			m_pSteg.lock();
			m_pSteg.bFileArrived = false;
			UINT ret = m_pSteg.Receive((void *)Msg, -1, 2);
			m_pSteg.unlock();
			m_RLength = 0;
			if (m_RFName != NULL)
			{
				if (m_RFName->ptr != NULL)
				{
					delete[] m_RFName->ptr;
					m_RFName->ptr = NULL;
					m_RFName->slen = 0;
				}
				delete[] m_RFName;
				m_RFName = NULL;
			}
			m_RFName = new pj_str_t;
			m_RFName->slen = 0;
			m_RFName->ptr = new char[m_pSteg.SIADU + 1];
			PJ_LOG(4, (THIS_FILE, "OnSIArrive():receive file name:%s, ret=%d!", Msg, ret));
			if (ret == 1)
			{
				pj_memset(m_RFName->ptr, 0, m_pSteg.SIADU * sizeof(char));
				pj_strcpy2(m_RFName, (const char *)Msg);
				if (m_RFPath != NULL)
				{
					if (m_RFPath->ptr != NULL)
					{
						delete[] m_RFPath->ptr;
						m_RFPath->ptr = NULL;
						m_RFPath->slen = 0;
					}
					delete[] m_RFPath;
					m_RFPath = NULL;
				}
				m_RFPath = new pj_str_t;				
				PJ_LOG(4, (THIS_FILE, "OnSIArrive():RFRoot:%s!", thread_id_RFRoot->ptr));
				PJ_LOG(4, (THIS_FILE, "OnSIArrive():m_RFName:%s!", m_RFName->ptr));
				m_RFPath->slen = (thread_id_RFRoot->slen) + (m_RFName->slen)+1;
				m_RFPath->ptr = new char[m_RFPath->slen];
				pj_memset(m_RFPath->ptr, 0, m_RFPath->slen);
				pj_memcpy(m_RFPath->ptr, thread_id_RFRoot->ptr, thread_id_RFRoot->slen);
				pj_memcpy(m_RFPath->ptr + thread_id_RFRoot->slen, m_RFName->ptr, m_RFName->slen);
				PJ_LOG(4, (THIS_FILE, "OnSIArrive():m_RFPath:%s!", m_RFPath->ptr));
				status = pj_file_open(pool, m_RFPath->ptr, PJ_O_WRONLY, &m_fileR);
				PJ_LOG(4, (THIS_FILE, "OnSIArrive():open file status=%d!", status));
				if (status==PJ_SUCCESS)
				{
					m_RFStep = 1;
				}
				else if(m_RFStep == 0){
					m_RFStep = 0;
					cancelFileTranslating(m_RFName);
					(file_cb.on_file_receive_result)(m_RFName, -status);
				}
				//todo: new file tags on receiver
				if (tag_file_cancel != NULL)
				{
					delete[] tag_file_cancel->ptr;
					tag_file_cancel->ptr = NULL;
					delete[] tag_file_cancel;
					tag_file_cancel = NULL;
				}
				tag_file_cancel = new pj_str_t;
				tag_file_cancel->ptr = new char[m_RFName->slen + 3 + 1];
				pj_strcpy(tag_file_cancel, m_RFName);
				pj_strcat2(tag_file_cancel, LAST_TAG_FILE_CANCEL);
				tag_file_cancel->ptr[tag_file_cancel->slen] = LAST_END_STR;

				if (tag_file_received != NULL)
				{
					delete[] tag_file_received->ptr;
					tag_file_received->ptr = NULL;
					delete[] tag_file_received;
					tag_file_received = NULL;
				}
				tag_file_received = new pj_str_t;
				tag_file_received->ptr = new char[m_RFName->slen + 3 + 1];
				pj_strcpy(tag_file_received, m_RFName);
				pj_strcat2(tag_file_received, LAST_TAG_FILE_RECEIVED_REPLY);
				tag_file_received->ptr[tag_file_received->slen] = LAST_END_STR;

				if (tag_file_sended != NULL)
				{
					delete[] tag_file_sended->ptr;
					tag_file_sended->ptr = NULL;
					delete[] tag_file_sended;
					tag_file_sended = NULL;
				}
				tag_file_sended = new pj_str_t;
				tag_file_sended->ptr = new char[m_RFName->slen + 3 + 1];
				pj_strcpy(tag_file_sended, m_RFName);
				pj_strcat2(tag_file_sended, LAST_TAG_FILE_SENDED_REQUEST);
				tag_file_sended->ptr[tag_file_sended->slen] = LAST_END_STR;

				PJ_LOG(4, (THIS_FILE, "OnSIArrive():create file end tag str=%s, slen=%d; file received tag=%s, slen=%d; file sended str=%s, slen=%d!", tag_file_cancel->ptr, tag_file_cancel->slen, tag_file_received->ptr, tag_file_received->slen, tag_file_sended->ptr, tag_file_sended->slen));

			}
		}
		else if (m_RFStep == 1)
		{
			//Step2: receive length of file
			UINT TLength;
			m_pSteg.lock();
			m_pSteg.bFileArrived = false;
			UINT ret = m_pSteg.Receive((void *)&TLength, -2, 2);
			m_pSteg.unlock();
			PJ_LOG(4, (THIS_FILE, "OnSIArrive():receive file length=%d, ret=%d!", TLength, ret));
			if (ret == 2)
			{
				m_RFStep = 2;
				m_RLength = TLength;
				m_RcvLength = 0;
				(file_cb.on_file_receive_progress)(m_RFName, m_RLength, float(m_RcvLength) / float(m_RLength));
			}
			else if(m_RFStep == 1)
			{
				m_RFStep = 0;
				if (m_fileR)
				{
					pj_file_close(m_fileR);
					m_fileR = NULL;
				}
				(file_cb.on_file_receive_result)(m_RFName, -2);
				printf("2 error!\r\n");
			}
		}
		else if (m_RFStep == 2)
		{
			//Step3: receive file process
			m_pSteg.lock();
			m_pSteg.bFileArrived = false;
			pj_ssize_t length = m_pSteg.Receive((void *)Msg, m_RLength - m_RcvLength, 2);
			m_pSteg.unlock();
			pj_status_t status = pj_file_write(m_fileR, Msg, &length);
			if (status != PJ_SUCCESS)
			{
				if (m_fileR)
				{
					pj_file_close(m_fileR);
					m_fileR = NULL;
				}
				m_RFStep = 0;
				(file_cb.on_file_receive_result)(m_RFName, -3);
			}
			pj_file_flush(m_fileR);
			m_RcvLength += length;
			PJ_LOG(4, (THIS_FILE, "OnSIArrive():step2 receive file this len=%d, have received length=%d, in all length=%d!", length, m_RcvLength, m_RLength));
			(file_cb.on_file_receive_progress)(m_RFName, m_RLength, float(m_RcvLength) / float(m_RLength));
		
			if (count_receive >= 0 && m_RcvLength > 0)
			{
				count_receive = 0;
			}
			if (m_RcvLength >= m_RLength) {
				//receive success, reply msg to sender
				if (m_fileR)
				{
					pj_file_close(m_fileR);
					m_fileR = NULL;
				}
				m_RFStep = 0;
				count_receive = -1;

				pj_str_t * file_received_tag = new pj_str_t;
				file_received_tag->ptr = new char[m_RFName->slen + 3 + 1];
				pj_strcpy(file_received_tag, m_RFName);
				pj_strcat2(file_received_tag, LAST_TAG_FILE_RECEIVED_REPLY);
				file_received_tag->ptr[file_received_tag->slen] = LAST_END_STR;
				sendMsg(file_received_tag);
				delete[] file_received_tag->ptr;
				file_received_tag->ptr = NULL;
				file_received_tag->slen = 0;
				delete[] file_received_tag;
				file_received_tag = NULL;

				(file_cb.on_file_receive_result)(m_RFPath, PJ_SUCCESS);
				if (tag_file_cancel != NULL)
				{
					delete[] tag_file_cancel->ptr;
					tag_file_cancel->ptr = NULL;
					delete[] tag_file_cancel;
					tag_file_cancel = NULL;
				}
				if (m_RFName != NULL)
				{
					if (m_RFName->ptr != NULL)
					{
						delete[] m_RFName->ptr;
						m_RFName->ptr = NULL;
						m_RFName->slen = 0;
					}
					delete[] m_RFName;
					m_RFName = NULL;
				}
				PJ_LOG(4, (THIS_FILE, "OnSIArrive():success receive file:%s!", m_RFPath->ptr));
			}
		}
		else if (m_RFStep == 10)
		{
			//Step10: get error
			m_RFStep = 0;
			m_pSteg.Configure();
			if (m_RFName && m_RFName->slen > 0 && m_RFName->ptr)
			{
				PJ_LOG(4, (THIS_FILE, "OnSIArrive():fail receive file:%s!", m_RFName->ptr));
				if (m_RLength > 0 && m_RcvLength >= m_RLength)
				{
					if (m_fileR)
					{
						pj_file_close(m_fileR);
						m_fileR = NULL;
					}
					(file_cb.on_file_receive_result)(m_RFPath, -11);
				}
				else
				{
					if (m_fileR)
					{
						pj_file_close(m_fileR);
						m_fileR = NULL;
					}
					(file_cb.on_file_receive_result)(m_RFName, -10);
				}
				if (tag_file_cancel != NULL)
				{
					delete[] tag_file_cancel->ptr;
					tag_file_cancel->ptr = NULL;
					delete[] tag_file_cancel;
					tag_file_cancel = NULL;
				}
				if (m_RFName != NULL)
				{
					if (m_RFName->ptr != NULL)
					{
						delete[] m_RFName->ptr;
						m_RFName->ptr = NULL;
						m_RFName->slen = 0;
					}
					delete[] m_RFName;
					m_RFName = NULL;
				}
			}
		}
		delete[] Msg;
//		PJ_LOG(4, ("steg_threadTAG", "++++++++++++++++++++steg_thread():2 file arrived after run, ready to sleep!"));
		return 2;
	}
	if (Msg) delete[]Msg;
//	PJ_LOG(4, ("steg_threadTAG", "++++++++++++++++++++steg_thread():0 file arrived after run, ready to sleep!"));
	return 0;
}

void StegFile::OnSenderTimer() {
	if (count_send_wait_reply>=0)
	{
		count_send_wait_reply++;
//		PJ_LOG(4, (THIS_FILE, "OnSIAClear(): in step 3 count_send_wait_reply=%d, max=%d!", count_send_wait_reply, MAX_COUNT_SEND_WAIT_REPLY));
		if (count_send_wait_reply > MAX_COUNT_SEND_WAIT_REPLY)
		{
			count_send_wait_reply = -1;
			if (m_SFStep != 10)
			{
				m_SFStep = 10;
				m_pSteg.bFileSent = true;
			}
		}
	}

}

void StegFile::OnReceiverTimer() {

	if (count_receive >= 0)
	{
		count_receive++;
//		PJ_LOG(4, (THIS_FILE, "OnSIArrive():tag_file_sended received: empty_count=%d, max_count=%d!", count_receive, MAX_COUNT_RECEIVE));
		if (count_receive > MAX_COUNT_RECEIVE)
		{
			count_receive = -1;
			if (m_RFName)
			{
				cancelFileTranslating(m_RFName);
			}
		}
	}
	
}

pj_status_t StegFile::thread_start(pj_str_t *receiveFileRoot) {
//	PJ_LOG(5, (THIS_FILE, "thread_start():thread of file has root:%s!", receiveFileRoot->ptr));
	

	if (thread_id_RFRoot&&pj_strcmp(thread_id_RFRoot, receiveFileRoot))
	{
		if (thread_id_RFRoot->ptr!=NULL)
		{
			delete[] thread_id_RFRoot->ptr;
			thread_id_RFRoot->ptr = NULL;
			thread_id_RFRoot->slen = 0;
		}
		delete[] thread_id_RFRoot;
		thread_id_RFRoot = NULL;
	}
	if (!thread_id_RFRoot)
	{
		thread_id_RFRoot = new pj_str_t;
		thread_id_RFRoot->ptr = new char[receiveFileRoot->slen + 1];
		pj_strcpy(thread_id_RFRoot, receiveFileRoot);
		thread_id_RFRoot->ptr[thread_id_RFRoot->slen] = LAST_END_STR;
	}

	if (!pool_file_steg)
	{
//		PJ_LOG(5, (THIS_FILE, "thread_start():thread of file start fail, null pool!"));
		return -1;
	}
//	PJ_LOG(5, (THIS_FILE, "thread_start():thread of file is ready!"));
	pj_status_t status = -1;
//	status = pj_thread_create(pool_file_steg, "thread_file_steg", &steg_thread, this, 0, 0, &thread_file_steg);
	status = pj_thread_create(pool_file_steg, "thread_file_steg_receive", &steg_thread_receive, this, 0, 0, &thread_file_steg_receive);
	status += pj_thread_create(pool_file_steg, "thread_file_steg_send", &steg_thread_send, this, 0, 0, &thread_file_steg_send);


//	PJ_LOG(4, (THIS_FILE, "thread_start():thread of file started result statu=%d", status));
	return status;
}

pj_status_t StegFile::thread_stop(pj_str_t *receiveFileRoot) {
	if (thread_id_RFRoot && receiveFileRoot&&pj_strcmp(receiveFileRoot, thread_id_RFRoot) == 0) {
		if (thread_id_RFRoot->ptr != NULL)
		{
			delete[] thread_id_RFRoot->ptr;
			thread_id_RFRoot->ptr = NULL;
			thread_id_RFRoot->slen = 0;
		}
		delete[] thread_id_RFRoot;
		thread_id_RFRoot = NULL;
		
		if (thread_file_steg_send) {
//			pj_thread_join(thread_file_steg_send);
			pj_thread_destroy(thread_file_steg_send);
			thread_file_steg_send = NULL;
		}
		if (thread_file_steg_receive) {
//			pj_thread_join(thread_file_steg_receive);
			pj_thread_destroy(thread_file_steg_receive);
			thread_file_steg_receive = NULL;
			return PJ_SUCCESS;
		}
		
		/*

		if (pool_file_steg) {
			pj_pool_reset(pool_file_steg);
			return PJ_SUCCESS;
		}

			if (pj_thread_join(thread_file_steg) == PJ_SUCCESS) {
			pj_thread_destroy(thread_file_steg);
			thread_file_steg = NULL;
			pj_pool_reset(pool_file_steg);
			return PJ_SUCCESS;
			}
		*/

	}
	return PJ_FALSE;
}

void StegFile::set_file_cb(FileCallback cb)
{
	file_cb = cb;
}

void StegFile::releaseCaching()
{
	m_SFStep = 0;
	m_RFStep = 0;
	m_SLength = 0;
	m_SndLength = 0;
	m_RLength = 0;
	m_RcvLength = 0;
	if (m_SFPath != NULL)
	{
		if (m_SFPath->ptr != NULL)
		{
			delete[] m_SFPath->ptr;
			m_SFPath->ptr = NULL;
			m_SFPath->slen = 0;
		}
		delete[] m_SFPath;
		m_SFPath = NULL;
	}
	if (m_SFName != NULL)
	{
		if (m_SFName->ptr != NULL)
		{
			delete[] m_SFName->ptr;
			m_SFName->ptr = NULL;
			m_SFName->slen = 0;
		}
		delete[] m_SFName;
		m_SFName = NULL;
	}
	if (m_RFPath != NULL)
	{
		if (m_RFPath->ptr != NULL)
		{
			delete[] m_RFPath->ptr;
			m_RFPath->ptr = NULL;
		}
		m_RFPath->slen = 0;
		delete[] m_RFPath;
		m_RFPath = NULL;
	}
	if (m_RFName != NULL)
	{
		if (m_RFName->ptr != NULL)
		{
			delete[] m_RFName->ptr;
			m_RFName->ptr = NULL;
			m_RFName->slen = 0;
		}
		delete[] m_RFName;
		m_RFName = NULL;
	}

	
	if (pool_file_steg) {
		pool_file_steg = NULL;
	}
}

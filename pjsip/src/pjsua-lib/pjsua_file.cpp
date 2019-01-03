#include <pjsua-lib/pjsua.h>
#include <pjsua-lib/pjsua_internal.h>
#include <pjmedia/steg_file.h>

#define THIS_FILE "pjsua_file.cpp"

StegFile m_StegFile;
FileCallback fcb;

static void on_file_receive_progress(const pj_str_t *fName, unsigned int len, float p)
{
	PJ_LOG(4, (THIS_FILE, "CallBack:on_file_receive_progress"));
	if (pjsua_var.ua_cfg.cb.on_pjmedia_file_receive_progress) {
		(*pjsua_var.ua_cfg.cb.on_pjmedia_file_receive_progress)(fName, len, p);
	}
}

static void on_file_receive_result(const pj_str_t *fName, int status)
{
	PJ_LOG(4, (THIS_FILE, "CallBack:on_file_receive_result"));
	if (pjsua_var.ua_cfg.cb.on_pjmedia_file_receive_result) {
		(*pjsua_var.ua_cfg.cb.on_pjmedia_file_receive_result)(fName, status);
	}
}

static void on_file_send_progress(const pj_str_t *fName, unsigned int len, float p)
{
	PJ_LOG(4, (THIS_FILE, "CallBack:on_file_send_progress"));
	if (pjsua_var.ua_cfg.cb.on_pjmedia_file_send_progress) {
		(*pjsua_var.ua_cfg.cb.on_pjmedia_file_send_progress)(fName, len, p);
	}
}

static void on_file_send_result(const pj_str_t *fName, int status)
{
	PJ_LOG(4, (THIS_FILE, "CallBack:on_file_send_result"));
	if (pjsua_var.ua_cfg.cb.on_pjmedia_file_send_result) {
		(*pjsua_var.ua_cfg.cb.on_pjmedia_file_send_result)(fName, status);
	}
}

static void on_msg_receive_result(const pj_str_t *msg)
{
	PJ_LOG(4, (THIS_FILE, "CallBack:on_msg_receive_result"));
	if (pjsua_var.ua_cfg.cb.on_pjmedia_msg_receive_result) {
		(*pjsua_var.ua_cfg.cb.on_pjmedia_msg_receive_result)(msg);
	}
}

static void on_msg_send_result(int status)
{
	PJ_LOG(4, (THIS_FILE, "CallBack:on_msg_send_result"));
	if (pjsua_var.ua_cfg.cb.on_pjmedia_msg_send_result) {
		(*pjsua_var.ua_cfg.cb.on_pjmedia_msg_send_result)(status);
	}
}

PJ_DEF(void) pjsua_file_init(pj_pool_t *pool)
{
	PJ_LOG(4, (THIS_FILE, "pjsua_file_init"));
	m_StegFile.initStegFile(pool);
	if (&fcb)
	{
		fcb.on_file_receive_progress = &on_file_receive_progress;
		fcb.on_file_receive_result = &on_file_receive_result;
		fcb.on_file_send_progress = &on_file_send_progress;
		fcb.on_file_send_result = &on_file_send_result;
		fcb.on_msg_receive_result = &on_msg_receive_result;
		fcb.on_msg_send_result = &on_msg_send_result;
		m_StegFile.set_file_cb(fcb);
	}
}

PJ_DEF(pj_status_t) pjsua_steg_send_file(pj_str_t *filepath, pj_str_t *filename) {
	PJ_LOG(4, (THIS_FILE, "pjsua_steg_send_file:%s", filename->ptr));
	return m_StegFile.sendFile(filepath, filename);
}

PJ_DEF(pj_status_t) pjsua_steg_is_file_sending() {
	pj_status_t s = m_StegFile.isFileSending();
//	PJ_LOG(5, (THIS_FILE, "pjsua_steg_is_file_sending s=%d", s));
	return s;
}

PJ_DEF(pj_status_t) pjsua_steg_is_file_receiving() {
	pj_status_t s = m_StegFile.isFileReceiving();
//	PJ_LOG(5, (THIS_FILE, "pjsua_steg_is_file_receiving s=%d", s));
	return s;
}

PJ_DEF(pj_status_t) pjsua_steg_cancel_file_translate(pj_str_t *filename) {
	PJ_LOG(4, (THIS_FILE, "pjsua_steg_cancel_send_file:%s, len=%d", filename->ptr, filename->slen));
	return m_StegFile.cancelFileTranslating(filename);
}

PJ_DEF(pj_status_t) pjsua_steg_send_msg(pj_str_t *strmsg) {
	PJ_LOG(4, (THIS_FILE, "pjsua_steg_send_msg:%s, len=%d", strmsg->ptr, (int)strmsg->slen));
	pj_status_t status= m_StegFile.sendMsg(strmsg);
	PJ_LOG(4, (THIS_FILE, "pjsua_steg_send_msg status:%d", status));
	return status;
}

PJ_DEF(pj_status_t) pjsua_steg_file_thread_start(pj_str_t *receiveFileRoot) {
//	PJ_LOG(5, (THIS_FILE, "pjsua_steg_file_thread_start root:%s",receiveFileRoot->ptr));
	return m_StegFile.thread_start(receiveFileRoot);
}

PJ_DEF(pj_status_t) pjsua_steg_file_thread_stop(pj_str_t *receiveFileRoot) {
//	PJ_LOG(4, (THIS_FILE, "pjsua_steg_file_thread_stop"));
	return m_StegFile.thread_stop(receiveFileRoot);
}

PJ_DEF(pj_size_t) pjsua_steg_file_get_max_len() {
	PJ_LOG(4, (THIS_FILE, "pjsua_steg_file_get_max_len"));
	return m_StegFile.getMaxSIADU();
}



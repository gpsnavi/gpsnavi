/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

/*
 * pthread_msg.c
 *
 *  Created on: 2013/11/15
 *      Author:t.aikawa
 */

#include "pthread_msq.h"

/**
 * メッセージキューの作成
 */
int pthread_msq_create(pthread_msq_id_t *queue, int qsize) {
#ifdef __SMS_APPLE__
#else
	sem_t *sendId;
	sem_t *receiveId;
#endif /* __SMS_APPLE__ */
	pthread_mutex_t *mutex;
#ifdef __SMS_APPLE__
#else
	int rc;
#endif /* __SMS_APPLE__ */
	size_t size;
	pthread_msq_msg_t *ringBuffer;

	if (NULL != queue->oneself) {
		return (PTHREAD_MSQ_ERROR);
	}
#ifdef __SMS_APPLE__
#else
	sendId = &queue->sendId;
	receiveId = &queue->receiveId;
#endif
	mutex = &queue->mutex;

#ifdef __SMS_APPLE__
	queue->sendId = sem_open(queue->sendName, O_CREAT, S_IRWXU, qsize);
	if (SEM_FAILED == queue->sendId) {
#else
	rc = sem_init(sendId, 0, qsize);
	if (0 != rc) {
#endif /* __SMS_APPLE__ */
		return (PTHREAD_MSQ_ERROR);
	}

#ifdef __SMS_APPLE__
	queue->receiveId = sem_open(queue->receiveName, O_CREAT, S_IRWXU, 0);
	if (SEM_FAILED == queue->receiveId) {
#else
	rc = sem_init(receiveId, 0, 0);
	if (0 != rc) {
		sem_destroy(sendId); /* セマフォを削除する */
#endif /* __SMS_APPLE__ */
		return (PTHREAD_MSQ_ERROR);
	}

	pthread_mutex_init(mutex, NULL);

	size = sizeof(pthread_msq_msg_t) * qsize;
	ringBuffer = malloc(size);
	if (NULL == ringBuffer) {
#ifdef __SMS_APPLE__
		sem_close(queue->sendId);
		sem_unlink(queue->sendName);
		sem_close(queue->receiveId);
		sem_unlink(queue->receiveName);
#else
		sem_destroy(sendId); /* セマフォを削除する */
		sem_destroy(receiveId); /* セマフォを削除する */
#endif /* __SMS_APPLE__ */
		pthread_mutex_destroy(mutex);
		return (PTHREAD_MSQ_ERROR);
	}
	queue->oneself = queue;
	queue->maxMsgQueueNum = qsize;
	queue->fifoIndex = 0;
	queue->queueNum = 0;
	queue->ringBuffer = ringBuffer;

	return (PTHREAD_MSQ_OK);
}

/**
 * メッセージ送信
 */
int pthread_msq_msg_send(pthread_msq_id_t *queue, pthread_msq_msg_t *msg, void *sender) {
	pthread_msq_msg_t *msq_msg;
	int fifo;
	int i;
	size_t *in, *out;

	/* メッセージキューIDのチェック */
	if (queue->oneself != queue) {
		return (PTHREAD_MSQ_ERROR);
	}

#ifdef __SMS_APPLE__
	sem_wait(queue->sendId); /* 送信可能になるまで待つ */
#else
	sem_wait(&queue->sendId); /* 送信可能になるまで待つ */
#endif /* __SMS_APPLE__ */
	pthread_mutex_lock(&queue->mutex);

	/* リングバッファー内のメッセージ格納位置を求める */
	fifo = queue->fifoIndex + queue->queueNum;
	if (fifo >= queue->maxMsgQueueNum) {
		//fifo = 0;
		fifo = fifo % queue->maxMsgQueueNum;
	}
	msq_msg = queue->ringBuffer + fifo;

	/* リングバッファーにメッセージを格納する */
	msq_msg->__sender = sender;
	out = msq_msg->data;
	in = msg->data;
	for (i = 0; i < PTHREAD_MSQ_MSG_NUM; i++) {
		*out++ = *in++;
	}

	/* メッセージの格納数を＋１する */
	queue->queueNum++;

	pthread_mutex_unlock(&queue->mutex);

#ifdef __SMS_APPLE__
	sem_post(queue->receiveId); /* 受信を許可する */
#else
	sem_post(&queue->receiveId); /* 受信を許可する */
#endif /* __SMS_APPLE__ */
	return (PTHREAD_MSQ_OK);
}

/**
 * メッセージ受信
 */
int pthread_msq_msg_receive(pthread_msq_id_t *queue, pthread_msq_msg_t *msg) {
	pthread_msq_msg_t *msq_msg;
	int i;
	size_t *in, *out;

	/* メッセージキューIDのチェック */
	if (queue->oneself != queue) {
		return (PTHREAD_MSQ_ERROR);
	}

#ifdef __SMS_APPLE__
	sem_wait(queue->receiveId); /* 受信可能になるまで待つ */
#else
	sem_wait(&queue->receiveId); /* 受信可能になるまで待つ */
#endif /* __SMS_APPLE__ */
	pthread_mutex_lock(&queue->mutex);

	/* リングバッファー内のメッセージ取り出し位置を求める */
	msq_msg = queue->ringBuffer + queue->fifoIndex;

	/* リングバッファーからメッセージを取り出す */
	msg->__sender = msq_msg->__sender;
	out = msg->data;
	in = msq_msg->data;
	for (i = 0; i < PTHREAD_MSQ_MSG_NUM; i++) {
		*out++ = *in++;
	}

	/* 次のメッセージ取り出し位置を求める */
	if (++queue->fifoIndex >= queue->maxMsgQueueNum) {
		queue->fifoIndex = 0;
	}
	/* メッセージの格納数をー１する */
	--queue->queueNum;

	pthread_mutex_unlock(&queue->mutex);
#ifdef __SMS_APPLE__
	sem_post(queue->sendId); /* 送信を許可する */
#else
	sem_post(&queue->sendId); /* 送信を許可する */
#endif /* __SMS_APPLE__ */
	return (PTHREAD_MSQ_OK);
}

/**
 * メッセージ受信(wait無し)
 */
int pthread_msq_msg_receive_try(pthread_msq_id_t *queue, pthread_msq_msg_t *msg) {
	pthread_msq_msg_t *msq_msg;
	int i, ret;
	size_t *in, *out;

	/* メッセージキューIDのチェック */
	if (queue->oneself != queue) {
		return (PTHREAD_MSQ_ERROR);
	}

#ifdef __SMS_APPLE__
	ret = sem_trywait(queue->receiveId); /* 受信可能ならば処理 */
#else
	ret = sem_trywait(&queue->receiveId); /* 受信可能ならば処理 */
#endif /* __SMS_APPLE__ */
	if (-1 == ret) {
		// msg無し
		return (PTHREAD_MSQ_ERROR);
	}
	pthread_mutex_lock(&queue->mutex);

	/* リングバッファー内のメッセージ取り出し位置を求める */
	msq_msg = queue->ringBuffer + queue->fifoIndex;

	/* リングバッファーからメッセージを取り出す */
	msg->__sender = msq_msg->__sender;
	out = msg->data;
	in = msq_msg->data;
	for (i = 0; i < PTHREAD_MSQ_MSG_NUM; i++) {
		*out++ = *in++;
	}

	/* 次のメッセージ取り出し位置を求める */
	if (++queue->fifoIndex >= queue->maxMsgQueueNum) {
		queue->fifoIndex = 0;
	}
	/* メッセージの格納数をー１する */
	--queue->queueNum;

	pthread_mutex_unlock(&queue->mutex);
#ifdef __SMS_APPLE__
	sem_post(queue->sendId); /* 送信を許可する */
#else
	sem_post(&queue->sendId); /* 送信を許可する */
#endif /* __SMS_APPLE__ */

	return (PTHREAD_MSQ_OK);
}

/**
 * メッセージキューの破壊
 */
int pthread_msq_destroy(pthread_msq_id_t *queue) {
#ifdef __SMS_APPLE__
	sem_close(queue->sendId);
	sem_unlink(queue->sendName);
	sem_close(queue->receiveId);
	sem_unlink(queue->receiveName);
#else
	sem_destroy(&queue->sendId); /* セマフォを削除する */
	sem_destroy(&queue->receiveId); /* セマフォを削除する */
#endif /* __SMS_APPLE__ */
	pthread_mutex_destroy(&queue->mutex); /* ミューテックスを破壊する */
	free(queue->ringBuffer);
	queue->oneself = NULL;
	return (PTHREAD_MSQ_OK);
}

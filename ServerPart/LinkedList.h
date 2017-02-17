#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include <stdio.h>
#include<WinSock2.h>
#include"Info.h"

struct chattinglog
{
	char *item;
	chattinglog *pNext;
};
struct LogQueueType
{
	chattinglog *pFirst;
	chattinglog *pLast;
};
struct ConnectedUser
{
	int nLoginCheck;
	char sID[11];
	char sNickname[21];
	int nCurrent;
	SOCKET sock;
	SOCKADDR_IN sockaddr;
	int nLogcnt;
	LogQueueType *pLog;
};

typedef SOCKET element; // char형을 연결 큐 element의 자료형으로 정의

typedef struct QNode{ // 연결 큐의 노드를 구조체로 정의
	struct QNode *prevlink;
	ConnectedUser *Client;
	element data;
	struct QNode *link;
} QNode;

typedef struct{ // 연결 큐에서 사용하는 포인터 front와 rear를 구조체로 정의
	QNode *front, *rear;
	char buf[100];
	char sRecvNick[21];
	char sRecvID[11];
	int buflen;
} LQueueType;

void enQueueLogClient(ConnectedUser *pClient, char* item)
{
	LogQueueType *pLog = pClient->pLog;
	chattinglog *newLog = (chattinglog*)malloc(sizeof(chattinglog));
	newLog->item = item;
	newLog->pNext = NULL;
	if (pClient->nLogcnt < 40)
	{
		if (pLog->pFirst == NULL)
		{
			pLog->pFirst = newLog;
			pLog->pLast = newLog;
		}
		else{
			pLog->pLast->pNext = newLog;
			pLog->pLast = newLog;
		}
	}
	else
	{
		pLog->pLast->pNext = newLog;
		pLog->pLast = newLog;
		chattinglog *temp;
		temp = pLog->pFirst;
		pLog->pFirst = pLog->pFirst->pNext;
		temp->pNext = NULL;
		free(temp);
	}
	pClient->nLogcnt++;
}
void deleteLogClient(ConnectedUser* pClient)
{
	LogQueueType *pLog = pClient->pLog;
	chattinglog *pTemp = pLog->pFirst;
	chattinglog *pPrev;
	printf("로그가 잘 삭제되는지 봅시다 %d->", pClient->nLogcnt);
	while (pTemp)
	{
		pPrev = pTemp;
		pTemp = pTemp->pNext;
		free(pPrev);
		pClient->nLogcnt--;
	}
	printf("%d\n", pClient->nLogcnt);

}

LQueueType *createLinkedQueue() // 공백 연결 큐 생성 연산
{
	LQueueType *LQ;
	LQ = (LQueueType *)malloc(sizeof(LQueueType));
	LQ->front = NULL;
	LQ->rear = NULL;
	return LQ;
}
int isEmpty(LQueueType *LQ) // 연결 큐가 공백인지 확인하는 연산
{
	if (LQ->front == NULL) {
		printf("\n Linked Queue is empty! \n");
		return 1;
	}
	else return 0;
}
void enQueue(LQueueType *LQ, ConnectedUser *item)
// 연결큐의 rear에원소를삽입하는 연산
{
	QNode *newNode = (QNode *)malloc(sizeof(QNode));
	newNode->Client = item;
	newNode->link = NULL;
	if (LQ->front == NULL) { // 현재 연결 큐가 공백인 경우//엠프티
		LQ->front = newNode;
		LQ->rear = newNode;
		newNode->prevlink = NULL;
	}
	else { // 현재 연결 큐가 공백이 아닌 경우
		newNode->prevlink = LQ->rear;
		LQ->rear->link = newNode;
		LQ->rear = newNode;
	}
}
/*
void enQueue(LQueueType *LQ, element item)
// 연결큐의 rear에원소를삽입하는 연산
{
	QNode *newNode = (QNode *)malloc(sizeof(QNode));
	newNode->data = item;
	newNode->link = NULL;
	if (LQ->front == NULL) { // 현재 연결 큐가 공백인 경우//엠프티
		LQ->front = newNode;
		LQ->rear = newNode;
	}
	else { // 현재 연결 큐가 공백이 아닌 경우
		LQ->rear->link = newNode;
		LQ->rear = newNode;
	}
}*/
element deQueue(LQueueType *LQ)
// 연결큐에서 front 원소를 삭제하고 반환하는 연산
{
	QNode *old = LQ->front;
	element item;
	if (isEmpty(LQ)) return 0;
	else {
		item = old->data;//리턴하기위해
		LQ->front = LQ->front->link;
		if (LQ->front == NULL)
			LQ->rear = NULL;
		free(old);//메모리해제
		return item;
	}
}
void exitRoom(LQueueType *LQ, ConnectedUser *item)
{

	QNode *temp = LQ->front;
	QNode *pre;
	while (temp)
	{
		if (!strcmp(temp->Client->sID, item->sID))
		{//같다면0임
			if (temp == LQ->front) //처음거라면 front를 뒤로
			{
				LQ->front = LQ->front->link;
				if (LQ->front == NULL)//그런데 다음게 없다면
					LQ->rear = NULL;//즉1개만있을때
				free(temp);
				break;
			}
			else if(temp == LQ->rear){
				LQ->rear = LQ->rear->prevlink;
				LQ->rear->link = NULL;
				free(temp);
				break;
			}
			else
			{
				pre = temp->prevlink;
				pre->link = temp->link;
				temp->link->prevlink = pre;
				free(temp);
				break;
			}
		}
		temp = temp->link;
	}
	
}
int del(LQueueType *LQ) // 연결 큐에서 front 원소를 삭제하는 연산
{
	QNode *old = LQ->front;
	if (isEmpty(LQ)) return 0;
	else {
		LQ->front = LQ->front->link;
		if (LQ->front == NULL)
			LQ->rear = NULL;
		free(old);
		return 1;
	}
}

element peek(LQueueType *LQ)
// 연결 큐에서 front 원소를 검색하여 반환하는 연산
{
	element item;
	if (isEmpty(LQ)) return 0;
	else {
		item = LQ->front->data;
		return item;
	}
}

void printLQ(LQueueType *LQ) // 연결 큐를 출력하는 연산
{
	QNode *temp = LQ->front;
	printf(" Linked Queue : [");
	while (temp) {
		printf("%3c", temp->data);
		temp = temp->link;
	}
	printf(" ] \n");
}

#endif
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

typedef SOCKET element; // char���� ���� ť element�� �ڷ������� ����

typedef struct QNode{ // ���� ť�� ��带 ����ü�� ����
	struct QNode *prevlink;
	ConnectedUser *Client;
	element data;
	struct QNode *link;
} QNode;

typedef struct{ // ���� ť���� ����ϴ� ������ front�� rear�� ����ü�� ����
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
	printf("�αװ� �� �����Ǵ��� ���ô� %d->", pClient->nLogcnt);
	while (pTemp)
	{
		pPrev = pTemp;
		pTemp = pTemp->pNext;
		free(pPrev);
		pClient->nLogcnt--;
	}
	printf("%d\n", pClient->nLogcnt);

}

LQueueType *createLinkedQueue() // ���� ���� ť ���� ����
{
	LQueueType *LQ;
	LQ = (LQueueType *)malloc(sizeof(LQueueType));
	LQ->front = NULL;
	LQ->rear = NULL;
	return LQ;
}
int isEmpty(LQueueType *LQ) // ���� ť�� �������� Ȯ���ϴ� ����
{
	if (LQ->front == NULL) {
		printf("\n Linked Queue is empty! \n");
		return 1;
	}
	else return 0;
}
void enQueue(LQueueType *LQ, ConnectedUser *item)
// ����ť�� rear�����Ҹ������ϴ� ����
{
	QNode *newNode = (QNode *)malloc(sizeof(QNode));
	newNode->Client = item;
	newNode->link = NULL;
	if (LQ->front == NULL) { // ���� ���� ť�� ������ ���//����Ƽ
		LQ->front = newNode;
		LQ->rear = newNode;
		newNode->prevlink = NULL;
	}
	else { // ���� ���� ť�� ������ �ƴ� ���
		newNode->prevlink = LQ->rear;
		LQ->rear->link = newNode;
		LQ->rear = newNode;
	}
}
/*
void enQueue(LQueueType *LQ, element item)
// ����ť�� rear�����Ҹ������ϴ� ����
{
	QNode *newNode = (QNode *)malloc(sizeof(QNode));
	newNode->data = item;
	newNode->link = NULL;
	if (LQ->front == NULL) { // ���� ���� ť�� ������ ���//����Ƽ
		LQ->front = newNode;
		LQ->rear = newNode;
	}
	else { // ���� ���� ť�� ������ �ƴ� ���
		LQ->rear->link = newNode;
		LQ->rear = newNode;
	}
}*/
element deQueue(LQueueType *LQ)
// ����ť���� front ���Ҹ� �����ϰ� ��ȯ�ϴ� ����
{
	QNode *old = LQ->front;
	element item;
	if (isEmpty(LQ)) return 0;
	else {
		item = old->data;//�����ϱ�����
		LQ->front = LQ->front->link;
		if (LQ->front == NULL)
			LQ->rear = NULL;
		free(old);//�޸�����
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
		{//���ٸ�0��
			if (temp == LQ->front) //ó���Ŷ�� front�� �ڷ�
			{
				LQ->front = LQ->front->link;
				if (LQ->front == NULL)//�׷��� ������ ���ٸ�
					LQ->rear = NULL;//��1����������
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
int del(LQueueType *LQ) // ���� ť���� front ���Ҹ� �����ϴ� ����
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
// ���� ť���� front ���Ҹ� �˻��Ͽ� ��ȯ�ϴ� ����
{
	element item;
	if (isEmpty(LQ)) return 0;
	else {
		item = LQ->front->data;
		return item;
	}
}

void printLQ(LQueueType *LQ) // ���� ť�� ����ϴ� ����
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
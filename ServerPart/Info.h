#ifndef INFO_H
#define INFO_H

#include"LinkedList.h"



struct MemberInformation //login
{
	char sID[11]; // 10���� ���̵�
	char sPassword[15]; // 14���� ��й�ȣ
	char sNickname[21];//�г��� 20����
};

struct RoomInformation
{
	RoomInformation *pPrev;
	int nMemberCnt;
	char name[100];//20���� ���̸�
	int nRoomID;
	//userlink
	LQueueType *UserLink;
	LogQueueType *pLog;
	int nLogcnt;
	RoomInformation *pNext;
	
};
typedef struct{ // ���� ť���� ����ϴ� ������ front�� rear�� ����ü�� ����
	RoomInformation *pFirst;
	RoomInformation *pLast;
} RQueueType;

RQueueType *createRoomLinkedQueue() // ���� ���� ť ���� ����
{
	RQueueType *RQ;
	RQ = (RQueueType *)malloc(sizeof(RQueueType));
	RQ->pFirst = NULL;
	RQ->pLast = NULL;
	return RQ;
}

void enQueueLog(RoomInformation *pRoom, char* item)
{
	LogQueueType *pLog = pRoom->pLog;
	chattinglog *newLog = (chattinglog*)malloc(sizeof(chattinglog));
	newLog->item = item;
	newLog->pNext = NULL;
	if (pRoom->nLogcnt < 40)
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
	pRoom->nLogcnt++;
	
	
}

void deleteLogRoom(RoomInformation* pRoom)
{
	LogQueueType *pLog = pRoom->pLog;
	chattinglog *pTemp = pLog->pFirst;
	chattinglog *pPrev;
	printf("��αװ� �� �����Ǵ��� ���ô� %d->", pRoom->nLogcnt);
	while (pTemp)
	{
		pPrev = pTemp;
		pTemp = pTemp->pNext;
		free(pPrev);
		pRoom->nLogcnt--;
	}
	printf("%d\n", pRoom->nLogcnt);

}


void enQueueRoom(RQueueType *RQ, RoomInformation *item)
// ����ť�� rear�����Ҹ������ϴ� ����
{
	RoomInformation* newRoom = item;
	newRoom->pNext = NULL;
	
	if (RQ->pFirst == NULL) { // ���� ���� ť�� ������ ���//����Ƽ
		RQ->pFirst = newRoom;
		RQ->pLast = newRoom;
		newRoom->pPrev = NULL;
	}
	else { // ���� ���� ť�� ������ �ƴ� ���
		newRoom->pPrev = RQ->pLast;
		RQ->pLast->pNext = newRoom;
		RQ->pLast = newRoom;
	}
}

void deleteRoom(RQueueType *RQ, RoomInformation *item, int n)
{

	RoomInformation *temp = RQ->pFirst;
	RoomInformation *pre;
	while (temp)
	{
		if (temp->nRoomID == n)
		{
			if (temp == RQ->pFirst) //ó���Ŷ�� front�� �ڷ�
			{
				RQ->pFirst = RQ->pFirst->pNext;
				if (RQ->pFirst == NULL)//�׷��� ������ ���ٸ�
					RQ->pLast = NULL;//��1����������
				break;
			}
			else if (temp == RQ->pLast){
				RQ->pLast = RQ->pLast->pPrev;
				RQ->pLast->pNext = NULL;
				break;
			}
			else
			{
				pre = temp->pNext;
				pre->pNext = temp->pNext;
				temp->pNext->pPrev = pre;
				
				break;
			}
		}

		temp = temp->pNext;
	}

	free(temp);

}
#endif
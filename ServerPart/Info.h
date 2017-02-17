#ifndef INFO_H
#define INFO_H

#include"LinkedList.h"



struct MemberInformation //login
{
	char sID[11]; // 10글자 아이디
	char sPassword[15]; // 14글자 비밀번호
	char sNickname[21];//닉네임 20글자
};

struct RoomInformation
{
	RoomInformation *pPrev;
	int nMemberCnt;
	char name[100];//20글자 방이름
	int nRoomID;
	//userlink
	LQueueType *UserLink;
	LogQueueType *pLog;
	int nLogcnt;
	RoomInformation *pNext;
	
};
typedef struct{ // 연결 큐에서 사용하는 포인터 front와 rear를 구조체로 정의
	RoomInformation *pFirst;
	RoomInformation *pLast;
} RQueueType;

RQueueType *createRoomLinkedQueue() // 공백 연결 큐 생성 연산
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
	printf("방로그가 잘 삭제되는지 봅시다 %d->", pRoom->nLogcnt);
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
// 연결큐의 rear에원소를삽입하는 연산
{
	RoomInformation* newRoom = item;
	newRoom->pNext = NULL;
	
	if (RQ->pFirst == NULL) { // 현재 연결 큐가 공백인 경우//엠프티
		RQ->pFirst = newRoom;
		RQ->pLast = newRoom;
		newRoom->pPrev = NULL;
	}
	else { // 현재 연결 큐가 공백이 아닌 경우
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
			if (temp == RQ->pFirst) //처음거라면 front를 뒤로
			{
				RQ->pFirst = RQ->pFirst->pNext;
				if (RQ->pFirst == NULL)//그런데 다음게 없다면
					RQ->pLast = NULL;//즉1개만있을때
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
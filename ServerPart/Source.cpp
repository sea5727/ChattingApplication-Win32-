
#pragma comment(lib, "ws2_32")
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include <fstream>
#include <Windows.h>
#include "Info.h"

CRITICAL_SECTION cs;

#define SERVERPORT 9030
#define BUFSIZE    512
#define MAXMEMBER 100
#define MAXROOM 10
RQueueType *RQ = createRoomLinkedQueue();
LQueueType *LobbyQ = createLinkedQueue();
MemberInformation *MI = (MemberInformation*)malloc(MAXMEMBER*sizeof(MemberInformation));
bool fninforecv;
bool bfilename;
bool *LoginCheck;
int nRoomCnt;
int nRoomID;
int MemberCount; 
int UserCount;

FILE *pIDFile;

typedef struct Files{
	char name[255];
	unsigned int byte;
}Files;

void CheckRoom(RQueueType *RQ) //방 링크를 편하게 확인하기 위한 함수
{
	RoomInformation *temp;
	temp = RQ->pFirst;
	printf("----------만들어진 방----------\n\n");
	
	printf("방 링크 : ");
	while (temp)
	{
		printf("%s->", temp->name);
		temp = temp->pNext;
	}
	printf("\n");
	printf("\n------------------------------------\n");
}
void CheckUser(LQueueType *LQ)//그 방에 대한 유저링크를 편하게 확인하기위한 함수
{
	QNode* temp;
	temp = LQ->front;
	printf("----------접속된 유저 정보----------\n\n");
	printf("유저링크: ");
	while (temp)
	{
		printf("%s(%d)->", temp->Client->sNickname , temp->Client->nCurrent);
		temp = temp->link;
	}
	printf("\n");
	printf("\n------------------------------------\n");
}
void Refresh()//로비의 사람들에게 방정보,유저정보를 갱신될때마다 뿌림
{
	QNode *temp;
	char buf[3];
	int retval;
	temp = LobbyQ->front;
	while (temp)
	{
		if (temp->Client->nCurrent == -1)//만약 로비의 상태라면(로비 : -1)
		{
			strcpy(buf, "a");
			buf[1] = '\0';
			retval = send(temp->Client->sock, buf, 2, 0);//헤더a로 로비정보를 갱신해야한다 라는 정보를 뿌림
		}
		temp = temp->link;
	}
}
// 소켓 함수 오류 출력 후 종료
void err_quit(char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(1);
}

// 소켓 함수 오류 출력
void err_display(char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	printf("[%s] %s", msg, (char *)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

// 내부 구현용 함수
int _recv_ahead(SOCKET s, char *p)
{
	__declspec(thread) static int nbytes = 0;
	__declspec(thread) static char buf[1024];
	__declspec(thread) static char *ptr;

	if (nbytes == 0 || nbytes == SOCKET_ERROR){
		nbytes = recv(s, buf, sizeof(buf), 0);
		if (nbytes == SOCKET_ERROR){
			return SOCKET_ERROR;
		}
		else if (nbytes == 0)
			return 0;
		ptr = buf;
	}

	--nbytes;
	*p = *ptr++;
	return 1;
}

// 사용자 정의 데이터 수신 함수
int recvline(SOCKET s, char *buf, int maxlen)
{
	int n, nbytes;
	char c, *ptr = buf;

	for (n = 1; n<maxlen; n++){
		nbytes = _recv_ahead(s, &c);
		if (nbytes == 1){
			*ptr++ = c;
			if (c == '\0')
				break;
		}
		else if (nbytes == 0){
			*ptr = 0;
			return n - 1;
		}
		else
			return SOCKET_ERROR;
	}

	*ptr = 0;
	return n;
}
int recvn(SOCKET s, char *buf, int len, int flags)
{
	int received;
	char *ptr = buf;
	int left = len;

	while (left > 0){
		received = recv(s, ptr, left, flags);    //receved에는  ptr이라는 버퍼를 가리키는 포인트에 
		//얼마나 데이터가 쌓여있는지 알려주는 거죠 음 몇 바이트를 받았나 이런식으로
		if (received == SOCKET_ERROR)  // 이것은 소켓.h인가에보면 아마 socket_error이 -1 값을갖을 겁니다. 
			return SOCKET_ERROR;
		else if (received == 0)   // 이거는 받을게 없다는거죠 
			break;// 받을깨 없으면 그만하라는 겁니다. 
		left -= received; // 받을게 있을때문 실행을 하는 문장이 이문장과 아래 문장 두개내요
		ptr += received;

		// left에는 버퍼의 크기가 들어 있을 겁니다.  요 위에 두문장과 while문장의 조건문

		// 요거가 키 포인트 입니다.

		// 밖에다가 따로 설명해 줄깨요
	}

	return (len - left);
}
void recvfile(SOCKET client_sock, SOCKET to_sock)
{
	printf("recvFile로 들어왔습니다\n");
	//FILE *fp = NULL;
	Files files;
	int retval;
	fninforecv = FALSE;
	unsigned int count;
	char buf[BUFSIZE];
	retval = recvn(client_sock, (char*)&files, sizeof(files), 0);//파일의 기본적인정보를 받고
	if (retval == SOCKET_ERROR){ err_display("recv()"); exit(1); }
	
	retval = send(to_sock, (char*)&files, sizeof(files), 0);//바로 상대클라에게 준다
	if (retval == SOCKET_ERROR) { err_display("send()"); exit(1); }
	
	unsigned int per;
	per = count = files.byte / BUFSIZE;


	while (!fninforecv)//여기서 정지를 시킨다. 상대클라에게 같은이름의 파일이 있는지 확인을 하여야 하기 때문이다.
	{

	}//있거나 혹은 없다는 정보를 확인하면 fn info recv 전역변수가 1이됨
	if (bfilename)//filename이 겹치지 않으면 전송을하고 //겹친다면 else
	{
		char str[2];
		str[0] = 'y';
		str[1] = '\0';
		retval = send(client_sock, str, 2,0);
		printf("파일전송을 받습니다\n");
		printf("전송하는 파일 : %s, 전송하는 파일 크기 : %d Byte\n", files.name, files.byte);
		printf("\n클라이언트로부터 파일을 받는 중입니다.\n");
		//fp = fopen(files.name, "wb");

		count = files.byte / BUFSIZE;

		while (count)
		{
			printf("파일전송중입니다 %d가 0이되어야 전송완료됩니다\n", count);
			retval = recvn(client_sock, buf, BUFSIZE, 0);
			if (retval == SOCKET_ERROR){ err_display("recv()"); exit(1); }

			//전송
			retval = send(to_sock, buf, BUFSIZE, 0);
			if (retval == SOCKET_ERROR){ err_display("send()"); exit(1); }

			//파일 작성 작업
			//	fwrite(buf, 1, BUFSIZE, fp);
			count--;

		}
		//남은 파일 크기만큼 나머지 받기
		count = files.byte - ((files.byte / BUFSIZE) * BUFSIZE);

		retval = recvn(client_sock, buf, BUFSIZE, 0);
		if (retval == SOCKET_ERROR){ err_display("recv()"); exit(1); }

		retval = send(to_sock, buf, BUFSIZE, 0);
		if (retval == SOCKET_ERROR){ err_display("send()"); exit(1); }

		//	fwrite(buf, 1, count, fp);

		//파일포인터 닫기
		//fclose(fp);
		printf("\n파일전송이 완료되었습니다\n");
	}
	else//no
	{
		char str[2];
		str[0] = 'n';
		str[1] = '\0';
		retval = send(client_sock, str, 2, 0);
		printf("\n파일전송이 취소되었습니다\n");
	}
	/*
	fp = fopen(files.name, "rb");
	if (fp == NULL)
	{
		printf("같은이름 파일이 없으므로 전송을 진행합니다\n");
	}
	else
	{
		//system("cls");
		printf("이미 같은 이름의 파일이 존재합니다");
	
		fclose(fp);
	}*/
	


}
//링크드리스트소켓으로 데이터 전송 쓰레드
DWORD WINAPI ThreadSend(LPVOID s)//같은방의 유저들에게 뿌려줄 쓰레드
{
	int retval;
	int len;
	
	LQueueType *LQ = (LQueueType*)s;
	QNode *temp = LQ->front;
	
	printf("%s 가 보내준 메세지를 보냅니다 : %s ", LQ->sRecvNick,LQ->buf);
	
	char *sNick;
	char *sBuf;

	int cnt = 0;
	while (temp)
	{
		sNick = (char*)malloc(21 * sizeof(char));
		sBuf = (char*)malloc(101 * sizeof(char));

		strcpy(sNick, LQ->sRecvNick);
		strcpy(sBuf, LQ->buf);
		EnterCriticalSection(&cs);
		enQueueLogClient(temp->Client, sNick);
		enQueueLogClient(temp->Client, sBuf);
		LeaveCriticalSection(&cs);
		retval = send(temp->Client->sock, LQ->sRecvNick, strlen(LQ->sRecvNick)+1, 0);
		retval = send(temp->Client->sock, LQ->buf, LQ->buflen, 0);
		temp = temp->link;
		cnt++;
	}
	printf(" %d번\n", cnt);
	return 0;
};
/*
DWORD WINAPI ThreadSendInfo(LPVOID s)
{
	int retval;
	LQueueType *LQ = (LQueueType*)s;
	QNode *temp = LQ->front;

	int cnt = 0;
	while (temp)
	{
		retval = send(temp->Client.sock, LQ->sRecvNick, strlen(LQ->sRecvNick) + 1, 0);
		temp = temp->link;
		cnt++;
	}
	free(temp);
	printf(" 방에들어와서 닉네임정보를 %d번 보냄\n", cnt);
	return 0;
};*/
DWORD WINAPI ThreadProc(LPVOID s)
{
	ConnectedUser *Client = (ConnectedUser*)s;//클라를매개변수로

	FILE *FileTemp;
	SOCKET client_sock = Client->sock;//둘다 같이쓰여서 혼동하여 써버렸음
	char sysmsg[21];
	char buf[BUFSIZE + 1];
	char CPS1[11];
	char CPS2[15];
	char CPS3[21];
	char PS1[11];
	char PS2[15];
	char PS3[21];
	char Count[5];
	
	char SystemMessage[20];
	int retval;
	int cnt;
	int j;
	int t;
	int len;
	int recvlen;
	bool JoinCheck = FALSE;
	bool bExit;//재대로 나갔는지여부
	QNode *temp;
	RoomInformation *pRoomtemp;
	chattinglog *pLogtemp;
	char* logmatrix[100];
	char* logmatrixNick[21];
	while (1)
	{
		//1 회원가입 , 2 로그인, 3 방,유저목록(로비), 4 채팅방생성
		//5 채팅하는부분, 6 기존의방에참여 7 유저목록(채팅방),8방에서나감
		//9별명을바꿈,a 로비정보갱신,b 파일전송을 받을지 물음, c 로그요청
		//d 파일전송, e 로비에서나감(정상적인종료), f파일전송을 받겠다고 응답한메세지
		//y 파일이름이 겹치지않음 n 파일이름이 겹침
		printf("---------Client의 응답 대기중----------\n");
		retval = recvline(client_sock, buf, BUFSIZE + 1);//클라의 요청을 기다림(헤더정보)
		
		if (retval == SOCKET_ERROR)
		{
			err_display("header");
			break;
		}
		printf("(%c) ", buf[0]);
		switch (buf[0])
		{
		case '1'://회원가입
			printf("회원가입 요청(case 1)\n");
			JoinCheck = FALSE;
			for (int i = 0; i < 3; i++)
			{
				retval = recvline(client_sock, buf, BUFSIZE + 1);//아이디, 비번, 별명을 세번받아서
				if (i == 0)
				{
					strcpy(PS1, buf);//아이디
				}
				else if (i == 1)
				{
					strcpy(PS2, buf);//비번
				}
				else if (i == 2)
				{
					strcpy(PS3, buf);//별명
				}
			}


			EnterCriticalSection(&cs);//읽는거뿐이므로 필요가없음
			FileTemp = fopen("ID.dat", "r");
			if (FileTemp)
			{
				printf("계정데이터 열기완료");

			}
			
			//fclose(pIDFile);
			LeaveCriticalSection(&cs);

			//같은아이디나 별명이 있을경우
			for (j = 0; j < MemberCount; j++)
			{
				
				fscanf(FileTemp, "%s\t%s\t%s\n", CPS1, CPS2, CPS3);
				//한줄씩 현재 저장된 아이디,비번, 닉네임
				
				if (!strcmp(CPS1, PS1))//아이디 가 같으면
				{//같은게 하나라도 발견되었다면
					printf("같은아이디가 있습니다 다시입력해주세요 %s, %s\n", CPS1, PS1);
					sprintf(buf, "아이디가 중복됩니다! %s", PS1);
					len = strlen(buf);
					buf[len++] = '\0';
					retval = send(client_sock, buf, len, 0);//중복메세지
					fclose(FileTemp);//파일을닫고 나옴
					break;
				}
				else if (!strcmp(CPS3, PS3))//닉네임이 중복
				{
					printf("같은닉네임이 있습니다 다시입력해주세요 %s, %s\n", CPS3, PS3);
					sprintf(buf, "별명이 중복됩니다! %s", PS3);
					len = strlen(buf);
					buf[len++] = '\0';
					retval = send(client_sock, buf, len, 0);//중복메세지
					fclose(FileTemp);//파일을닫고 나옴
					break;
				}
				if (j == MemberCount - 1)//위의 두조건을 둘다 피하면
				{//다뒤져도 겹치는게 없다! 회원가입승인
					fclose(FileTemp);
					printf("회원가입을합니다.\n");

					EnterCriticalSection(&cs);//쓰기이므로 혼동이 생기지 않게 CriticalSection
					FileTemp = fopen("ID.dat", "a");
					if (FileTemp)
					{
						printf("계정데이터 열기완료");
					}
					fprintf(FileTemp, "%s\t%s\t%s\n", PS1, PS2, PS3);
					fclose(FileTemp);
					LeaveCriticalSection(&cs);

					
	

					JoinCheck = TRUE;
					sprintf(buf, "회원가입이 완료되었습니다!");//회원가입이 완료되었다.
					len = strlen(buf);
					buf[len++] = '\0';
					retval = send(client_sock, buf, len, 0);
				}
			}
			if (JoinCheck)//안에 써도 되는데 어쩌다보니 밖에 씀
				MemberCount++;//멤버수 ++
			
			memset(PS1, 0, sizeof(PS1));
			memset(PS2, 0, sizeof(PS2));
			memset(PS3, 0, sizeof(PS3));
			
			break;
		case '2':
			printf("로그인 요청(case 2)\n");//로그인요청
			retval = recvline(client_sock, buf, BUFSIZE + 1);
			strcpy(PS1, buf);
			retval = recvline(client_sock, buf, BUFSIZE + 1);
			strcpy(PS2, buf);//아이디와 비밀번호를 받고
			EnterCriticalSection(&cs);
			FileTemp = fopen("ID.dat", "r");
			if (FileTemp)
			{
				printf("계정데이터 열기완료 \n");
				
			}
			
			//fclose(pIDFile);
			LeaveCriticalSection(&cs);
			for (int i = 0; i < MemberCount; i++)
			{
				EnterCriticalSection(&cs);
				fscanf(FileTemp, "%s\t%s\t%s\n", CPS1, CPS2, CPS3);//멤버수만큼 돌려서
				//printf("저장된 계정정보:  %s,%s,%s\n", CPS1, CPS2, CPS3);
				
				LeaveCriticalSection(&cs);
				if (!strcmp(CPS1, PS1) && !strcmp(CPS2, PS2))//아이디 비번 둘다 맞으면
				{
					if (LoginCheck[i] == 0)//중복로그인을 확인하고
					{
						LoginCheck[i] = 1;//중복못시키게 1로
						Client->nLoginCheck = i;//인덱스값을 준다.
						strcpy(buf, "1");//1로그인성공
						buf[1] = '\0';
						retval = send(client_sock, buf, 2, 0);

						strncpy(Client->sID, CPS1, strlen(CPS1) + 1);//Client->sID 구조체정보에 넣음
						strncpy(Client->sNickname, CPS3, strlen(CPS3) + 1);//클라구조체정보에 넣음
						printf("로그인하였습니다 ID: %s, 별명 : %s\n", Client->sID, Client->sNickname);

						Client->nCurrent = -1;//먼저 로비로들어감
						EnterCriticalSection(&cs);
						enQueue(LobbyQ, Client);//로비에도 큐가 있음
						LeaveCriticalSection(&cs);
						bExit = FALSE;//이 회원은 아직 강제종료하지 않았음
						UserCount++;//접속유저 ++
						CheckUser(LobbyQ);//서버화면에 로비유저들목록이 뜸
						fclose(FileTemp);//파일을닫음

						break;
					}
					else //LoginCheck[i] == 1 중복로그인이되었음
					{
						strcpy(buf, "3");//누군가 접속해있음
						buf[1] = '\0';
						retval = send(client_sock, buf, 2, 0);
						printf("중복로그인하셨습니다.\n");
						fclose(FileTemp);//파일을닫음
						break;
					}
					
				}//아이디와 비번이 맞지않음
				else if (i == MemberCount - 1)
				{
					fclose(FileTemp);
					//다시로그인하라고 알려줘야함
					strcpy(buf, "2");//로그인실패
					buf[1] = '\0';
					retval = send(client_sock, buf, 2, 0);
					printf("로그인에 실패하셨습니다\n");
				}
			}
		
			break;
		case '3'://로비에서 현재 방정보 및 유저정보를 보냄
			printf("현재 생성된 채팅방정보(case 3)\n");
			if (nRoomCnt == 0)//방이 0이면 0이라고보냄
			{
		
				itoa(nRoomCnt, buf, 10);//방카운트갯수를 문자형으로 변환
				len = strlen(buf);
				buf[len++] = '\0'; 
				retval = send(client_sock, buf, 2, 0);//보냄
			}
			else //방이 한개라도있으면
			{
				itoa(nRoomCnt, buf, 10);//방이있음//방카운트 갯수를 문자형으로 변환
				len = strlen(buf);
				buf[len++] = '\0';
				retval = send(client_sock, buf, 2, 0);//보냄
				
				pRoomtemp = RQ->pFirst;//방의 링크드 첫번째
				while (pRoomtemp)//더이상의 연결이 업으면 빠져나옴
				{
					strcpy(buf, pRoomtemp->name);
					len = strlen(pRoomtemp->name);
					buf[len++] = '\0';
					retval = send(client_sock, buf, len, 0);//방이름을 보냄
					memset(buf, 0, BUFSIZE + 1);

					itoa(pRoomtemp->nMemberCnt, buf, 10);
					len = strlen(buf);
					buf[len++] = '\0';
					retval = send(client_sock, buf, len, 0);//방의 멤버수를 보냄

					itoa(pRoomtemp->nRoomID, buf, 10);
					len = strlen(buf);
					buf[len++] = '\0';
					retval = send(client_sock, buf, len, 0);//방 ID를 보냄

					pRoomtemp = pRoomtemp->pNext;//그다음연결로
				}
			}
			//여기까지 방에 대한 정보를 보냄//
			printf("현재 유저접속수 :  %d 입니다", UserCount);//Server에 출력
			itoa(UserCount, buf, 10);//유저숫자를 문자형으로 변환후
			len = strlen(buf);
			buf[len++] = '\0';
			retval = send(Client->sock, buf, len, 0);//보냄
			temp = LobbyQ->front;//로비큐의 첫번째
			printf("접속유저 현황 : ");
			while (temp)
			{
				if (temp->Client->nCurrent == -1)//그사람이 로비라면
					sprintf(buf, "%s(로비)", temp->Client->sNickname);
				else//아니면 방위치도함께출력
					sprintf(buf, "%s(방ID:%d)", temp->Client->sNickname, temp->Client->nCurrent);
				len = strlen(buf);
				buf[len++] = '\0';
				printf("%s, ", buf);
				retval = send(Client->sock, buf, len, 0);//유저의 위치정보를 보냄
				temp = temp->link;
			}
			printf("\n");
			break;
		case '4':
			printf("채팅방을 생성합니다(case 4)\n");//방생성
			retval = recvline(client_sock, buf, BUFSIZE);//방이름을 받아서
			if (retval == SOCKET_ERROR)
			{
				err_display("recv() 방만들기부분");
				break;
			}
			RoomInformation *newRoom;
			newRoom = (RoomInformation*)malloc(sizeof(RoomInformation));//방을 생성
			
			newRoom->UserLink = createLinkedQueue();//방의 소켓링크를 생성

			//nRoomID는 서버시작일땐0 방이 만들어질때마다 1씩증가중임
			newRoom->nRoomID = nRoomID;//이방 아이디도 ID돌
			Client->nCurrent = nRoomID;//현재유저위치를 방ID로
			
			Client->nLogcnt = 0;
			Client->pLog = (LogQueueType*)malloc(sizeof(LogQueueType));//로그생성//방이생성되면 로그도 생성됨
			Client->pLog->pFirst = NULL;
			Client->pLog->pLast = NULL;

			EnterCriticalSection(&cs);
			enQueue(newRoom->UserLink, Client);//클라를 유저링크에 연결함(이때는 혼자이므로 첫번째에 연결될듯)
			LeaveCriticalSection(&cs);
			sprintf(newRoom->name, "%s", buf);//이제서야 방이름을 넣어줌
			printf("방 ID :%d, %s를 생성하고 log및 socket 정보 초기화합니다\n", Client->nCurrent, newRoom->name);//서버에출력
			newRoom->nMemberCnt = 1;//이방의 유저수1
			newRoom->nLogcnt = 0;//로그갯수 초기화0
			EnterCriticalSection(&cs);
			enQueueRoom(RQ, newRoom);//방을 링크드로 연결함//RQ는 전역으로 선언되어있음
			LeaveCriticalSection(&cs);
			memset(buf, 0, BUFSIZE + 1);
	
			nRoomCnt++;//방갯수 증가
			nRoomID++;//방고유번호 증가
			CheckRoom(RQ);//서버에 방현황출력
			Refresh();//로비에 유저들에게 방이 생겼으니 로비정보를 초기화
			
			newRoom->pLog = (LogQueueType*)malloc(sizeof(LogQueueType));//로그생성//방이생성되면 로그도 생성됨
			newRoom->pLog->pFirst = NULL;
			newRoom->pLog->pLast = NULL;

			break;
		case '5'://채팅하는부분
			printf("case 5로 들어옵니다 채팅하는거\n");
			retval = recvline(client_sock, buf, BUFSIZE);//채팅정보를 받음
			if (retval == SOCKET_ERROR){
				err_display("recv()1");
				break;
			}
			else if (retval == 0)
				break;
			// 받은 데이터 출력
			printf("[TCP] %s의 메세지를 받았습니다 :  %s\n", Client->sNickname, buf);//서버에 출력
			
			
			//보내는쓰레드
			pRoomtemp = RQ->pFirst; //이사람이 속해있는 방을 찾고
			while (pRoomtemp)
			{
				if (pRoomtemp->nRoomID == Client->nCurrent)//방ID와 현재 유저의 상태를 비교하여
					break;//같으면나오고
				pRoomtemp = pRoomtemp->pNext;//찾을때까지 돌림
			}
			recvlen = strlen(buf);//받은길이
			EnterCriticalSection(&cs);
			//로그기록저장//할때부터 별명과 붙여서가아니고 따로
			*logmatrixNick = (char*)malloc(sizeof(char[21]));
			strcpy(*logmatrixNick, Client->sNickname);
			enQueueLog(pRoomtemp, *logmatrixNick);//로그에 인큐
	
			*logmatrix = (char*)malloc(sizeof(char[100]));//채팅정보 인큐
			strcpy(*logmatrix, buf);
			enQueueLog(pRoomtemp, *logmatrix);
			LeaveCriticalSection(&cs);
			//별명-채팅-별명-채팅 순으로 되어있다.//총길이 40개//별명-채팅이 하나로치면20개
			//로그기록

			memcpy(pRoomtemp->UserLink->buf, buf, recvlen);//Userlink에 buf를 저장 같이 매개변수로 쓰레드로 보냄
			pRoomtemp->UserLink->buf[recvlen++] = '\0';
			pRoomtemp->UserLink->buflen = recvlen;
			memcpy(pRoomtemp->UserLink->sRecvNick, Client->sNickname, strlen(Client->sNickname) + 1);//UserLink에 닉네임도 같이 저장
			memcpy(pRoomtemp->UserLink->sRecvID, Client->sID, strlen(Client->sID) + 1);//클라이언트 아이디도 저장//쓰이진 않았음
			CreateThread(NULL, 0, ThreadSend, (LPVOID)pRoomtemp->UserLink, 0, NULL);//뿌려주는 쓰레드
			break;
		case '6':
			printf("채팅방에 참여합니다(case 6)\n");//방참여
			memset(buf, 0, BUFSIZE);

			retval = recvline(client_sock, buf, BUFSIZE);//방ID 를 받는다아마도 배열에 저장되어있다.
			Client->nCurrent = atoi(buf);//방아이디를 문자형을 정수형으로

			strncpy(SystemMessage, "/a*System**",11);//누가 참가하므로 채팅방유저에게 알린다.//채팅방유저들에게 누가 참여한다고 알림
			SystemMessage[11] = '\0';
			
			
			pRoomtemp = RQ->pFirst;
			while (pRoomtemp)//어떤방인지 찾음
			{
				
				if (pRoomtemp->nRoomID == Client->nCurrent) //받은 방ID로 방Link를 찾고
					break;
				pRoomtemp = pRoomtemp->pNext;
			}
			printf("방제%s 참여\n",pRoomtemp->name);
			
			temp = pRoomtemp->UserLink->front;
			while (temp)//방안에 링크를 연결하기위해
			{
				retval = send(temp->Client->sock, SystemMessage, 12, 0);//시스템이
				retval = send(temp->Client->sock, Client->sNickname, strlen(Client->sNickname)+1, 0);//누가 접속했는지 알려줌
				temp = temp->link;
			}
			pRoomtemp->nMemberCnt++;//방의 유저수 ++

			Client->nLogcnt = 0;
			Client->pLog = (LogQueueType*)malloc(sizeof(LogQueueType));//로그생성//방이생성되면 로그도 생성됨
			Client->pLog->pFirst = NULL;
			Client->pLog->pLast = NULL;

			enQueue(pRoomtemp->UserLink, Client);//유저를 링크드리스터로 연결
			Refresh();//로비에있는사람에게 정보초기화
			break;
		case '7':
			printf("채팅방의 유저목록을 보여줌(case 7)\n");//채팅방에 들어오면 유저목록을 보내줍니다.

			pRoomtemp = RQ->pFirst;
			while (pRoomtemp)//방을 찾고
			{
				if (pRoomtemp->nRoomID == Client->nCurrent)
					break;
				pRoomtemp = pRoomtemp->pNext;
			}

			temp = pRoomtemp->UserLink->front; 
			
		
			cnt = pRoomtemp->nMemberCnt; 
			itoa(cnt, Count, 10); 
			len = strlen(Count);
			Count[len++] = '\0';
			retval = send(client_sock, Count, len, 0);//채팅방 참여자수 보냄
			while(temp)//채팅방에 있는사람들의 닉네임을 모두 받음
			{
				retval = send(client_sock, temp->Client->sNickname, strlen(temp->Client->sNickname)+1, 0);
				temp = temp->link;
			}
			CheckUser(LobbyQ);//로비현황확인
			break;
		case '8':
			printf("방에서 나가는 요청(case 8)\n");//채팅방에 나갑니다
			//해야할일 : 로그를 지움
			//방을 없애야 하는지?
			deleteLogClient(Client);
			pRoomtemp = RQ->pFirst;
			while (pRoomtemp)//방을찾고
			{
				if (pRoomtemp->nRoomID == Client->nCurrent)
					break;
				pRoomtemp = pRoomtemp->pNext;
			}

			exitRoom(pRoomtemp->UserLink, Client);//유저를 링크드에서 디큐함
			pRoomtemp->nMemberCnt--;//멤버수--
			if (pRoomtemp->nMemberCnt != 0) //채팅방안에 사람이 있으면 시스템메세지 전송
			{
				strncpy(SystemMessage, "/b*System**", 11);
				SystemMessage[11] = '\0';
				temp = pRoomtemp->UserLink->front;
				while (temp)
				{
					retval = send(temp->Client->sock, SystemMessage, 12, 0);//시스템이
					retval = send(temp->Client->sock, Client->sNickname, strlen(Client->sNickname) + 1, 0);//누가 나갔는지 알려줌
					temp = temp->link;
				}
			}
			else
			{
				deleteLogRoom(pRoomtemp);//메모리를위해서 방로그를 지움
				printf("방을 지웁니다\n");//사람이 없으면
				deleteRoom(RQ, pRoomtemp, Client->nCurrent);//방을 방링크드리스트에서 디큐함
				nRoomCnt--;//방갯수--
				CheckRoom(RQ);//방이 지워졌는지 서버에서 확인하게 출력함
			}
			//Refresh();

			Client->nCurrent = -1;//이제 이 유저는 로비임
			
			
			break;
		case '9': //별명바꾸기
			printf("별명바꾸는 요청(case 9)\n");//별명바꾸기
			retval = recvline(client_sock, buf, BUFSIZE);//별명을 받음
			strcpy(PS3,buf);
			printf("받은 별명%s\n", PS3);//받은별명을 일단 PS3에 보관

			EnterCriticalSection(&cs);
			FileTemp = fopen("ID.dat", "r");//읽기위해염
			if (FileTemp)
			{
				printf("계정데이터 열기완료");

			}

			//fclose(pIDFile);
			LeaveCriticalSection(&cs);


			for (j = 0; j < MemberCount; j++)
			{
				EnterCriticalSection(&cs);
				fscanf(FileTemp, "%s\t%s\t%s\n", CPS1, CPS2, CPS3);//차근히 읽어서//CPS3이 기존의별명
				printf("현재 저장된 아이디비번닉네임 정보: %s,%s,%s\n", CPS1, CPS2, CPS3);
				LeaveCriticalSection(&cs);

				if (!strcmp(CPS1, Client->sID))
				{
					t = j;
				}
				if (!strcmp(CPS3, PS3))//CPS3기존의 별명과 PS3 바꾸고싶은별명을 비교
				{
					printf("같은닉네임이 있습니다 다시입력해주세요 %s, %s\n", CPS3, PS3);//누군가 쓰고있어서 겹침
					sprintf(buf, "*중복됩니다* %s", PS3);
					len = strlen(buf);
					buf[len++] = '\0';
					retval = send(client_sock, buf, len, 0);
					break;
				}
				if (j == MemberCount - 1)
				{//다뒤져도 겹치는게 없다! 승인
					fclose(FileTemp);
					printf("겹치는게 없네여 별명을바꿉니다\n");
					strcpy(Client->sNickname, PS3);
					strcpy_s(MI[t].sNickname, PS3);//배열에 저장중이었음 회원정보를//배열을먼저바꿈
					//여기서 데이터를 바꿔야하는데...
					EnterCriticalSection(&cs);
					FileTemp = fopen("ID.dat", "w");//새로쓰는이유는 도중에 바꾸는게 어렵거나 방법이 없었음
					for (int i = 0; i < MemberCount; i++)
					{
						fprintf(FileTemp,"%s\t%s\t%s\n", MI[i].sID, MI[i].sPassword, MI[i].sNickname);//바꾼걸 새로씀
					}
					fclose(FileTemp);
					LeaveCriticalSection(&cs);
					sprintf(buf, "완료되었습니다!");
					len = strlen(buf);
					buf[len++] = '\0';
					retval = send(client_sock, buf, len, 0);//완료정보 보냄
				}
			}

			
			
			break;
		case 'a':
			printf("현재 로비 정보(case a)\n");//로비에있는사람들에게 갱신정보를 뿌림
			Refresh();
			break;
		case 'b'://파일전송 연결을 설정
			printf("파일전송을 위한 연결설정을 합니다(case b)\n");

			retval = recvline(client_sock, buf, BUFSIZE);//받을 클라이언트의 닉네임을 받음
		
			t = atoi(buf);//닉네임을 받고(GUI라서 Index로 받아짐)

			pRoomtemp = RQ->pFirst;
			while (pRoomtemp)//방을 찾고
			{
				if (pRoomtemp->nRoomID == Client->nCurrent)
					break;
				pRoomtemp = pRoomtemp->pNext;
			}

			temp = pRoomtemp->UserLink->front;

			for (int i = 0; i < t; i++)//주어진 인덱스로 클라정보를 찾음
				temp = temp->link;
			//여기까지받을클라 찾음
			printf("보내는사람%s", Client->sNickname);
			printf(" 받는사람%s\n", temp->Client->sNickname);
			
			
			if (!strcmp(temp->Client->sNickname, Client->sNickname))//근데 자기자신에게 보낸다고 했으면
			{//못한다고 알림

				printf("error 자기자신에겐 전송할수 없습니다\n");
				strcpy(sysmsg, "/c**system***");
				len = strlen(sysmsg);
				sysmsg[len++] = '\0';
				retval = send(Client->sock, sysmsg, len, 0);

				strcpy(buf, "자기자신에겐 전송할수 없습니다");
				len = strlen(buf);
				buf[len++] = '\0';
				retval = send(Client->sock, buf, len, 0);

			}
			else//자기자신이 아니면 상대에게 받을지 묻고
			{
				strcpy(sysmsg, "/e**system***");//시스템헤더e
				len = strlen(sysmsg);
				sysmsg[len++] = '\0';
				retval = send(temp->Client->sock, sysmsg, len, 0);//상대방에게 받을지 물음

				strcpy(buf, Client->sNickname);
				//받는쪽에게 의사를 물어봄
				len = strlen(buf);
				buf[len++] = '\0';
				retval = send(temp->Client->sock, buf, len, 0);	

				printf("%s 파일전송의사를 물어봅니다\n", temp->Client->sNickname);
				
			}

			break;
		case 'c':
			printf("로그를 불러옵니다(case c)\n");//로그를 클라에서 요청함
			pLogtemp = Client->pLog->pFirst;
			while (pLogtemp)
			{
				retval = send(Client->sock, pLogtemp->item, strlen(pLogtemp->item) + 1, 0);
				pLogtemp = pLogtemp->pNext;
			}
			break;
			/*pRoomtemp = RQ->pFirst;//어떤방인지 찾고
			while (pRoomtemp)
			{
				if (pRoomtemp->nRoomID == Client->nCurrent)
					break;
				pRoomtemp = pRoomtemp->pNext;
			}

			
			pLogtemp = pRoomtemp->pLog->pFirst;//그방의 로그를 다보냄//40개(별명-내용) 으로 저장됨으로 그냥다보내도 20개이하가됨
			while (pLogtemp)
			{
				retval = send(Client->sock, pLogtemp->item, strlen(pLogtemp->item) + 1, 0);
				pLogtemp= pLogtemp->pNext;
			}*/
		case 'd'://파일전송하는부분
			printf("파일전송을 합니다(case d)\n");
			retval = recvline(client_sock, buf, BUFSIZE);//상대닉네임정보를받음
			pRoomtemp = RQ->pFirst;
			while (pRoomtemp)//방을찾고
			{
				if (pRoomtemp->nRoomID == Client->nCurrent)
					break;
				pRoomtemp = pRoomtemp->pNext;
			}
			temp = pRoomtemp->UserLink->front;//유저를 찾음
			while (temp)
			{
				if (!strcmp(temp->Client->sNickname, buf))
					break;
				temp = temp->link;
			}
			
			retval = recvline(Client->sock, buf, BUFSIZE);//전송자가 파일을 재대로 클릭함
			if (buf[0] == 'y')
			{
				printf("%s --> %s 전송시작\n", Client->sNickname, temp->Client->sNickname);
				char str[2];
				str[0] = 'y';
				str[1] = '\0';
				retval = send(temp->Client->sock, str, 2, 0);
				recvfile(Client->sock, temp->Client->sock);//전송을 시작함
			}
			else//재대로 클릭안하고 취소를클릭함
			{
				printf("취소하라고받았습니다\n");
				//전송받을상대에게 취소하라고 
				char str[2];
				str[0] = 'n';
				str[1] = '\0';
				retval = send(temp->Client->sock, str, 2, 0);//전송을 취소함
			}
			
			break;
		case 'e'://로비룸나가기//즉 종료이다
			printf("로비룸을 나갑니다(case e)\n");
			exitRoom(LobbyQ, Client);//로비룸큐에서 빼고
			UserCount--;//접속유저--
			LoginCheck[Client->nLoginCheck] = 0;//로그인체크를 풀고
			CheckUser(LobbyQ);//로비현황을 뛰우고
			Refresh();//로비에있는유저에게 새로운갱신정보
			bExit = TRUE;//강제로나간게 아니기때문에 TRUE
			break;
		case 'f':
			printf("파일전송을 받을지 대답을받음(case f)\n");//전송을 받을지말지 상대클라에게 물어봄그후 답이옴
			if (buf[1] == 'y')//전송을 받겠다고 응답함
			{
				printf("전송받을것인지에대한 응답을 했습니다 %s\n", buf);
				retval = recvline(client_sock, buf, BUFSIZE);//전송받겠다고 응답했음 buf는 전송을 해주는 클라닉네임
				pRoomtemp = RQ->pFirst;
				while (pRoomtemp)
				{
					if (pRoomtemp->nRoomID == Client->nCurrent)//방을찾고//두 클라 모두 같은방이므로
					{
						break;
					}
					pRoomtemp = pRoomtemp->pNext;
				}

				temp = pRoomtemp->UserLink->front;//유저를찾고
				while (temp)
				{
					if (!strcmp(temp->Client->sNickname, buf))//닉네임정보로 유저를 찾음
						break;
					temp = temp->link;
				}
					

				
				strcpy(sysmsg, "/d**system***");
				len = strlen(sysmsg);
				sysmsg[len++] = '\0';
				retval = send(temp->Client->sock, sysmsg, len, 0);//받을쪾에서 전송준비가 되었다고 알림

				strcpy(buf, Client->sNickname);
				len = strlen(buf);
				buf[len++] = '\0';
				retval = send(temp->Client->sock, buf, len, 0);//받을쪾에서 전송준비가 되었다고 알림

			}
			else if (buf[1] == 'n')//받을쪽에서 전송을 안받는다고 눌렀을경우
			{
				printf("전송받을것인지에대한 응답을 했습니다 %s\n", buf);
				retval = recvline(client_sock, buf, BUFSIZE);
				printf("또받았습니다. %s\n", buf);
				pRoomtemp = RQ->pFirst;
				while (pRoomtemp)//방을찾고
				{
					if (pRoomtemp->nRoomID == Client->nCurrent)
					{
						break;
					}
					pRoomtemp = pRoomtemp->pNext;
				}

				temp = pRoomtemp->UserLink->front;//유저를 찾고
				while (temp)
				{
					if (!strcmp(temp->Client->sNickname, buf))
						break;
					temp = temp->link;
				}
				printf("%s를 찾아습니다\n", temp->Client->sNickname);

				strcpy(sysmsg, "/f**system***");
				len = strlen(sysmsg);
				sysmsg[len++] = '\0';
				retval = send(temp->Client->sock, sysmsg, len, 0);//전송을 안한다고 알림

				strcpy(buf, Client->sNickname);
				len = strlen(buf);
				buf[len++] = '\0';
				retval = send(temp->Client->sock, buf, len, 0);//전송을 안받겠다고 했따고 알림

			}
			break;
		case 'y':
			fninforecv = TRUE;//파일이름이 있는지 없는지에 대한 정보가 왔고
			bfilename = TRUE;//파일이름겹침이 없음
			break;
		case 'n':
			fninforecv = TRUE;
			bfilename = FALSE;//파일이름침이 있음
			break;
		default:
			printf("%s가 보낸", Client->sNickname);
			printf("디폴트입니다");
	
		
		}
		
	}
	////쓰레드의 while 바깥부분/////
	if (bExit == FALSE)//갑작스럽게 나갔다면
	{
		if (Client->nCurrent != -1)//로비를나가 정상적인 종료를 한것이 아니라면
		{
			pRoomtemp = RQ->pFirst;
			while (pRoomtemp)
			{
				if (pRoomtemp->nRoomID == Client->nCurrent)
					break;
				pRoomtemp = pRoomtemp->pNext;
			}
			exitRoom(pRoomtemp->UserLink, Client);//있던방에 유저링크를 디큐해주고

			pRoomtemp->nMemberCnt--;//유저수--
			if (pRoomtemp->nMemberCnt != 0) //사람이 있으면 시스템메세지 전송
			{
				strncpy(SystemMessage, "/b*System**", 11);
				SystemMessage[11] = '\0';
				temp = pRoomtemp->UserLink->front;
				while (temp)
				{
					retval = send(temp->Client->sock, SystemMessage, 12, 0);//시스템이
					retval = send(temp->Client->sock, Client->sNickname, strlen(Client->sNickname) + 1, 0);//누가 나갔는지 알려줌
					temp = temp->link;
				}
			}
			else//혼자있었다면 
			{

				printf("방을 지웁니다\n");
				deleteRoom(RQ, pRoomtemp, Client->nCurrent);
				nRoomCnt--;
				CheckRoom(RQ);
				//방지우라는 메세지
			}
		}
		exitRoom(LobbyQ, Client);//방을 링크에서 디큐함
		LoginCheck[Client->nLoginCheck] = 0;//로그인체크 풀어주고
		UserCount--;//접속유저수--
		CheckUser(LobbyQ);
		Refresh();
	}
	
	
		// closesocket()
	closesocket(client_sock);
	printf("[TCP 서버] 클라이언트 종료: IP주소=%s,포트번호 =%d\n",inet_ntoa(Client->sockaddr.sin_addr), ntohs(Client->sockaddr.sin_port));
	
	free(Client);
	return 0;
}
int main(int argc, char *argv[])
{
	LoginCheck = (bool*)malloc(MAXMEMBER*sizeof(bool));//BOOL배열로 중복로그인을 방지하는 동적할당배열, 접속을하면1, 안하면0
	for (int i = 0; i < MAXMEMBER; i++)
		LoginCheck[i] = 0;

	MemberCount = 0;//전역변수 초기화
	nRoomCnt = 0;
	UserCount = 0;
	int retval;
	

	char sID[11]; // 10글자 아이디
	char sPassword[15]; // 14글자 비밀번호
	char sNickname[21];//닉네임 20글자

	pIDFile = fopen("ID.dat", "r+");
	if (pIDFile)
	{
		printf("계정데이터 오픈완료\n");
	}

	while (MemberCount < MAXMEMBER)//최대멤버까지 쭉 .dat파일을 읽어서
	{
		fscanf(pIDFile, "%s\t%s\t%s\n",sID,sPassword,sNickname );
		if (MemberCount != 0)//전에 뽑았던 아이디와 지금아이디가 겹치면(즉 마지막이면 그만둠)
		{
			if (!strcmp(MI[MemberCount - 1].sID, sID))
				break;
		}
		strcpy_s(MI[MemberCount].sID, sID);//구조체 MemberInformation인 MI에 각각 저장
		strcpy_s(MI[MemberCount].sPassword, sPassword);
		strcpy_s(MI[MemberCount].sNickname, sNickname);
		
		MemberCount++;// 멤버수
	}
	
	printf("현재 회원현황(%d명)\n", MemberCount);
	for (int i = 0; i < MemberCount; i++)
	{
		printf("%s\t%s\t%s\n", MI[i].sID, MI[i].sPassword, MI[i].sNickname);
	}//현재의 회원들정보

	fclose(pIDFile);

	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// socket()
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) err_quit("socket()");

	// bind()
	SOCKADDR_IN serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = bind(listen_sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("bind()");

	// listen()
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR) err_quit("listen()");

	// 데이터 통신에 사용할 변수
	
	int addrlen;


	InitializeCriticalSection(&cs);

	printf("채팅방 서버 실행완료\n");
	while (1){
		// accept()
	
		///
		SOCKET client_sock;
		SOCKADDR_IN clientaddr;

		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (SOCKADDR *)&clientaddr, &addrlen);
		///
	
		if (client_sock == INVALID_SOCKET){
			err_display("accept()");
			break;
		}



		// 접속한 클라이언트 정보 출력
		printf("\n[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n",
			inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

		ConnectedUser *Client1;
		Client1 = (ConnectedUser*)malloc(sizeof(ConnectedUser));
		Client1->sock = client_sock;
		Client1->sockaddr = clientaddr;//클라가 접속할때마다 클라생성
		CreateThread(NULL, 0, ThreadProc, (LPVOID)Client1, 0, NULL);//클라를 매개변수로 쓰레드 생성
	
	}
	DeleteCriticalSection(&cs);
	// closesocket()
	closesocket(listen_sock);

	// 윈속 종료
	WSACleanup();
	return 0;
}

	



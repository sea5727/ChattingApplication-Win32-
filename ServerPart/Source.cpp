
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

void CheckRoom(RQueueType *RQ) //�� ��ũ�� ���ϰ� Ȯ���ϱ� ���� �Լ�
{
	RoomInformation *temp;
	temp = RQ->pFirst;
	printf("----------������� ��----------\n\n");
	
	printf("�� ��ũ : ");
	while (temp)
	{
		printf("%s->", temp->name);
		temp = temp->pNext;
	}
	printf("\n");
	printf("\n------------------------------------\n");
}
void CheckUser(LQueueType *LQ)//�� �濡 ���� ������ũ�� ���ϰ� Ȯ���ϱ����� �Լ�
{
	QNode* temp;
	temp = LQ->front;
	printf("----------���ӵ� ���� ����----------\n\n");
	printf("������ũ: ");
	while (temp)
	{
		printf("%s(%d)->", temp->Client->sNickname , temp->Client->nCurrent);
		temp = temp->link;
	}
	printf("\n");
	printf("\n------------------------------------\n");
}
void Refresh()//�κ��� ����鿡�� ������,���������� ���ŵɶ����� �Ѹ�
{
	QNode *temp;
	char buf[3];
	int retval;
	temp = LobbyQ->front;
	while (temp)
	{
		if (temp->Client->nCurrent == -1)//���� �κ��� ���¶��(�κ� : -1)
		{
			strcpy(buf, "a");
			buf[1] = '\0';
			retval = send(temp->Client->sock, buf, 2, 0);//���a�� �κ������� �����ؾ��Ѵ� ��� ������ �Ѹ�
		}
		temp = temp->link;
	}
}
// ���� �Լ� ���� ��� �� ����
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

// ���� �Լ� ���� ���
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

// ���� ������ �Լ�
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

// ����� ���� ������ ���� �Լ�
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
		received = recv(s, ptr, left, flags);    //receved����  ptr�̶�� ���۸� ����Ű�� ����Ʈ�� 
		//�󸶳� �����Ͱ� �׿��ִ��� �˷��ִ� ���� �� �� ����Ʈ�� �޾ҳ� �̷�������
		if (received == SOCKET_ERROR)  // �̰��� ����.h�ΰ������� �Ƹ� socket_error�� -1 �������� �̴ϴ�. 
			return SOCKET_ERROR;
		else if (received == 0)   // �̰Ŵ� ������ ���ٴ°��� 
			break;// ������ ������ �׸��϶�� �̴ϴ�. 
		left -= received; // ������ �������� ������ �ϴ� ������ �̹���� �Ʒ� ���� �ΰ�����
		ptr += received;

		// left���� ������ ũ�Ⱑ ��� ���� �̴ϴ�.  �� ���� �ι���� while������ ���ǹ�

		// ��Ű� Ű ����Ʈ �Դϴ�.

		// �ۿ��ٰ� ���� ������ �ٱ���
	}

	return (len - left);
}
void recvfile(SOCKET client_sock, SOCKET to_sock)
{
	printf("recvFile�� ���Խ��ϴ�\n");
	//FILE *fp = NULL;
	Files files;
	int retval;
	fninforecv = FALSE;
	unsigned int count;
	char buf[BUFSIZE];
	retval = recvn(client_sock, (char*)&files, sizeof(files), 0);//������ �⺻���������� �ް�
	if (retval == SOCKET_ERROR){ err_display("recv()"); exit(1); }
	
	retval = send(to_sock, (char*)&files, sizeof(files), 0);//�ٷ� ���Ŭ�󿡰� �ش�
	if (retval == SOCKET_ERROR) { err_display("send()"); exit(1); }
	
	unsigned int per;
	per = count = files.byte / BUFSIZE;


	while (!fninforecv)//���⼭ ������ ��Ų��. ���Ŭ�󿡰� �����̸��� ������ �ִ��� Ȯ���� �Ͽ��� �ϱ� �����̴�.
	{

	}//�ְų� Ȥ�� ���ٴ� ������ Ȯ���ϸ� fn info recv ���������� 1�̵�
	if (bfilename)//filename�� ��ġ�� ������ �������ϰ� //��ģ�ٸ� else
	{
		char str[2];
		str[0] = 'y';
		str[1] = '\0';
		retval = send(client_sock, str, 2,0);
		printf("���������� �޽��ϴ�\n");
		printf("�����ϴ� ���� : %s, �����ϴ� ���� ũ�� : %d Byte\n", files.name, files.byte);
		printf("\nŬ���̾�Ʈ�κ��� ������ �޴� ���Դϴ�.\n");
		//fp = fopen(files.name, "wb");

		count = files.byte / BUFSIZE;

		while (count)
		{
			printf("�����������Դϴ� %d�� 0�̵Ǿ�� ���ۿϷ�˴ϴ�\n", count);
			retval = recvn(client_sock, buf, BUFSIZE, 0);
			if (retval == SOCKET_ERROR){ err_display("recv()"); exit(1); }

			//����
			retval = send(to_sock, buf, BUFSIZE, 0);
			if (retval == SOCKET_ERROR){ err_display("send()"); exit(1); }

			//���� �ۼ� �۾�
			//	fwrite(buf, 1, BUFSIZE, fp);
			count--;

		}
		//���� ���� ũ�⸸ŭ ������ �ޱ�
		count = files.byte - ((files.byte / BUFSIZE) * BUFSIZE);

		retval = recvn(client_sock, buf, BUFSIZE, 0);
		if (retval == SOCKET_ERROR){ err_display("recv()"); exit(1); }

		retval = send(to_sock, buf, BUFSIZE, 0);
		if (retval == SOCKET_ERROR){ err_display("send()"); exit(1); }

		//	fwrite(buf, 1, count, fp);

		//���������� �ݱ�
		//fclose(fp);
		printf("\n���������� �Ϸ�Ǿ����ϴ�\n");
	}
	else//no
	{
		char str[2];
		str[0] = 'n';
		str[1] = '\0';
		retval = send(client_sock, str, 2, 0);
		printf("\n���������� ��ҵǾ����ϴ�\n");
	}
	/*
	fp = fopen(files.name, "rb");
	if (fp == NULL)
	{
		printf("�����̸� ������ �����Ƿ� ������ �����մϴ�\n");
	}
	else
	{
		//system("cls");
		printf("�̹� ���� �̸��� ������ �����մϴ�");
	
		fclose(fp);
	}*/
	


}
//��ũ�帮��Ʈ�������� ������ ���� ������
DWORD WINAPI ThreadSend(LPVOID s)//�������� �����鿡�� �ѷ��� ������
{
	int retval;
	int len;
	
	LQueueType *LQ = (LQueueType*)s;
	QNode *temp = LQ->front;
	
	printf("%s �� ������ �޼����� �����ϴ� : %s ", LQ->sRecvNick,LQ->buf);
	
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
	printf(" %d��\n", cnt);
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
	printf(" �濡���ͼ� �г��������� %d�� ����\n", cnt);
	return 0;
};*/
DWORD WINAPI ThreadProc(LPVOID s)
{
	ConnectedUser *Client = (ConnectedUser*)s;//Ŭ�󸦸Ű�������

	FILE *FileTemp;
	SOCKET client_sock = Client->sock;//�Ѵ� ���̾����� ȥ���Ͽ� �������
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
	bool bExit;//���� ������������
	QNode *temp;
	RoomInformation *pRoomtemp;
	chattinglog *pLogtemp;
	char* logmatrix[100];
	char* logmatrixNick[21];
	while (1)
	{
		//1 ȸ������ , 2 �α���, 3 ��,�������(�κ�), 4 ä�ù����
		//5 ä���ϴºκ�, 6 �����ǹ濡���� 7 �������(ä�ù�),8�濡������
		//9�������ٲ�,a �κ���������,b ���������� ������ ����, c �α׿�û
		//d ��������, e �κ񿡼�����(������������), f���������� �ްڴٰ� �����Ѹ޼���
		//y �����̸��� ��ġ������ n �����̸��� ��ħ
		printf("---------Client�� ���� �����----------\n");
		retval = recvline(client_sock, buf, BUFSIZE + 1);//Ŭ���� ��û�� ��ٸ�(�������)
		
		if (retval == SOCKET_ERROR)
		{
			err_display("header");
			break;
		}
		printf("(%c) ", buf[0]);
		switch (buf[0])
		{
		case '1'://ȸ������
			printf("ȸ������ ��û(case 1)\n");
			JoinCheck = FALSE;
			for (int i = 0; i < 3; i++)
			{
				retval = recvline(client_sock, buf, BUFSIZE + 1);//���̵�, ���, ������ �����޾Ƽ�
				if (i == 0)
				{
					strcpy(PS1, buf);//���̵�
				}
				else if (i == 1)
				{
					strcpy(PS2, buf);//���
				}
				else if (i == 2)
				{
					strcpy(PS3, buf);//����
				}
			}


			EnterCriticalSection(&cs);//�д°Ż��̹Ƿ� �ʿ䰡����
			FileTemp = fopen("ID.dat", "r");
			if (FileTemp)
			{
				printf("���������� ����Ϸ�");

			}
			
			//fclose(pIDFile);
			LeaveCriticalSection(&cs);

			//�������̵� ������ �������
			for (j = 0; j < MemberCount; j++)
			{
				
				fscanf(FileTemp, "%s\t%s\t%s\n", CPS1, CPS2, CPS3);
				//���پ� ���� ����� ���̵�,���, �г���
				
				if (!strcmp(CPS1, PS1))//���̵� �� ������
				{//������ �ϳ��� �߰ߵǾ��ٸ�
					printf("�������̵� �ֽ��ϴ� �ٽ��Է����ּ��� %s, %s\n", CPS1, PS1);
					sprintf(buf, "���̵� �ߺ��˴ϴ�! %s", PS1);
					len = strlen(buf);
					buf[len++] = '\0';
					retval = send(client_sock, buf, len, 0);//�ߺ��޼���
					fclose(FileTemp);//�������ݰ� ����
					break;
				}
				else if (!strcmp(CPS3, PS3))//�г����� �ߺ�
				{
					printf("�����г����� �ֽ��ϴ� �ٽ��Է����ּ��� %s, %s\n", CPS3, PS3);
					sprintf(buf, "������ �ߺ��˴ϴ�! %s", PS3);
					len = strlen(buf);
					buf[len++] = '\0';
					retval = send(client_sock, buf, len, 0);//�ߺ��޼���
					fclose(FileTemp);//�������ݰ� ����
					break;
				}
				if (j == MemberCount - 1)//���� �������� �Ѵ� ���ϸ�
				{//�ٵ����� ��ġ�°� ����! ȸ�����Խ���
					fclose(FileTemp);
					printf("ȸ���������մϴ�.\n");

					EnterCriticalSection(&cs);//�����̹Ƿ� ȥ���� ������ �ʰ� CriticalSection
					FileTemp = fopen("ID.dat", "a");
					if (FileTemp)
					{
						printf("���������� ����Ϸ�");
					}
					fprintf(FileTemp, "%s\t%s\t%s\n", PS1, PS2, PS3);
					fclose(FileTemp);
					LeaveCriticalSection(&cs);

					
	

					JoinCheck = TRUE;
					sprintf(buf, "ȸ�������� �Ϸ�Ǿ����ϴ�!");//ȸ�������� �Ϸ�Ǿ���.
					len = strlen(buf);
					buf[len++] = '\0';
					retval = send(client_sock, buf, len, 0);
				}
			}
			if (JoinCheck)//�ȿ� �ᵵ �Ǵµ� ��¼�ٺ��� �ۿ� ��
				MemberCount++;//����� ++
			
			memset(PS1, 0, sizeof(PS1));
			memset(PS2, 0, sizeof(PS2));
			memset(PS3, 0, sizeof(PS3));
			
			break;
		case '2':
			printf("�α��� ��û(case 2)\n");//�α��ο�û
			retval = recvline(client_sock, buf, BUFSIZE + 1);
			strcpy(PS1, buf);
			retval = recvline(client_sock, buf, BUFSIZE + 1);
			strcpy(PS2, buf);//���̵�� ��й�ȣ�� �ް�
			EnterCriticalSection(&cs);
			FileTemp = fopen("ID.dat", "r");
			if (FileTemp)
			{
				printf("���������� ����Ϸ� \n");
				
			}
			
			//fclose(pIDFile);
			LeaveCriticalSection(&cs);
			for (int i = 0; i < MemberCount; i++)
			{
				EnterCriticalSection(&cs);
				fscanf(FileTemp, "%s\t%s\t%s\n", CPS1, CPS2, CPS3);//�������ŭ ������
				//printf("����� ��������:  %s,%s,%s\n", CPS1, CPS2, CPS3);
				
				LeaveCriticalSection(&cs);
				if (!strcmp(CPS1, PS1) && !strcmp(CPS2, PS2))//���̵� ��� �Ѵ� ������
				{
					if (LoginCheck[i] == 0)//�ߺ��α����� Ȯ���ϰ�
					{
						LoginCheck[i] = 1;//�ߺ�����Ű�� 1��
						Client->nLoginCheck = i;//�ε������� �ش�.
						strcpy(buf, "1");//1�α��μ���
						buf[1] = '\0';
						retval = send(client_sock, buf, 2, 0);

						strncpy(Client->sID, CPS1, strlen(CPS1) + 1);//Client->sID ����ü������ ����
						strncpy(Client->sNickname, CPS3, strlen(CPS3) + 1);//Ŭ����ü������ ����
						printf("�α����Ͽ����ϴ� ID: %s, ���� : %s\n", Client->sID, Client->sNickname);

						Client->nCurrent = -1;//���� �κ�ε�
						EnterCriticalSection(&cs);
						enQueue(LobbyQ, Client);//�κ񿡵� ť�� ����
						LeaveCriticalSection(&cs);
						bExit = FALSE;//�� ȸ���� ���� ������������ �ʾ���
						UserCount++;//�������� ++
						CheckUser(LobbyQ);//����ȭ�鿡 �κ����������� ��
						fclose(FileTemp);//����������

						break;
					}
					else //LoginCheck[i] == 1 �ߺ��α����̵Ǿ���
					{
						strcpy(buf, "3");//������ ����������
						buf[1] = '\0';
						retval = send(client_sock, buf, 2, 0);
						printf("�ߺ��α����ϼ̽��ϴ�.\n");
						fclose(FileTemp);//����������
						break;
					}
					
				}//���̵�� ����� ��������
				else if (i == MemberCount - 1)
				{
					fclose(FileTemp);
					//�ٽ÷α����϶�� �˷������
					strcpy(buf, "2");//�α��ν���
					buf[1] = '\0';
					retval = send(client_sock, buf, 2, 0);
					printf("�α��ο� �����ϼ̽��ϴ�\n");
				}
			}
		
			break;
		case '3'://�κ񿡼� ���� ������ �� ���������� ����
			printf("���� ������ ä�ù�����(case 3)\n");
			if (nRoomCnt == 0)//���� 0�̸� 0�̶����
			{
		
				itoa(nRoomCnt, buf, 10);//��ī��Ʈ������ ���������� ��ȯ
				len = strlen(buf);
				buf[len++] = '\0'; 
				retval = send(client_sock, buf, 2, 0);//����
			}
			else //���� �Ѱ���������
			{
				itoa(nRoomCnt, buf, 10);//��������//��ī��Ʈ ������ ���������� ��ȯ
				len = strlen(buf);
				buf[len++] = '\0';
				retval = send(client_sock, buf, 2, 0);//����
				
				pRoomtemp = RQ->pFirst;//���� ��ũ�� ù��°
				while (pRoomtemp)//���̻��� ������ ������ ��������
				{
					strcpy(buf, pRoomtemp->name);
					len = strlen(pRoomtemp->name);
					buf[len++] = '\0';
					retval = send(client_sock, buf, len, 0);//���̸��� ����
					memset(buf, 0, BUFSIZE + 1);

					itoa(pRoomtemp->nMemberCnt, buf, 10);
					len = strlen(buf);
					buf[len++] = '\0';
					retval = send(client_sock, buf, len, 0);//���� ������� ����

					itoa(pRoomtemp->nRoomID, buf, 10);
					len = strlen(buf);
					buf[len++] = '\0';
					retval = send(client_sock, buf, len, 0);//�� ID�� ����

					pRoomtemp = pRoomtemp->pNext;//�״��������
				}
			}
			//������� �濡 ���� ������ ����//
			printf("���� �������Ӽ� :  %d �Դϴ�", UserCount);//Server�� ���
			itoa(UserCount, buf, 10);//�������ڸ� ���������� ��ȯ��
			len = strlen(buf);
			buf[len++] = '\0';
			retval = send(Client->sock, buf, len, 0);//����
			temp = LobbyQ->front;//�κ�ť�� ù��°
			printf("�������� ��Ȳ : ");
			while (temp)
			{
				if (temp->Client->nCurrent == -1)//�׻���� �κ���
					sprintf(buf, "%s(�κ�)", temp->Client->sNickname);
				else//�ƴϸ� ����ġ���Բ����
					sprintf(buf, "%s(��ID:%d)", temp->Client->sNickname, temp->Client->nCurrent);
				len = strlen(buf);
				buf[len++] = '\0';
				printf("%s, ", buf);
				retval = send(Client->sock, buf, len, 0);//������ ��ġ������ ����
				temp = temp->link;
			}
			printf("\n");
			break;
		case '4':
			printf("ä�ù��� �����մϴ�(case 4)\n");//�����
			retval = recvline(client_sock, buf, BUFSIZE);//���̸��� �޾Ƽ�
			if (retval == SOCKET_ERROR)
			{
				err_display("recv() �游���κ�");
				break;
			}
			RoomInformation *newRoom;
			newRoom = (RoomInformation*)malloc(sizeof(RoomInformation));//���� ����
			
			newRoom->UserLink = createLinkedQueue();//���� ���ϸ�ũ�� ����

			//nRoomID�� ���������϶�0 ���� ������������� 1����������
			newRoom->nRoomID = nRoomID;//�̹� ���̵� ID��
			Client->nCurrent = nRoomID;//����������ġ�� ��ID��
			
			Client->nLogcnt = 0;
			Client->pLog = (LogQueueType*)malloc(sizeof(LogQueueType));//�α׻���//���̻����Ǹ� �α׵� ������
			Client->pLog->pFirst = NULL;
			Client->pLog->pLast = NULL;

			EnterCriticalSection(&cs);
			enQueue(newRoom->UserLink, Client);//Ŭ�� ������ũ�� ������(�̶��� ȥ���̹Ƿ� ù��°�� ����ɵ�)
			LeaveCriticalSection(&cs);
			sprintf(newRoom->name, "%s", buf);//�������� ���̸��� �־���
			printf("�� ID :%d, %s�� �����ϰ� log�� socket ���� �ʱ�ȭ�մϴ�\n", Client->nCurrent, newRoom->name);//���������
			newRoom->nMemberCnt = 1;//�̹��� ������1
			newRoom->nLogcnt = 0;//�αװ��� �ʱ�ȭ0
			EnterCriticalSection(&cs);
			enQueueRoom(RQ, newRoom);//���� ��ũ��� ������//RQ�� �������� ����Ǿ�����
			LeaveCriticalSection(&cs);
			memset(buf, 0, BUFSIZE + 1);
	
			nRoomCnt++;//�氹�� ����
			nRoomID++;//�������ȣ ����
			CheckRoom(RQ);//������ ����Ȳ���
			Refresh();//�κ� �����鿡�� ���� �������� �κ������� �ʱ�ȭ
			
			newRoom->pLog = (LogQueueType*)malloc(sizeof(LogQueueType));//�α׻���//���̻����Ǹ� �α׵� ������
			newRoom->pLog->pFirst = NULL;
			newRoom->pLog->pLast = NULL;

			break;
		case '5'://ä���ϴºκ�
			printf("case 5�� ���ɴϴ� ä���ϴ°�\n");
			retval = recvline(client_sock, buf, BUFSIZE);//ä�������� ����
			if (retval == SOCKET_ERROR){
				err_display("recv()1");
				break;
			}
			else if (retval == 0)
				break;
			// ���� ������ ���
			printf("[TCP] %s�� �޼����� �޾ҽ��ϴ� :  %s\n", Client->sNickname, buf);//������ ���
			
			
			//�����¾�����
			pRoomtemp = RQ->pFirst; //�̻���� �����ִ� ���� ã��
			while (pRoomtemp)
			{
				if (pRoomtemp->nRoomID == Client->nCurrent)//��ID�� ���� ������ ���¸� ���Ͽ�
					break;//�����鳪����
				pRoomtemp = pRoomtemp->pNext;//ã�������� ����
			}
			recvlen = strlen(buf);//��������
			EnterCriticalSection(&cs);
			//�αױ������//�Ҷ����� ����� �ٿ������ƴϰ� ����
			*logmatrixNick = (char*)malloc(sizeof(char[21]));
			strcpy(*logmatrixNick, Client->sNickname);
			enQueueLog(pRoomtemp, *logmatrixNick);//�α׿� ��ť
	
			*logmatrix = (char*)malloc(sizeof(char[100]));//ä������ ��ť
			strcpy(*logmatrix, buf);
			enQueueLog(pRoomtemp, *logmatrix);
			LeaveCriticalSection(&cs);
			//����-ä��-����-ä�� ������ �Ǿ��ִ�.//�ѱ��� 40��//����-ä���� �ϳ���ġ��20��
			//�αױ��

			memcpy(pRoomtemp->UserLink->buf, buf, recvlen);//Userlink�� buf�� ���� ���� �Ű������� ������� ����
			pRoomtemp->UserLink->buf[recvlen++] = '\0';
			pRoomtemp->UserLink->buflen = recvlen;
			memcpy(pRoomtemp->UserLink->sRecvNick, Client->sNickname, strlen(Client->sNickname) + 1);//UserLink�� �г��ӵ� ���� ����
			memcpy(pRoomtemp->UserLink->sRecvID, Client->sID, strlen(Client->sID) + 1);//Ŭ���̾�Ʈ ���̵� ����//������ �ʾ���
			CreateThread(NULL, 0, ThreadSend, (LPVOID)pRoomtemp->UserLink, 0, NULL);//�ѷ��ִ� ������
			break;
		case '6':
			printf("ä�ù濡 �����մϴ�(case 6)\n");//������
			memset(buf, 0, BUFSIZE);

			retval = recvline(client_sock, buf, BUFSIZE);//��ID �� �޴´پƸ��� �迭�� ����Ǿ��ִ�.
			Client->nCurrent = atoi(buf);//����̵� �������� ����������

			strncpy(SystemMessage, "/a*System**",11);//���� �����ϹǷ� ä�ù��������� �˸���.//ä�ù������鿡�� ���� �����Ѵٰ� �˸�
			SystemMessage[11] = '\0';
			
			
			pRoomtemp = RQ->pFirst;
			while (pRoomtemp)//������� ã��
			{
				
				if (pRoomtemp->nRoomID == Client->nCurrent) //���� ��ID�� ��Link�� ã��
					break;
				pRoomtemp = pRoomtemp->pNext;
			}
			printf("����%s ����\n",pRoomtemp->name);
			
			temp = pRoomtemp->UserLink->front;
			while (temp)//��ȿ� ��ũ�� �����ϱ�����
			{
				retval = send(temp->Client->sock, SystemMessage, 12, 0);//�ý�����
				retval = send(temp->Client->sock, Client->sNickname, strlen(Client->sNickname)+1, 0);//���� �����ߴ��� �˷���
				temp = temp->link;
			}
			pRoomtemp->nMemberCnt++;//���� ������ ++

			Client->nLogcnt = 0;
			Client->pLog = (LogQueueType*)malloc(sizeof(LogQueueType));//�α׻���//���̻����Ǹ� �α׵� ������
			Client->pLog->pFirst = NULL;
			Client->pLog->pLast = NULL;

			enQueue(pRoomtemp->UserLink, Client);//������ ��ũ�帮���ͷ� ����
			Refresh();//�κ��ִ»������ �����ʱ�ȭ
			break;
		case '7':
			printf("ä�ù��� ��������� ������(case 7)\n");//ä�ù濡 ������ ��������� �����ݴϴ�.

			pRoomtemp = RQ->pFirst;
			while (pRoomtemp)//���� ã��
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
			retval = send(client_sock, Count, len, 0);//ä�ù� �����ڼ� ����
			while(temp)//ä�ù濡 �ִ»������ �г����� ��� ����
			{
				retval = send(client_sock, temp->Client->sNickname, strlen(temp->Client->sNickname)+1, 0);
				temp = temp->link;
			}
			CheckUser(LobbyQ);//�κ���ȲȮ��
			break;
		case '8':
			printf("�濡�� ������ ��û(case 8)\n");//ä�ù濡 �����ϴ�
			//�ؾ����� : �α׸� ����
			//���� ���־� �ϴ���?
			deleteLogClient(Client);
			pRoomtemp = RQ->pFirst;
			while (pRoomtemp)//����ã��
			{
				if (pRoomtemp->nRoomID == Client->nCurrent)
					break;
				pRoomtemp = pRoomtemp->pNext;
			}

			exitRoom(pRoomtemp->UserLink, Client);//������ ��ũ�忡�� ��ť��
			pRoomtemp->nMemberCnt--;//�����--
			if (pRoomtemp->nMemberCnt != 0) //ä�ù�ȿ� ����� ������ �ý��۸޼��� ����
			{
				strncpy(SystemMessage, "/b*System**", 11);
				SystemMessage[11] = '\0';
				temp = pRoomtemp->UserLink->front;
				while (temp)
				{
					retval = send(temp->Client->sock, SystemMessage, 12, 0);//�ý�����
					retval = send(temp->Client->sock, Client->sNickname, strlen(Client->sNickname) + 1, 0);//���� �������� �˷���
					temp = temp->link;
				}
			}
			else
			{
				deleteLogRoom(pRoomtemp);//�޸𸮸����ؼ� ��α׸� ����
				printf("���� ����ϴ�\n");//����� ������
				deleteRoom(RQ, pRoomtemp, Client->nCurrent);//���� �渵ũ�帮��Ʈ���� ��ť��
				nRoomCnt--;//�氹��--
				CheckRoom(RQ);//���� ���������� �������� Ȯ���ϰ� �����
			}
			//Refresh();

			Client->nCurrent = -1;//���� �� ������ �κ���
			
			
			break;
		case '9': //����ٲٱ�
			printf("����ٲٴ� ��û(case 9)\n");//����ٲٱ�
			retval = recvline(client_sock, buf, BUFSIZE);//������ ����
			strcpy(PS3,buf);
			printf("���� ����%s\n", PS3);//���������� �ϴ� PS3�� ����

			EnterCriticalSection(&cs);
			FileTemp = fopen("ID.dat", "r");//�б����ؿ�
			if (FileTemp)
			{
				printf("���������� ����Ϸ�");

			}

			//fclose(pIDFile);
			LeaveCriticalSection(&cs);


			for (j = 0; j < MemberCount; j++)
			{
				EnterCriticalSection(&cs);
				fscanf(FileTemp, "%s\t%s\t%s\n", CPS1, CPS2, CPS3);//������ �о//CPS3�� �����Ǻ���
				printf("���� ����� ���̵����г��� ����: %s,%s,%s\n", CPS1, CPS2, CPS3);
				LeaveCriticalSection(&cs);

				if (!strcmp(CPS1, Client->sID))
				{
					t = j;
				}
				if (!strcmp(CPS3, PS3))//CPS3������ ����� PS3 �ٲٰ���������� ��
				{
					printf("�����г����� �ֽ��ϴ� �ٽ��Է����ּ��� %s, %s\n", CPS3, PS3);//������ �����־ ��ħ
					sprintf(buf, "*�ߺ��˴ϴ�* %s", PS3);
					len = strlen(buf);
					buf[len++] = '\0';
					retval = send(client_sock, buf, len, 0);
					break;
				}
				if (j == MemberCount - 1)
				{//�ٵ����� ��ġ�°� ����! ����
					fclose(FileTemp);
					printf("��ġ�°� ���׿� �������ٲߴϴ�\n");
					strcpy(Client->sNickname, PS3);
					strcpy_s(MI[t].sNickname, PS3);//�迭�� �������̾��� ȸ��������//�迭�������ٲ�
					//���⼭ �����͸� �ٲ���ϴµ�...
					EnterCriticalSection(&cs);
					FileTemp = fopen("ID.dat", "w");//���ξ��������� ���߿� �ٲٴ°� ��ưų� ����� ������
					for (int i = 0; i < MemberCount; i++)
					{
						fprintf(FileTemp,"%s\t%s\t%s\n", MI[i].sID, MI[i].sPassword, MI[i].sNickname);//�ٲ۰� ���ξ�
					}
					fclose(FileTemp);
					LeaveCriticalSection(&cs);
					sprintf(buf, "�Ϸ�Ǿ����ϴ�!");
					len = strlen(buf);
					buf[len++] = '\0';
					retval = send(client_sock, buf, len, 0);//�Ϸ����� ����
				}
			}

			
			
			break;
		case 'a':
			printf("���� �κ� ����(case a)\n");//�κ��ִ»���鿡�� ���������� �Ѹ�
			Refresh();
			break;
		case 'b'://�������� ������ ����
			printf("���������� ���� ���ἳ���� �մϴ�(case b)\n");

			retval = recvline(client_sock, buf, BUFSIZE);//���� Ŭ���̾�Ʈ�� �г����� ����
		
			t = atoi(buf);//�г����� �ް�(GUI�� Index�� �޾���)

			pRoomtemp = RQ->pFirst;
			while (pRoomtemp)//���� ã��
			{
				if (pRoomtemp->nRoomID == Client->nCurrent)
					break;
				pRoomtemp = pRoomtemp->pNext;
			}

			temp = pRoomtemp->UserLink->front;

			for (int i = 0; i < t; i++)//�־��� �ε����� Ŭ�������� ã��
				temp = temp->link;
			//�����������Ŭ�� ã��
			printf("�����»��%s", Client->sNickname);
			printf(" �޴»��%s\n", temp->Client->sNickname);
			
			
			if (!strcmp(temp->Client->sNickname, Client->sNickname))//�ٵ� �ڱ��ڽſ��� �����ٰ� ������
			{//���Ѵٰ� �˸�

				printf("error �ڱ��ڽſ��� �����Ҽ� �����ϴ�\n");
				strcpy(sysmsg, "/c**system***");
				len = strlen(sysmsg);
				sysmsg[len++] = '\0';
				retval = send(Client->sock, sysmsg, len, 0);

				strcpy(buf, "�ڱ��ڽſ��� �����Ҽ� �����ϴ�");
				len = strlen(buf);
				buf[len++] = '\0';
				retval = send(Client->sock, buf, len, 0);

			}
			else//�ڱ��ڽ��� �ƴϸ� ��뿡�� ������ ����
			{
				strcpy(sysmsg, "/e**system***");//�ý������e
				len = strlen(sysmsg);
				sysmsg[len++] = '\0';
				retval = send(temp->Client->sock, sysmsg, len, 0);//���濡�� ������ ����

				strcpy(buf, Client->sNickname);
				//�޴��ʿ��� �ǻ縦 ���
				len = strlen(buf);
				buf[len++] = '\0';
				retval = send(temp->Client->sock, buf, len, 0);	

				printf("%s ���������ǻ縦 ����ϴ�\n", temp->Client->sNickname);
				
			}

			break;
		case 'c':
			printf("�α׸� �ҷ��ɴϴ�(case c)\n");//�α׸� Ŭ�󿡼� ��û��
			pLogtemp = Client->pLog->pFirst;
			while (pLogtemp)
			{
				retval = send(Client->sock, pLogtemp->item, strlen(pLogtemp->item) + 1, 0);
				pLogtemp = pLogtemp->pNext;
			}
			break;
			/*pRoomtemp = RQ->pFirst;//������� ã��
			while (pRoomtemp)
			{
				if (pRoomtemp->nRoomID == Client->nCurrent)
					break;
				pRoomtemp = pRoomtemp->pNext;
			}

			
			pLogtemp = pRoomtemp->pLog->pFirst;//�׹��� �α׸� �ٺ���//40��(����-����) ���� ��������� �׳ɴٺ����� 20�����ϰ���
			while (pLogtemp)
			{
				retval = send(Client->sock, pLogtemp->item, strlen(pLogtemp->item) + 1, 0);
				pLogtemp= pLogtemp->pNext;
			}*/
		case 'd'://���������ϴºκ�
			printf("���������� �մϴ�(case d)\n");
			retval = recvline(client_sock, buf, BUFSIZE);//���г�������������
			pRoomtemp = RQ->pFirst;
			while (pRoomtemp)//����ã��
			{
				if (pRoomtemp->nRoomID == Client->nCurrent)
					break;
				pRoomtemp = pRoomtemp->pNext;
			}
			temp = pRoomtemp->UserLink->front;//������ ã��
			while (temp)
			{
				if (!strcmp(temp->Client->sNickname, buf))
					break;
				temp = temp->link;
			}
			
			retval = recvline(Client->sock, buf, BUFSIZE);//�����ڰ� ������ ���� Ŭ����
			if (buf[0] == 'y')
			{
				printf("%s --> %s ���۽���\n", Client->sNickname, temp->Client->sNickname);
				char str[2];
				str[0] = 'y';
				str[1] = '\0';
				retval = send(temp->Client->sock, str, 2, 0);
				recvfile(Client->sock, temp->Client->sock);//������ ������
			}
			else//���� Ŭ�����ϰ� ��Ҹ�Ŭ����
			{
				printf("����϶��޾ҽ��ϴ�\n");
				//���۹�����뿡�� ����϶�� 
				char str[2];
				str[0] = 'n';
				str[1] = '\0';
				retval = send(temp->Client->sock, str, 2, 0);//������ �����
			}
			
			break;
		case 'e'://�κ�볪����//�� �����̴�
			printf("�κ���� �����ϴ�(case e)\n");
			exitRoom(LobbyQ, Client);//�κ��ť���� ����
			UserCount--;//��������--
			LoginCheck[Client->nLoginCheck] = 0;//�α���üũ�� Ǯ��
			CheckUser(LobbyQ);//�κ���Ȳ�� �ٿ��
			Refresh();//�κ��ִ��������� ���ο������
			bExit = TRUE;//�����γ����� �ƴϱ⶧���� TRUE
			break;
		case 'f':
			printf("���������� ������ ���������(case f)\n");//������ ���������� ���Ŭ�󿡰� ������� ���̿�
			if (buf[1] == 'y')//������ �ްڴٰ� ������
			{
				printf("���۹��������������� ������ �߽��ϴ� %s\n", buf);
				retval = recvline(client_sock, buf, BUFSIZE);//���۹ްڴٰ� �������� buf�� ������ ���ִ� Ŭ��г���
				pRoomtemp = RQ->pFirst;
				while (pRoomtemp)
				{
					if (pRoomtemp->nRoomID == Client->nCurrent)//����ã��//�� Ŭ�� ��� �������̹Ƿ�
					{
						break;
					}
					pRoomtemp = pRoomtemp->pNext;
				}

				temp = pRoomtemp->UserLink->front;//������ã��
				while (temp)
				{
					if (!strcmp(temp->Client->sNickname, buf))//�г��������� ������ ã��
						break;
					temp = temp->link;
				}
					

				
				strcpy(sysmsg, "/d**system***");
				len = strlen(sysmsg);
				sysmsg[len++] = '\0';
				retval = send(temp->Client->sock, sysmsg, len, 0);//�����U���� �����غ� �Ǿ��ٰ� �˸�

				strcpy(buf, Client->sNickname);
				len = strlen(buf);
				buf[len++] = '\0';
				retval = send(temp->Client->sock, buf, len, 0);//�����U���� �����غ� �Ǿ��ٰ� �˸�

			}
			else if (buf[1] == 'n')//�����ʿ��� ������ �ȹ޴´ٰ� ���������
			{
				printf("���۹��������������� ������ �߽��ϴ� %s\n", buf);
				retval = recvline(client_sock, buf, BUFSIZE);
				printf("�ǹ޾ҽ��ϴ�. %s\n", buf);
				pRoomtemp = RQ->pFirst;
				while (pRoomtemp)//����ã��
				{
					if (pRoomtemp->nRoomID == Client->nCurrent)
					{
						break;
					}
					pRoomtemp = pRoomtemp->pNext;
				}

				temp = pRoomtemp->UserLink->front;//������ ã��
				while (temp)
				{
					if (!strcmp(temp->Client->sNickname, buf))
						break;
					temp = temp->link;
				}
				printf("%s�� ã�ƽ��ϴ�\n", temp->Client->sNickname);

				strcpy(sysmsg, "/f**system***");
				len = strlen(sysmsg);
				sysmsg[len++] = '\0';
				retval = send(temp->Client->sock, sysmsg, len, 0);//������ ���Ѵٰ� �˸�

				strcpy(buf, Client->sNickname);
				len = strlen(buf);
				buf[len++] = '\0';
				retval = send(temp->Client->sock, buf, len, 0);//������ �ȹްڴٰ� �ߵ��� �˸�

			}
			break;
		case 'y':
			fninforecv = TRUE;//�����̸��� �ִ��� �������� ���� ������ �԰�
			bfilename = TRUE;//�����̸���ħ�� ����
			break;
		case 'n':
			fninforecv = TRUE;
			bfilename = FALSE;//�����̸�ħ�� ����
			break;
		default:
			printf("%s�� ����", Client->sNickname);
			printf("����Ʈ�Դϴ�");
	
		
		}
		
	}
	////�������� while �ٱ��κ�/////
	if (bExit == FALSE)//���۽����� �����ٸ�
	{
		if (Client->nCurrent != -1)//�κ񸦳��� �������� ���Ḧ �Ѱ��� �ƴ϶��
		{
			pRoomtemp = RQ->pFirst;
			while (pRoomtemp)
			{
				if (pRoomtemp->nRoomID == Client->nCurrent)
					break;
				pRoomtemp = pRoomtemp->pNext;
			}
			exitRoom(pRoomtemp->UserLink, Client);//�ִ��濡 ������ũ�� ��ť���ְ�

			pRoomtemp->nMemberCnt--;//������--
			if (pRoomtemp->nMemberCnt != 0) //����� ������ �ý��۸޼��� ����
			{
				strncpy(SystemMessage, "/b*System**", 11);
				SystemMessage[11] = '\0';
				temp = pRoomtemp->UserLink->front;
				while (temp)
				{
					retval = send(temp->Client->sock, SystemMessage, 12, 0);//�ý�����
					retval = send(temp->Client->sock, Client->sNickname, strlen(Client->sNickname) + 1, 0);//���� �������� �˷���
					temp = temp->link;
				}
			}
			else//ȥ���־��ٸ� 
			{

				printf("���� ����ϴ�\n");
				deleteRoom(RQ, pRoomtemp, Client->nCurrent);
				nRoomCnt--;
				CheckRoom(RQ);
				//�������� �޼���
			}
		}
		exitRoom(LobbyQ, Client);//���� ��ũ���� ��ť��
		LoginCheck[Client->nLoginCheck] = 0;//�α���üũ Ǯ���ְ�
		UserCount--;//����������--
		CheckUser(LobbyQ);
		Refresh();
	}
	
	
		// closesocket()
	closesocket(client_sock);
	printf("[TCP ����] Ŭ���̾�Ʈ ����: IP�ּ�=%s,��Ʈ��ȣ =%d\n",inet_ntoa(Client->sockaddr.sin_addr), ntohs(Client->sockaddr.sin_port));
	
	free(Client);
	return 0;
}
int main(int argc, char *argv[])
{
	LoginCheck = (bool*)malloc(MAXMEMBER*sizeof(bool));//BOOL�迭�� �ߺ��α����� �����ϴ� �����Ҵ�迭, �������ϸ�1, ���ϸ�0
	for (int i = 0; i < MAXMEMBER; i++)
		LoginCheck[i] = 0;

	MemberCount = 0;//�������� �ʱ�ȭ
	nRoomCnt = 0;
	UserCount = 0;
	int retval;
	

	char sID[11]; // 10���� ���̵�
	char sPassword[15]; // 14���� ��й�ȣ
	char sNickname[21];//�г��� 20����

	pIDFile = fopen("ID.dat", "r+");
	if (pIDFile)
	{
		printf("���������� ���¿Ϸ�\n");
	}

	while (MemberCount < MAXMEMBER)//�ִ������� �� .dat������ �о
	{
		fscanf(pIDFile, "%s\t%s\t%s\n",sID,sPassword,sNickname );
		if (MemberCount != 0)//���� �̾Ҵ� ���̵�� ���ݾ��̵� ��ġ��(�� �������̸� �׸���)
		{
			if (!strcmp(MI[MemberCount - 1].sID, sID))
				break;
		}
		strcpy_s(MI[MemberCount].sID, sID);//����ü MemberInformation�� MI�� ���� ����
		strcpy_s(MI[MemberCount].sPassword, sPassword);
		strcpy_s(MI[MemberCount].sNickname, sNickname);
		
		MemberCount++;// �����
	}
	
	printf("���� ȸ����Ȳ(%d��)\n", MemberCount);
	for (int i = 0; i < MemberCount; i++)
	{
		printf("%s\t%s\t%s\n", MI[i].sID, MI[i].sPassword, MI[i].sNickname);
	}//������ ȸ��������

	fclose(pIDFile);

	// ���� �ʱ�ȭ
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

	// ������ ��ſ� ����� ����
	
	int addrlen;


	InitializeCriticalSection(&cs);

	printf("ä�ù� ���� ����Ϸ�\n");
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



		// ������ Ŭ���̾�Ʈ ���� ���
		printf("\n[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n",
			inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

		ConnectedUser *Client1;
		Client1 = (ConnectedUser*)malloc(sizeof(ConnectedUser));
		Client1->sock = client_sock;
		Client1->sockaddr = clientaddr;//Ŭ�� �����Ҷ����� Ŭ�����
		CreateThread(NULL, 0, ThreadProc, (LPVOID)Client1, 0, NULL);//Ŭ�� �Ű������� ������ ����
	
	}
	DeleteCriticalSection(&cs);
	// closesocket()
	closesocket(listen_sock);

	// ���� ����
	WSACleanup();
	return 0;
}

	



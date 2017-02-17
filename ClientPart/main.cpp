/*------------------------------------------------
������ �⺻ ���α׷���
-------------------------------------------------*/
#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include <Windows.h>
#include <WindowsX.h>
#include "resource.h"


#define SERVERIP   "127.0.0.1"
#define SERVERPORT 9030
#define BUFSIZE    512

typedef struct Files{
	char name[255];
	unsigned int byte;
}Files;

/////////////��������//////////////////
bool FileResponseCheck;
bool FileSend;
SOCKET sOck;
SOCKADDR_IN serveraddress;
HINSTANCE hInst;
HWND hWl,hCht;
HWND hEdit1, hEdit2;//ä�ù��� �ڵ�
HWND hSystem;//�������� �ý����ڵ�
HWND hLogin,hWattingRoom, hWattingRoomUser,hID, hUser;
HANDLE hThread;
int *rID = (int*)malloc(10*sizeof(int));
/////////////��������//////////////////
///////////////�Լ�////////////////////
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	Join(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	Login_Menu(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	ChattingRoom(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	WaittingRoom(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	CreateRoom(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	ChangeNick(HWND, UINT, WPARAM, LPARAM);

int recvline(SOCKET , char*, int );
int _recv_ahead(SOCKET , char *);
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
void sendfile(SOCKET sock, OPENFILENAME ofn, char* name)
{
	unsigned int count;
	int retval;
	FILE *fp;

	Files files;

	strcpy(files.name, name);

	fp = fopen(ofn.lpstrFile, "rb");

	if (fp == NULL)
	{
		//printf("FILE Pointer ERROR ");
		exit(1);
	}
	//���� ������ ��ġ �ű�
	fseek(fp, 0L, SEEK_END);
	//���� ����Ʈ�� ���
	files.byte = ftell(fp);
	//�ٽ� ���� ó������ ��ġ �ű�
	fseek(fp, 0L, SEEK_SET);


	// ������ ��ſ� ����� ����
	char buf[BUFSIZE];

	//���� �⺻ ���� ����
	retval = send(sock, (char*)&files, sizeof(files), 0);
	if (retval == SOCKET_ERROR) { err_display("send()"); exit(1); }

	char str[100];
	retval = recvline(sock, str, 100);
	if (str[0] == 'y')
	{
		//printf("�̹������� %s, %d ����Ʈ �Դϴ�.\n", files.name, files.byte);
		unsigned int per;
		per = count = files.byte / BUFSIZE;

		while (count)
		{
			//system("cls");
			//printf("�����ϴ� ���� : %s, �����ϴ� ���� ũ�� : %d Byte\n", files.name, files.byte);

			//���� �о ���ۿ� ����
			fread(buf, 1, BUFSIZE, fp);

			//����
			retval = send(sock, buf, BUFSIZE, 0);
			if (retval == SOCKET_ERROR){ err_display("send()"); exit(1); }

			//printf("\n���൵ : %d %%", (per - count) * 100 / per);

			count--;

		}

		//���� ���� ũ�� ��ŭ ������ ����
		count = files.byte - (per*BUFSIZE);
		fread(buf, 1, count, fp);
		retval = send(sock, buf, BUFSIZE, 0);
		if (retval == SOCKET_ERROR){ err_display("send()"); exit(1); }

		//system("cls");
		//printf("�����ϴ� ���� :%s , �����ϴ� ���� ũ�� : %d Byte\n", files.name, files.byte);
		//printf("\n ���൵ : 100%%");
		//printf("\n\n������ �Ϸ�Ǿ����ϴ�.");

		fclose(fp);
		MessageBox(hCht, "���������� �Ϸ�Ǿ����ϴ�", "����", MB_OK);
	}
	else//n
	{
		fclose(fp);
		MessageBox(0, "������ �����̸��� �ߺ��Ǿ����ϴ�", "����", MB_OK);
	}
	
	//recvline(sOck, buf, 100);
}
void DisplayText(char *fmt, ...);
void header(char *m)
{
	char string[5];
	int len;
	int retval;
	strcpy(string, m);
	len = strlen(string);
	string[len++] = '\0';
	retval = send(sOck, string, len, 0);
	if (retval == SOCKET_ERROR) err_quit("header()");
}
void recvfile(SOCKET client_sock)
{

	FILE *fp = NULL;
	Files files;
	int retval;
	unsigned int count;
	char buf[BUFSIZE];
	retval = recvn(client_sock, (char*)&files, sizeof(files), 0);
	if (retval == SOCKET_ERROR){ err_display("recv()"); exit(1); }



	fp = fopen(files.name, "rb");
	if (fp == NULL)
	{
		header("y");
		//printf("�����̸� ������ �����Ƿ� ������ �����մϴ�\n");
		//printf("���������� �޽��ϴ�\n");
		//printf("�����ϴ� ���� : %s, �����ϴ� ���� ũ�� : %d Byte\n", files.name, files.byte);
		//printf("\nŬ���̾�Ʈ�κ��� ������ �޴� ���Դϴ�.\n");
		fp = fopen(files.name, "wb");

		count = files.byte / BUFSIZE;

		while (count)
		{
			//�ݱ�
			retval = recvn(client_sock, buf, BUFSIZE, 0);
			if (retval == SOCKET_ERROR){ err_display("recv()"); exit(1); }
			//���� �ۼ� �۾�
			fwrite(buf, 1, BUFSIZE, fp);
			count--;

		}
		//���� ���� ũ�⸸ŭ ������ �ޱ�
		count = files.byte - ((files.byte / BUFSIZE) * BUFSIZE);

		retval = recvn(client_sock, buf, BUFSIZE, 0);
		if (retval == SOCKET_ERROR){ err_display("recv()"); exit(1); }

		fwrite(buf, 1, count, fp);

		//���������� �ݱ�
		fclose(fp);
		//printf("\n���������� �Ϸ�Ǿ����ϴ�\n");
		MessageBox(hCht, "���������� �Ϸ�Ǿ����ϴ�", "����", MB_OK);


	}
	else
	{
		//system("cls");
		header("n");

		MessageBox(0, "�̹� ���� �̸��� ������ �����մϴ�", "����", MB_OK);
		
		fclose(fp);
	}
	

}
DWORD WINAPI ThreadRecv(LPVOID s)
{
	
	SOCKET sock = (SOCKET)s;
	char buf[100];
	char Nick[20];
	char chatting[121];
	int retval;
	int n;
	int len;
	int answer;
	while (1)
	{
		retval = recvline(sOck, Nick, 19);
		if (retval == SOCKET_ERROR) err_quit("ä�������� ���� ���߽��ϴ�.");
		retval = recvline(sOck, buf, 99);
		if (retval == SOCKET_ERROR) err_quit("ä�������� ���� ���߽��ϴ�.");

		if (Nick[0] == '/')//�ý��ۿ�û�ϼ��������� ���ο��
		{
			switch (Nick[1])
			{
			case 'a':
				ListBox_AddString(hSystem, buf);
				sprintf(chatting, "%s : %s���� �����ϼ̽��ϴ�.", Nick, buf);
				DisplayText("%s\n", chatting);
				break;
			case 'b':
				n = ListBox_FindString(hSystem, 0, buf);
				ListBox_DeleteString(hSystem, n);
				sprintf(chatting, "%s : %s���� �����ϼ̽��ϴ�.", Nick, buf, n);
				DisplayText("%s\n", chatting);
				break;
			case 'c':
				FileSend = FALSE;
				FileResponseCheck = TRUE;
				
				sprintf(chatting, "%s : %s", Nick, buf);
				MessageBox(0, chatting, "����", MB_ICONERROR);
				
				break;
			case 'd':
				header("d");
				//���۴� �������� �������̴�
				len = strlen(buf);
				buf[len++] = '\0';
				retval = send(sOck, buf, len, 0);
				FileSend = TRUE;
				FileResponseCheck = TRUE;

				if (FileSend == TRUE)
				{
					char szFullPath[_MAX_PATH];

					OPENFILENAME ofn;
					ZeroMemory(&ofn, sizeof(OPENFILENAME));

					FILE *fp;
					ZeroMemory(szFullPath, sizeof(szFullPath));


					ofn.lStructSize = sizeof(OPENFILENAME);
					ofn.lpstrFilter = "��� ����(*.*)\0*.*\0\0";
					ofn.lpstrFile = szFullPath;
					ofn.nMaxFile = sizeof(szFullPath);
					ofn.Flags = OFN_EXPLORER | OFN_LONGNAMES | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
					while (1)
					{
						if (GetOpenFileName(&ofn)) {

							char szFname[_MAX_FNAME];
							char szExt[_MAX_EXT];
							char name[_MAX_FNAME + _MAX_EXT];
							_splitpath(szFullPath, NULL, NULL, szFname, szExt);
							//MessageBox(hCht, "���������Դϴ�. �ٽ� �������ּ���", "����", MB_ICONERROR);

							FILE *fp = fopen(szFullPath, "rb");
							if (fp == NULL)
								MessageBox(hCht, szFullPath, "����", MB_ICONERROR);
							else
							{
								sprintf(name, "%s%s", szFname, szExt);
								header("y");
								sendfile(sOck, ofn, name);
								break;
							}
						}
						else //���Ϻ�����â���� ���� ����Ұ��
						{
							header("n");
							break;
						}
					}

				}
				
				break;
			case 'e':
				sprintf(chatting, "%s : %s���� ���������� �����ðڽ��ϱ�?", Nick, buf);
				answer = MessageBox(hCht, chatting, "����", MB_YESNO);
				if (answer == IDYES){
					header("fy");
					len = strlen(buf);
					buf[len++] = '\0';
					retval = send(sOck, buf, len, 0);

					retval = recvline(sOck, buf,5);
					if (buf[0] == 'y')
						recvfile(sOck);
					else//n
						MessageBox(hCht, "������ ����߽��ϴ�", "����", MB_OK);
						//���ٽ� ä������
				}
				else //No
				{
					strcpy(Nick, "fn");
					len = strlen(Nick);
					Nick[len++] = '\0';
					retval = send(sOck, Nick, len, 0);

					len = strlen(buf);
					buf[len++] = '\0';
					retval = send(sOck, buf, len, 0);
				}
				break;
			case 'f':

				FileSend = FALSE;
				
				FileResponseCheck = TRUE;
				MessageBox(hCht, "������ ������ �ź��߽��ϴ�", "����", MB_OK);
				break;
			default:
				break;
			}
			
		}
		else
		{
			sprintf(chatting, "%s : %s", Nick, buf);
			DisplayText("%s\n", chatting);
		}
		
	}

	return 0;
}
DWORD WINAPI ThreadRefresh(LPVOID s)
{
	SOCKET sock = (SOCKET)s;
	char string[100];
	int retval;
	int len;
	int cnt;
	while (1)
	{
		retval = recvline(sock, string, 99);
		switch(string[0])
		{
		case 'a':

			ListBox_ResetContent(hWattingRoom);
			ListBox_ResetContent(hWattingRoomUser);
			ListBox_ResetContent(hID);
			ListBox_ResetContent(hUser);

			header("3");

			retval = recvline(sock, string, 99);
			if (retval == SOCKET_ERROR) err_quit("���������� ���� ���߽��ϴ�.");
			cnt = atoi(string);
			if (cnt != 0)	{
				for (int i = 0; i < cnt; i++){
					retval = recvline(sock, string, 99);
					if (retval == SOCKET_ERROR) err_quit("���������� ���� ���߽��ϴ�.");
					ListBox_AddString(hWattingRoom, string);
					retval = recvline(sock, string, 99);
					if (retval == SOCKET_ERROR) err_quit("���������� ���� ���߽��ϴ�.");
					ListBox_AddString(hWattingRoomUser, string);
					retval = recvline(sock, string, 99);
					if (retval == SOCKET_ERROR) err_quit("���������� ���� ���߽��ϴ�.");
					ListBox_AddString(hID, string);

					rID[i] = atoi(string);
				}
			}
			else{

			}

			retval = recvline(sock, string, 99);
			cnt = atoi(string);
			for (int i = 0; i < cnt; i++)
			{
				retval = recvline(sock, string, 99);
				ListBox_AddString(hUser, string);
			}
			break;
		default:
			break;
		}

	}
	return 0;
}
///////////////�Լ�////////////////////
int WINAPI WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow)
{


	for (int i = 0; i < 10; i++)
	{
		rID[i] = -1;
	}
	///////////////////
	int retval;

	// ���� �ʱ�ȭ
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// socket()
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) err_quit("socket()");

	// connect()
	SOCKADDR_IN serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr(SERVERIP);
	serveraddr.sin_port = htons(SERVERPORT);
	serveraddress = serveraddr;

	retval = connect(sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("connect()");
	sOck = sock;


	/////////////////
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPLICATION));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);
	wcex.lpszClassName = "basic";
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_APPLICATION));
	RegisterClassEx(&wcex);

	HWND hWnd = CreateWindow("basic", "��ǻ�ͳ�Ʈ��ũ ��������Ʈ", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 300, 300, NULL,
		NULL, hInstance, NULL);

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	HDC hdc;
	PAINTSTRUCT ps;
	char string[100];
	int retval;
	switch (message)
	{
	case WM_COMMAND:
		wmId = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{

		case IDM_LOGIN: //�α���
			DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG1), hWnd, Login_Menu);
			break;
		case IDM_EXIT:
			MessageBox(hWnd, "�ȳ���������", "����", MB_OK);
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);

		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
INT_PTR CALLBACK Join(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int IDcheck = 0;
	char string[100];
	char sID[50];
	char sNick[50];
	char PS1[50];
	char PS2[50];
	int retval;
	int len;
	int hIDC_EDIT[3] = { IDC_EDIT1, IDC_EDIT2, IDC_EDIT4 };
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK)
		{
			GetDlgItemText(hDlg, IDC_EDIT1, sID, 50);
			GetDlgItemText(hDlg, IDC_EDIT4, sNick, 50);
			GetDlgItemText(hDlg, IDC_EDIT2, PS1, 50);
			GetDlgItemText(hDlg, IDC_EDIT3, PS2, 50);
			if (sID[0] == '\0')
			{
				MessageBox(0, "���̵� �Է��ϼ���", "����", MB_OK);
				return (INT_PTR)TRUE;
			}
			if (sNick[0] == '\0')
			{
				MessageBox(0, "������ �Է��ϼ���", "����", MB_OK);
				return (INT_PTR)TRUE;
			}
			if (PS1[0] == '\0')
			{
				MessageBox(0, "��й�ȣ�� �Է��ϼ���", "����", MB_OK);
				return (INT_PTR)TRUE;
			}
			if (strcmp(PS1, PS2) != 0)
			{
				MessageBox(0, "��й�ȣ�� �ٸ��ϴ�", "����", MB_OK);
				SetDlgItemText(hDlg, IDC_EDIT2, "");
				SetDlgItemText(hDlg, IDC_EDIT3, "");
				SetFocus(GetDlgItem(hDlg, IDC_EDIT2));
				return (INT_PTR)TRUE;	 // EndDialog()�� ȣ������ �ʾ����Ƿ� ��� ���̾�α״� ��µ� ����
			}
			if (strlen(sID) > 10)
			{
				MessageBox(0, "ID�� 10���� �̳������մϴ�", "����", MB_OK);
				SetDlgItemText(hDlg, IDC_EDIT1, "");
				SetFocus(GetDlgItem(hDlg, IDC_EDIT1));
				return (INT_PTR)TRUE;	
			}
			if (strlen(sNick) > 20)
			{
				MessageBox(0, "����� 20���� �̳������մϴ�", "����", MB_OK);
				SetDlgItemText(hDlg, IDC_EDIT4, "");
				SetFocus(GetDlgItem(hDlg, IDC_EDIT4));
				return (INT_PTR)TRUE;
			}
			if (strlen(PS1) > 14)
			{
				MessageBox(0, "Password�� 14���� �̳������մϴ�", "����", MB_OK);
				SetDlgItemText(hDlg, IDC_EDIT2, "");
				SetDlgItemText(hDlg, IDC_EDIT3, "");
				SetFocus(GetDlgItem(hDlg, IDC_EDIT2));
				return (INT_PTR)TRUE;
			}
			len = strlen(sID);
			for (int i = 0; i < len; i++)
			{
				if (sID[i] == ' ')//������ �������
				{
					MessageBox(0, "���̵� �߸��Ǿ����ϴ�(�پ��Ұ�)", "����", MB_OK);
					SetDlgItemText(hDlg, IDC_EDIT1, "");
					SetFocus(GetDlgItem(hDlg, IDC_EDIT1));
					return (INT_PTR)TRUE;
				}
				if (((sID[i] > 64 && sID[i] < 91) || (sID[i] > 96 && i < sID[i]) || (sID[i] > 47 && i < sID[i])))
				{
				}
				else{
					MessageBox(0, "���̵� �߸��Ǿ����ϴ�(���� ���ڸ� ����)", "����", MB_OK);
					SetDlgItemText(hDlg, IDC_EDIT1, "");
					SetFocus(GetDlgItem(hDlg, IDC_EDIT1));
					return (INT_PTR)TRUE;
				}
				
			}
			len = strlen(sNick);
			for (int i = 0; i < len; i++)
			{
				if (sNick[i] == ' ')
				{
					MessageBox(0, "������ �߸��Ǿ����ϴ�(�پ��Ұ�)", "����", MB_OK);
					SetDlgItemText(hDlg, IDC_EDIT4, "");
					SetFocus(GetDlgItem(hDlg, IDC_EDIT4));
					return (INT_PTR)TRUE;
				}
				if ((sNick[i] == '/') || (sNick[i] == '\\'))
				{
					MessageBox(0, "/��\\�� ���Ұ��Դϴ�", "����", MB_OK);
					SetDlgItemText(hDlg, IDC_EDIT4, "");
					SetFocus(GetDlgItem(hDlg, IDC_EDIT4));
					return (INT_PTR)TRUE;
				}

			} 
			
			header("1");

			for (int i = 0; i < 3; i++)//�α������� ������//
			{
				GetDlgItemText(hDlg, hIDC_EDIT[i], string, 99);
				len = strlen(string);
				string[len++] = '\0';
				retval = send(sOck, string, len, 0);
				if (retval == SOCKET_ERROR) err_quit("ȸ�������� ������ ���߽��ϴ�.");
				memset(string, 0, len);
			}
			
			retval = recvline(sOck, string, 99);//ȸ�����Լ����̶�� �޼�������	
			MessageBox(hDlg, string , "����", MB_OK);
			EndDialog(hDlg, LOWORD(wParam));
			DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG1), hDlg, Login_Menu);//�α��θ޴���
			//ȸ�������� �α������� �Ѿ��
			//DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG1), hDlg, Login_Menu);
			return (INT_PTR)TRUE;
		}
		if (LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
// Message handler for about box.
INT_PTR CALLBACK Login_Menu(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	char string[100];
	int retval;
	int len;
	int hIDC_EDIT[2] = { IDC_EDIT1, IDC_EDIT2};

	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		hLogin = hDlg;
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDC_BUTTON1)
		{
			EndDialog(hDlg, LOWORD(wParam));
			DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG2), hDlg, Join);
			return (INT_PTR)TRUE;
		}
		if (LOWORD(wParam) == IDOK)
		{
			//�α������ؼ� �´ٸ� ä�ù�//
			header("2");
			for (int i = 0; i < 2; i++)//�α������� ������//
			{
				GetDlgItemText(hDlg, hIDC_EDIT[i], string, 99);
				len = strlen(string);
				string[len++] = '\0';
				retval = send(sOck, string, len, 0);
				if (retval == SOCKET_ERROR) err_quit("�α��������� ������ ���߽��ϴ�.");
				memset(string, 0, len);
			}
			retval = recvline(sOck, string, 99);
			if (retval == SOCKET_ERROR) err_quit("�α��������� ���� ���߽��ϴ�.");
			if (string[0] == '1')
			{
				EndDialog(hDlg, LOWORD(wParam));
				DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG4), hDlg, WaittingRoom);
				//DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG3), hDlg, ChattingRoom);
			}
			if (string[0] == '3')
			{
				MessageBox(NULL, "�ߺ��α����߽��ϴ�", "�ٽ��Է����ּ���", MB_OK);
				SetDlgItemText(hDlg, IDC_EDIT1, "");
				SetDlgItemText(hDlg, IDC_EDIT2, "");
				SetFocus(GetDlgItem(hDlg, IDC_EDIT1));
			}
			if (string[0] == '2')
			{
				MessageBox(NULL, "�߸��� ���̵�� ��й�ȣ", "�ٽ��Է����ּ���", MB_OK);
				SetDlgItemText(hDlg, IDC_EDIT1, "");
				SetDlgItemText(hDlg, IDC_EDIT2, "");
				SetFocus(GetDlgItem(hDlg, IDC_EDIT1));
			}
			memset(string, 0, 100);
			return (INT_PTR)TRUE;
		}
		if (LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
INT_PTR CALLBACK WaittingRoom(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HWND hListBox;
	static HWND Member;
	static HWND ID;
	static HWND User;
	char string[100];
	int len;
	int retval;
	
	int cnt; 
	
	switch (message)
	{
	case WM_INITDIALOG:

		hWl = hDlg;
		hListBox = GetDlgItem(hDlg, IDC_LIST1);
		Member = GetDlgItem(hDlg, IDC_LIST2);
		ID = GetDlgItem(hDlg, IDC_LIST3);
		User = GetDlgItem(hDlg, IDC_LIST4);
		hWattingRoom = hListBox;
		hWattingRoomUser = Member;
		hID = ID;
		hUser = User;

		hThread = CreateThread(NULL, 0, ThreadRefresh, (LPVOID)sOck, 0, NULL);
		header("a");
		return (INT_PTR)TRUE;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDC_BUTTON3)//�游���
		{
			TerminateThread(hThread, 0);
			if (WaitForSingleObject(hThread, 1000) == WAIT_FAILED)
				return (INT_PTR)TRUE;

			EndDialog(hDlg, 0);
			DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG5), hDlg, CreateRoom);
			return (INT_PTR)TRUE;
		}
		if (LOWORD(wParam) == IDC_BUTTON1)//���ΰ�ħ
		{
			header("a");
			return (INT_PTR)TRUE;
		}
		if (LOWORD(wParam) == IDC_BUTTON4)//����ٲٱ�
		{
			TerminateThread(hThread, 0);
			if (WaitForSingleObject(hThread, 1000) == WAIT_FAILED)
				return (INT_PTR)TRUE;
			EndDialog(hDlg, 0);
			DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG6), hDlg, ChangeNick);
			return (INT_PTR)TRUE;
		}
		if (LOWORD(wParam) == IDOK)//�����
		{
			int Index;
			Index = ListBox_GetCurSel(hListBox);
			if ( Index >= 0)
			{
				header("6");
				itoa(rID[Index], string, 10);
				len = strlen(string);
				string[len++] = '\0';
				retval = send(sOck, string, len, 0);
				if (retval == SOCKET_ERROR) err_quit("���������� ������ ���߽��ϴ�.");
	
				TerminateThread(hThread, 0);
				if (WaitForSingleObject(hThread, 1000) == WAIT_FAILED)
					return (INT_PTR)TRUE;
				EndDialog(hDlg, 0);
				DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG3), hDlg, ChattingRoom);
				return (INT_PTR)TRUE;
			}	
			return (INT_PTR)TRUE;
			
		}
		if (LOWORD(wParam) == IDCANCEL)
		{
			TerminateThread(hThread, 0);
			if (WaitForSingleObject(hThread, 1000) == WAIT_FAILED)
				return (INT_PTR)TRUE;
			header("e");
			EndDialog(hDlg, 0);
			DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG1), hLogin, Login_Menu);
			return (INT_PTR)TRUE;
		}
		break;
	case WM_CLOSE:
		TerminateThread(hThread, 0);
		if (WaitForSingleObject(hThread, 1000) == WAIT_FAILED)
			return (INT_PTR)TRUE;
		header("e");
		EndDialog(hDlg, 0);
		DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG1), hLogin, Login_Menu);
		return (INT_PTR)TRUE;
	}
	return (INT_PTR)FALSE;
}
INT_PTR CALLBACK ChattingRoom(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HWND hNickname;
	char string[100];
	char sendm[100];
	int retval;
	int len;
	int cnt;
	FILE *fp;	
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		
		header("7");
		hCht = hDlg;
		hNickname = GetDlgItem(hDlg, IDC_LIST2);
		hSystem = hNickname;
		FileResponseCheck = FALSE;
		retval = recvline(sOck, string, 99);//������� ����
		cnt = atoi(string);
		for (int i = 0; i < cnt; i++)
		{
			retval = recvline(sOck, string, 99);//������� ����
			ListBox_AddString(hNickname, string);
		}
		hEdit1 = GetDlgItem(hDlg, IDC_EDIT1);
		hEdit2 = GetDlgItem(hDlg, IDC_EDIT2);
		hThread = CreateThread(NULL, 0, ThreadRecv, (LPVOID)sOck, 0, NULL);
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if ((LOWORD(wParam) == IDC_BUTTON3))//�α�����
		{
			header("c");
			return (INT_PTR)TRUE;
		}
		if ((LOWORD(wParam) == IDC_BUTTON2))//�α�����
		{
			//Edit_SetSel(hEdit1, 0, -1);
			Edit_SetText(hEdit1, "");
			
			return (INT_PTR)TRUE;
		}
		if ((LOWORD(wParam) == IDC_BUTTON1))//��������
		{
			FileResponseCheck = FALSE;
			FileSend = FALSE;
			int Index;
			Index = ListBox_GetCurSel(hNickname);
			if (Index >= 0)
			{
				header("b");

				itoa(Index, string, 10);
				len = strlen(string);
				string[len++] = '\0';
				retval = send(sOck, string, len, 0);
				if (retval == SOCKET_ERROR) err_quit("������ ������ ���߽��ϴ�.");
			}
			return (INT_PTR)TRUE;
		}
		if ((LOWORD(wParam) == IDOK))
		{
			GetDlgItemText(hDlg, IDC_EDIT2, string, 99);
			if (string[0] == '\0')
			{
				return (INT_PTR)TRUE;
			}
			header("5");
			
			//GetDlgItemText(hDlg, IDC_EDIT2, sendm, 99);

			len = strlen(string);
			string[len++] = '\0';

			retval = send(sOck, string, len, 0);
			if (retval == SOCKET_ERROR) err_quit("ä�������� ���������߽��ϴ�");
			SetDlgItemText(hDlg, IDC_EDIT2, "");
			memset(string, 0, 100);

			return (INT_PTR)TRUE;
		}

		if (LOWORD(wParam) == IDCANCEL)
		{
			TerminateThread(hThread, 0);
			if (WaitForSingleObject(hThread, 1000) == WAIT_FAILED)
				return (INT_PTR)TRUE;
			//�����ٴ� �޼����� ������.. dequeue�� �ؾ���
			header("8");
			EndDialog(hDlg, LOWORD(wParam));
			DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG4), hDlg, WaittingRoom);
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK CreateRoom(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	char string[100];
	int retval;
	int len;

	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		
		return (INT_PTR)TRUE;

	case WM_COMMAND:

		if (LOWORD(wParam) == IDOK)
		{
			header("4");

			GetDlgItemText(hDlg, IDC_EDIT1, string, 99);//���̸��Է�
			len = strlen(string);
			string[len++] = '\0';
			retval = send(sOck, string, len, 0);//���̸�����
			if (retval == SOCKET_ERROR) err_quit("���������� ������ ���߽��ϴ�.");
			EndDialog(hDlg, LOWORD(wParam));
			DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG3), hDlg, ChattingRoom);
			return (INT_PTR)TRUE;
		}
		if (LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG4), hWl, WaittingRoom);
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;



}

INT_PTR CALLBACK ChangeNick(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	char string[100];
	int retval;
	int len;

	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK)
		{
			header("9");

			GetDlgItemText(hDlg, IDC_EDIT1, string, 99);//�ٲܺ����Է�
			len = strlen(string);
			string[len++] = '\0';
			retval = send(sOck, string, len, 0);//����
			if (retval == SOCKET_ERROR) err_quit("���������� ������ ���߽��ϴ�.");

			retval = recvline(sOck, string, BUFSIZE);
			if (string[0] == '*'){
				MessageBox(hDlg, string, "����", MB_ICONERROR);
				SetDlgItemText(hDlg, IDC_EDIT1, "");
				SetFocus(GetDlgItem(hDlg, IDC_EDIT1));
				return (INT_PTR)TRUE;
			}
				
			else
				MessageBox(hDlg, string, "����", MB_OK);
			//recv �Ϸ� or ����se
			EndDialog(hDlg, LOWORD(wParam));
			DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG4), hWl, WaittingRoom);
			return (INT_PTR)TRUE;
		}
		if (LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG4), hWl, WaittingRoom);
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;


}
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
void DisplayText(char *fmt, ...)
{
	va_list arg;
	va_start(arg, fmt);

	char cbuf[BUFSIZE + 256];
	vsprintf(cbuf, fmt, arg);

	int nLength = GetWindowTextLength(hEdit1);
	SendMessage(hEdit1, EM_SETSEL, nLength, nLength);
	SendMessage(hEdit1, EM_REPLACESEL, FALSE, (LPARAM)cbuf);

	va_end(arg);


}
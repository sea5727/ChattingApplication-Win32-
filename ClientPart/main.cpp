/*------------------------------------------------
윈도우 기본 프로그래밍
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

/////////////전역변수//////////////////
bool FileResponseCheck;
bool FileSend;
SOCKET sOck;
SOCKADDR_IN serveraddress;
HINSTANCE hInst;
HWND hWl,hCht;
HWND hEdit1, hEdit2;//채팅방의 핸들
HWND hSystem;//쓰레드의 시스템핸들
HWND hLogin,hWattingRoom, hWattingRoomUser,hID, hUser;
HANDLE hThread;
int *rID = (int*)malloc(10*sizeof(int));
/////////////전역변수//////////////////
///////////////함수////////////////////
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	Join(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	Login_Menu(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	ChattingRoom(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	WaittingRoom(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	CreateRoom(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	ChangeNick(HWND, UINT, WPARAM, LPARAM);

int recvline(SOCKET , char*, int );
int _recv_ahead(SOCKET , char *);
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
	//파일 끝으로 위치 옮김
	fseek(fp, 0L, SEEK_END);
	//파일 바이트값 출력
	files.byte = ftell(fp);
	//다시 파일 처음으로 위치 옮김
	fseek(fp, 0L, SEEK_SET);


	// 데이터 통신에 사용할 변수
	char buf[BUFSIZE];

	//파일 기본 정보 전송
	retval = send(sock, (char*)&files, sizeof(files), 0);
	if (retval == SOCKET_ERROR) { err_display("send()"); exit(1); }

	char str[100];
	retval = recvline(sock, str, 100);
	if (str[0] == 'y')
	{
		//printf("이번파일은 %s, %d 바이트 입니다.\n", files.name, files.byte);
		unsigned int per;
		per = count = files.byte / BUFSIZE;

		while (count)
		{
			//system("cls");
			//printf("전송하는 파일 : %s, 전송하는 파일 크기 : %d Byte\n", files.name, files.byte);

			//파일 읽어서 버퍼에 저장
			fread(buf, 1, BUFSIZE, fp);

			//전송
			retval = send(sock, buf, BUFSIZE, 0);
			if (retval == SOCKET_ERROR){ err_display("send()"); exit(1); }

			//printf("\n진행도 : %d %%", (per - count) * 100 / per);

			count--;

		}

		//남은 파일 크기 만큼 나머지 전송
		count = files.byte - (per*BUFSIZE);
		fread(buf, 1, count, fp);
		retval = send(sock, buf, BUFSIZE, 0);
		if (retval == SOCKET_ERROR){ err_display("send()"); exit(1); }

		//system("cls");
		//printf("전송하는 파일 :%s , 전송하는 파일 크기 : %d Byte\n", files.name, files.byte);
		//printf("\n 진행도 : 100%%");
		//printf("\n\n전송이 완료되었습니다.");

		fclose(fp);
		MessageBox(hCht, "파일전송이 완료되었습니다", "정보", MB_OK);
	}
	else//n
	{
		fclose(fp);
		MessageBox(0, "상대방의 파일이름이 중복되었습니다", "정보", MB_OK);
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
		//printf("같은이름 파일이 없으므로 전송을 진행합니다\n");
		//printf("파일전송을 받습니다\n");
		//printf("전송하는 파일 : %s, 전송하는 파일 크기 : %d Byte\n", files.name, files.byte);
		//printf("\n클라이언트로부터 파일을 받는 중입니다.\n");
		fp = fopen(files.name, "wb");

		count = files.byte / BUFSIZE;

		while (count)
		{
			//반기
			retval = recvn(client_sock, buf, BUFSIZE, 0);
			if (retval == SOCKET_ERROR){ err_display("recv()"); exit(1); }
			//파일 작성 작업
			fwrite(buf, 1, BUFSIZE, fp);
			count--;

		}
		//남은 파일 크기만큼 나머지 받기
		count = files.byte - ((files.byte / BUFSIZE) * BUFSIZE);

		retval = recvn(client_sock, buf, BUFSIZE, 0);
		if (retval == SOCKET_ERROR){ err_display("recv()"); exit(1); }

		fwrite(buf, 1, count, fp);

		//파일포인터 닫기
		fclose(fp);
		//printf("\n파일전송이 완료되었습니다\n");
		MessageBox(hCht, "파일전송이 완료되었습니다", "정보", MB_OK);


	}
	else
	{
		//system("cls");
		header("n");

		MessageBox(0, "이미 같은 이름의 파일이 존재합니다", "정보", MB_OK);
		
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
		if (retval == SOCKET_ERROR) err_quit("채팅정보를 받지 못했습니다.");
		retval = recvline(sOck, buf, 99);
		if (retval == SOCKET_ERROR) err_quit("채팅정보를 받지 못했습니다.");

		if (Nick[0] == '/')//시스템요청일수도있으니 새로운구문
		{
			switch (Nick[1])
			{
			case 'a':
				ListBox_AddString(hSystem, buf);
				sprintf(chatting, "%s : %s님이 참가하셨습니다.", Nick, buf);
				DisplayText("%s\n", chatting);
				break;
			case 'b':
				n = ListBox_FindString(hSystem, 0, buf);
				ListBox_DeleteString(hSystem, n);
				sprintf(chatting, "%s : %s님이 퇴장하셨습니다.", Nick, buf, n);
				DisplayText("%s\n", chatting);
				break;
			case 'c':
				FileSend = FALSE;
				FileResponseCheck = TRUE;
				
				sprintf(chatting, "%s : %s", Nick, buf);
				MessageBox(0, chatting, "정보", MB_ICONERROR);
				
				break;
			case 'd':
				header("d");
				//버퍼는 닉정보를 포함중이다
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
					ofn.lpstrFilter = "모든 파일(*.*)\0*.*\0\0";
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
							//MessageBox(hCht, "없는파일입니다. 다시 선택해주세요", "에러", MB_ICONERROR);

							FILE *fp = fopen(szFullPath, "rb");
							if (fp == NULL)
								MessageBox(hCht, szFullPath, "에러", MB_ICONERROR);
							else
							{
								sprintf(name, "%s%s", szFname, szExt);
								header("y");
								sendfile(sOck, ofn, name);
								break;
							}
						}
						else //파일보내는창까지 열고 취소할경우
						{
							header("n");
							break;
						}
					}

				}
				
				break;
			case 'e':
				sprintf(chatting, "%s : %s에게 파일전송을 받으시겠습니까?", Nick, buf);
				answer = MessageBox(hCht, chatting, "정보", MB_YESNO);
				if (answer == IDYES){
					header("fy");
					len = strlen(buf);
					buf[len++] = '\0';
					retval = send(sOck, buf, len, 0);

					retval = recvline(sOck, buf,5);
					if (buf[0] == 'y')
						recvfile(sOck);
					else//n
						MessageBox(hCht, "상대방이 취소했습니다", "정보", MB_OK);
						//걍다시 채팅으로
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
				MessageBox(hCht, "상대방이 전송을 거부했습니다", "정보", MB_OK);
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
			if (retval == SOCKET_ERROR) err_quit("대기방정보를 받지 못했습니다.");
			cnt = atoi(string);
			if (cnt != 0)	{
				for (int i = 0; i < cnt; i++){
					retval = recvline(sock, string, 99);
					if (retval == SOCKET_ERROR) err_quit("대기방정보를 받지 못했습니다.");
					ListBox_AddString(hWattingRoom, string);
					retval = recvline(sock, string, 99);
					if (retval == SOCKET_ERROR) err_quit("대기방정보를 받지 못했습니다.");
					ListBox_AddString(hWattingRoomUser, string);
					retval = recvline(sock, string, 99);
					if (retval == SOCKET_ERROR) err_quit("대기방정보를 받지 못했습니다.");
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
///////////////함수////////////////////
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

	// 윈속 초기화
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

	HWND hWnd = CreateWindow("basic", "컴퓨터네트워크 텀프로젝트", WS_OVERLAPPEDWINDOW,
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

		case IDM_LOGIN: //로그인
			DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG1), hWnd, Login_Menu);
			break;
		case IDM_EXIT:
			MessageBox(hWnd, "안녕히가세요", "종료", MB_OK);
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
				MessageBox(0, "아이디를 입력하세요", "에러", MB_OK);
				return (INT_PTR)TRUE;
			}
			if (sNick[0] == '\0')
			{
				MessageBox(0, "별명을 입력하세요", "에러", MB_OK);
				return (INT_PTR)TRUE;
			}
			if (PS1[0] == '\0')
			{
				MessageBox(0, "비밀번호를 입력하세요", "에러", MB_OK);
				return (INT_PTR)TRUE;
			}
			if (strcmp(PS1, PS2) != 0)
			{
				MessageBox(0, "비밀번호가 다릅니다", "에러", MB_OK);
				SetDlgItemText(hDlg, IDC_EDIT2, "");
				SetDlgItemText(hDlg, IDC_EDIT3, "");
				SetFocus(GetDlgItem(hDlg, IDC_EDIT2));
				return (INT_PTR)TRUE;	 // EndDialog()를 호출하지 않았으므로 계속 다이얼로그는 출력된 상태
			}
			if (strlen(sID) > 10)
			{
				MessageBox(0, "ID는 10글자 이내여야합니다", "에러", MB_OK);
				SetDlgItemText(hDlg, IDC_EDIT1, "");
				SetFocus(GetDlgItem(hDlg, IDC_EDIT1));
				return (INT_PTR)TRUE;	
			}
			if (strlen(sNick) > 20)
			{
				MessageBox(0, "별명는 20글자 이내여야합니다", "에러", MB_OK);
				SetDlgItemText(hDlg, IDC_EDIT4, "");
				SetFocus(GetDlgItem(hDlg, IDC_EDIT4));
				return (INT_PTR)TRUE;
			}
			if (strlen(PS1) > 14)
			{
				MessageBox(0, "Password는 14글자 이내여야합니다", "에러", MB_OK);
				SetDlgItemText(hDlg, IDC_EDIT2, "");
				SetDlgItemText(hDlg, IDC_EDIT3, "");
				SetFocus(GetDlgItem(hDlg, IDC_EDIT2));
				return (INT_PTR)TRUE;
			}
			len = strlen(sID);
			for (int i = 0; i < len; i++)
			{
				if (sID[i] == ' ')//공백이 있을경우
				{
					MessageBox(0, "아이디가 잘못되었습니다(뛰어쓰기불가)", "에러", MB_OK);
					SetDlgItemText(hDlg, IDC_EDIT1, "");
					SetFocus(GetDlgItem(hDlg, IDC_EDIT1));
					return (INT_PTR)TRUE;
				}
				if (((sID[i] > 64 && sID[i] < 91) || (sID[i] > 96 && i < sID[i]) || (sID[i] > 47 && i < sID[i])))
				{
				}
				else{
					MessageBox(0, "아이디가 잘못되었습니다(영어 숫자만 가능)", "에러", MB_OK);
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
					MessageBox(0, "별명이 잘못되었습니다(뛰어쓰기불가)", "에러", MB_OK);
					SetDlgItemText(hDlg, IDC_EDIT4, "");
					SetFocus(GetDlgItem(hDlg, IDC_EDIT4));
					return (INT_PTR)TRUE;
				}
				if ((sNick[i] == '/') || (sNick[i] == '\\'))
				{
					MessageBox(0, "/및\\는 사용불가입니다", "에러", MB_OK);
					SetDlgItemText(hDlg, IDC_EDIT4, "");
					SetFocus(GetDlgItem(hDlg, IDC_EDIT4));
					return (INT_PTR)TRUE;
				}

			} 
			
			header("1");

			for (int i = 0; i < 3; i++)//로그인정보 보내기//
			{
				GetDlgItemText(hDlg, hIDC_EDIT[i], string, 99);
				len = strlen(string);
				string[len++] = '\0';
				retval = send(sOck, string, len, 0);
				if (retval == SOCKET_ERROR) err_quit("회원정보를 보내지 못했습니다.");
				memset(string, 0, len);
			}
			
			retval = recvline(sOck, string, 99);//회원가입성공이라는 메세지받음	
			MessageBox(hDlg, string , "정보", MB_OK);
			EndDialog(hDlg, LOWORD(wParam));
			DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG1), hDlg, Login_Menu);//로그인메뉴로
			//회원가입후 로그인으로 넘어가자
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
			//로그인을해서 맞다면 채팅방//
			header("2");
			for (int i = 0; i < 2; i++)//로그인정보 보내기//
			{
				GetDlgItemText(hDlg, hIDC_EDIT[i], string, 99);
				len = strlen(string);
				string[len++] = '\0';
				retval = send(sOck, string, len, 0);
				if (retval == SOCKET_ERROR) err_quit("로그인정보를 보내지 못했습니다.");
				memset(string, 0, len);
			}
			retval = recvline(sOck, string, 99);
			if (retval == SOCKET_ERROR) err_quit("로그인정보를 받지 못했습니다.");
			if (string[0] == '1')
			{
				EndDialog(hDlg, LOWORD(wParam));
				DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG4), hDlg, WaittingRoom);
				//DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG3), hDlg, ChattingRoom);
			}
			if (string[0] == '3')
			{
				MessageBox(NULL, "중복로그인했습니다", "다시입력해주세요", MB_OK);
				SetDlgItemText(hDlg, IDC_EDIT1, "");
				SetDlgItemText(hDlg, IDC_EDIT2, "");
				SetFocus(GetDlgItem(hDlg, IDC_EDIT1));
			}
			if (string[0] == '2')
			{
				MessageBox(NULL, "잘못된 아이디와 비밀번호", "다시입력해주세요", MB_OK);
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
		if (LOWORD(wParam) == IDC_BUTTON3)//방만들기
		{
			TerminateThread(hThread, 0);
			if (WaitForSingleObject(hThread, 1000) == WAIT_FAILED)
				return (INT_PTR)TRUE;

			EndDialog(hDlg, 0);
			DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG5), hDlg, CreateRoom);
			return (INT_PTR)TRUE;
		}
		if (LOWORD(wParam) == IDC_BUTTON1)//새로고침
		{
			header("a");
			return (INT_PTR)TRUE;
		}
		if (LOWORD(wParam) == IDC_BUTTON4)//별명바꾸기
		{
			TerminateThread(hThread, 0);
			if (WaitForSingleObject(hThread, 1000) == WAIT_FAILED)
				return (INT_PTR)TRUE;
			EndDialog(hDlg, 0);
			DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG6), hDlg, ChangeNick);
			return (INT_PTR)TRUE;
		}
		if (LOWORD(wParam) == IDOK)//방들어가기
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
				if (retval == SOCKET_ERROR) err_quit("대기방정보를 보내지 못했습니다.");
	
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
		retval = recvline(sOck, string, 99);//멤버수를 받음
		cnt = atoi(string);
		for (int i = 0; i < cnt; i++)
		{
			retval = recvline(sOck, string, 99);//멤버수를 받음
			ListBox_AddString(hNickname, string);
		}
		hEdit1 = GetDlgItem(hDlg, IDC_EDIT1);
		hEdit2 = GetDlgItem(hDlg, IDC_EDIT2);
		hThread = CreateThread(NULL, 0, ThreadRecv, (LPVOID)sOck, 0, NULL);
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if ((LOWORD(wParam) == IDC_BUTTON3))//로그전송
		{
			header("c");
			return (INT_PTR)TRUE;
		}
		if ((LOWORD(wParam) == IDC_BUTTON2))//로그지움
		{
			//Edit_SetSel(hEdit1, 0, -1);
			Edit_SetText(hEdit1, "");
			
			return (INT_PTR)TRUE;
		}
		if ((LOWORD(wParam) == IDC_BUTTON1))//파일전송
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
				if (retval == SOCKET_ERROR) err_quit("정보를 보내지 못했습니다.");
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
			if (retval == SOCKET_ERROR) err_quit("채팅전송을 보내지못했습니다");
			SetDlgItemText(hDlg, IDC_EDIT2, "");
			memset(string, 0, 100);

			return (INT_PTR)TRUE;
		}

		if (LOWORD(wParam) == IDCANCEL)
		{
			TerminateThread(hThread, 0);
			if (WaitForSingleObject(hThread, 1000) == WAIT_FAILED)
				return (INT_PTR)TRUE;
			//나간다는 메세지를 보내줌.. dequeue를 해야함
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

			GetDlgItemText(hDlg, IDC_EDIT1, string, 99);//방이름입력
			len = strlen(string);
			string[len++] = '\0';
			retval = send(sOck, string, len, 0);//방이름보냄
			if (retval == SOCKET_ERROR) err_quit("대기방정보를 보내지 못했습니다.");
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

			GetDlgItemText(hDlg, IDC_EDIT1, string, 99);//바꿀병명입력
			len = strlen(string);
			string[len++] = '\0';
			retval = send(sOck, string, len, 0);//센드
			if (retval == SOCKET_ERROR) err_quit("별명정보를 보내지 못했습니다.");

			retval = recvline(sOck, string, BUFSIZE);
			if (string[0] == '*'){
				MessageBox(hDlg, string, "정보", MB_ICONERROR);
				SetDlgItemText(hDlg, IDC_EDIT1, "");
				SetFocus(GetDlgItem(hDlg, IDC_EDIT1));
				return (INT_PTR)TRUE;
			}
				
			else
				MessageBox(hDlg, string, "정보", MB_OK);
			//recv 완료 or 실패se
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
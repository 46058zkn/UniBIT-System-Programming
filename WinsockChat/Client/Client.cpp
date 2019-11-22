/*
 * Курсова работа по "Системно програмиране" от Красимир Банчев, Фак. №46058зкн
 * Winsock 2.0 клиент/сървър приложение за мрежов чат
 * C++ Win32 API графичен клиент v. 0.9
 */

#ifndef UNICODE
#define UNICODE
#endif

#include <WS2tcpip.h>
#include <windows.h>
#include <winuser.h>
#include <vector>
#include <string>
#include <process.h>
#include <comdef.h>
#include <cstdlib>
#include <tchar.h>
#include <cwchar>
#include <fstream>

#pragma comment(lib, "ws2_32.lib" )
#pragma comment(lib,"User32.lib")
#pragma warning(disable : 4996)
#pragma execution_character_set("utf-8")

#define ID_EDIT 100
#define ID_EDITCHILD1 101
#define ID_EDITCHILD2 202
#define ID_BUTTON 103

WSADATA wsa_data;
int c_result;
sockaddr_in c_address;
SOCKET client_connect, client_socket;
char* c_name = nullptr;
const char* ip = nullptr;
int port = 0;
bool network_success = false;
bool socket_success = false;
bool username_success = false;
unsigned int __stdcall  Client_Thread(void* p);
LRESULT CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK LoginWndProc(HWND, UINT, WPARAM, LPARAM);
void Append_New_Message_To_Chat(wchar_t* new_text);
void Set_Username_To_Window_Title(wchar_t* new_title);
wchar_t* Convert_Message_To_Unicode(char* message);
int Startup_Check();

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	PWSTR lpCmdLine, int nCmdShow) {
	MSG  sys_messages;
	WNDCLASSW main_window_class = { 0 };
	main_window_class.lpszClassName = L"MainWindow";
	main_window_class.hInstance = hInstance;
	main_window_class.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
	main_window_class.lpfnWndProc = MainWndProc;
	main_window_class.hCursor = LoadCursor(0, IDC_ARROW);

	RegisterClassW(&main_window_class);
	CreateWindowW(main_window_class.lpszClassName, L"Winsock Чат клиент",
		WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE,
		120, 120, 597, 500, 0, 0, hInstance, 0);

	Startup_Check();

	if (network_success == true && socket_success == true)
	{
		CreateThread(0, 0, LPTHREAD_START_ROUTINE(Client_Thread), 0, 0, 0);
	}

	WNDCLASSW login_windows_class = { 0 };
	login_windows_class.lpszClassName = L"LoginWindow";
	login_windows_class.hInstance = hInstance;
	login_windows_class.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
	login_windows_class.lpfnWndProc = LoginWndProc;
	login_windows_class.hCursor = LoadCursor(0, IDC_ARROW);

	RegisterClassW(&login_windows_class);
	CreateWindow(login_windows_class.lpszClassName, L"Как се казвате?",
		WS_CAPTION | WS_VISIBLE | WS_POPUP,
		270, 290, 270, 100, 0, 0, hInstance, 0);

	while (GetMessage(&sys_messages, NULL, 0, 0)) {
		TranslateMessage(&sys_messages);
		DispatchMessage(&sys_messages);
	}

	return int(sys_messages.wParam);
}

LRESULT CALLBACK LoginWndProc(HWND hWnd, UINT msg,
	WPARAM wParam, LPARAM lParam) {
	PAINTSTRUCT ps;
	HDC hdc;
	TCHAR greeting[] = _T("Моля, въведете името си:");

	static HWND hwnd_login;
	HWND hwnd_enter;

	switch (msg) {
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		TextOut(hdc,
			5, 5,
			greeting, _tcslen(greeting));
		EndPaint(hWnd, &ps);
		break;

	case WM_CREATE:

		if (network_success == false)
		{
			DestroyWindow(hWnd);
		}

		hwnd_login = CreateWindowW(L"Edit", NULL,
			WS_CHILD | WS_VISIBLE,
			5, 30, 160, 25, hWnd, (HMENU)ID_EDIT,
			NULL, NULL);

		hwnd_enter = CreateWindowW(L"Button", L"Влез",
			WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON | BS_FLAT, 170, 30, 80, 25,
			hWnd, (HMENU)ID_BUTTON, NULL, NULL);

		break;

	case WM_COMMAND:

		if (HIWORD(wParam) == BN_CLICKED) {
			const int len = GetWindowTextLengthW(hwnd_login) + 1;
			const auto text = new wchar_t[len];
			GetWindowTextW(hwnd_login, text, len);

			const char* empty = "";
			const _bstr_t nickname(text);
			c_name = nickname;

			if (strcmp(c_name, empty) != 0)
			{
				c_result = send(client_connect, c_name, 256, NULL);

				if (c_result == SOCKET_ERROR)
				{
					MessageBoxW(NULL, _T("Сървърът е недостъпен!"), _T("Грешка при свързване!"), MB_ICONERROR | MB_OK);
					closesocket(client_connect);
					WSACleanup();
					socket_success = false;
					return WSAGetLastError();
				}

				username_success = true;
				Set_Username_To_Window_Title(text);
				DestroyWindow(hWnd);
			}
		}

		break;

	default:;
	}

	return DefWindowProcW(hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg,
	WPARAM wParam, LPARAM lParam) {
	PAINTSTRUCT ps;
	HDC hdc;
	TCHAR greeting[] = _T("Курсова работа по \"Системно програмиране\" от Красимир Банчев, Фак. №46058зкн");

	static HWND hwnd_edit, hwnd_messages;
	HWND hwnd_button;

	TCHAR disclaimer[] = L"\r\n	Winsock 2.0 клиент/сървър приложение за мрежов чат.\r\n\r\n";

	switch (msg) {
	case WM_PAINT:
		hdc = BeginPaint(hwnd, &ps);
		TextOut(hdc,
			5, 5,
			greeting, _tcslen(greeting));
		EndPaint(hwnd, &ps);
		break;

	case WM_CREATE:
		hwnd_messages = CreateWindowW(L"Edit", NULL,
			WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER |
			ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL,
			5, 30, 571, 400, hwnd, (HMENU)ID_EDITCHILD1,
			NULL, NULL);

		SendMessageW(hwnd_messages, WM_SETTEXT, 0, (LPARAM)disclaimer);

		hwnd_edit = CreateWindowW(L"Edit", NULL,
			WS_CHILD | WS_VISIBLE,
			5, 435, 485, 25, hwnd, (HMENU)ID_EDITCHILD2,
			NULL, NULL);

		hwnd_button = CreateWindowW(L"Button", L"Изпрати",
			WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON | BS_FLAT, 495, 435, 80, 25,
			hwnd, (HMENU)ID_BUTTON, NULL, NULL);

		break;

	case WM_COMMAND:

		if (HIWORD(wParam) == BN_CLICKED) {
			if (network_success == true && socket_success == true && username_success == true)
			{
				char* chat_msg = new char[256];
				SecureZeroMemory(chat_msg, 256);

				const int len = GetWindowTextLengthW(hwnd_edit) + 1;
				wchar_t* const text = new wchar_t[len];
				GetWindowTextW(hwnd_edit, text, len);

				const char* empty = "";
				const _bstr_t _msg(text);
				chat_msg = _msg;

				if (strcmp(chat_msg, empty) != 0)
				{
					c_result = send(client_connect, chat_msg, 256, NULL);
					if (c_result == SOCKET_ERROR)
					{
						MessageBoxW(NULL, _T("Не може да бъде изпратено съобщение!"), _T("Грешка при изпращане!"), MB_ICONERROR | MB_OK);
						closesocket(client_connect);
						WSACleanup();
						return 1;
					}
				}

				SetWindowTextW(hwnd_edit, nullptr);
			}
			else
			{
				MessageBoxW(NULL, _T("Проверете сървъра и рестартирайте програмата!"), _T("Системна грешка!"), MB_ICONERROR | MB_OK);
			}
		}

		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:;
	}

	return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int Startup_Check()
{
	const wchar_t* filename = _T("\\Client.ini");
	wchar_t c_path[MAX_PATH];
	GetCurrentDirectoryW(MAX_PATH, c_path);

	wchar_t* path = wcscat(c_path, filename);

	wchar_t addr[128];
	GetPrivateProfileStringW(_T("host"), _T("ip"), _T("127.0.0.1"), addr, sizeof(addr), path);
	const _bstr_t _addr(addr);
	ip = _addr;

	port = GetPrivateProfileIntW(_T("host"), _T("port"), 22220, path);

	c_address.sin_family = AF_INET;
	c_address.sin_port = htons(port);
	c_address.sin_addr.S_un.S_addr = inet_addr(ip);

	network_success = true;

	c_result = WSAStartup(MAKEWORD(2, 2), &wsa_data);

	if (c_result)
	{
		MessageBoxW(NULL, _T("Winsock липсва или е стара версия!"), _T("Грешка!"), MB_ICONWARNING | MB_OK);
		network_success = false;
		return 1;
	}

	client_connect = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (client_connect == INVALID_SOCKET)
	{
		MessageBoxW(NULL, _T("Невалиден сокет!"), _T("Грешка!"), MB_ICONWARNING | MB_OK);
		WSACleanup();
		network_success = false;
		return 1;
	}

	c_result = connect(client_connect, reinterpret_cast<SOCKADDR*>(&c_address), sizeof(sockaddr_in));
	if (c_result)
	{
		MessageBoxW(NULL, _T("Сървърът е недостъпен!"), _T("Грешка при свързване!"), MB_ICONERROR | MB_OK);
		closesocket(client_connect);
		WSACleanup();
		network_success = false;
		return 1;
	}

	socket_success = true;
	return 0;
}

unsigned int __stdcall Client_Thread(void*)
{
	char* buffer = new char[256]{ 0 };

	int size = 0;
	while (true)
	{
		SecureZeroMemory(buffer, 256);
		if ((size = recv(client_connect, buffer, 256, NULL) > 0))
		{
			wchar_t* msg = Convert_Message_To_Unicode(buffer);
			Append_New_Message_To_Chat(msg);
		}
		Sleep(50);
	}
}

void Set_Username_To_Window_Title(wchar_t* new_title)
{
	const HWND& hwnd = FindWindowExW(NULL, NULL, _T("MainWindow"), NULL);

	const int len = GetWindowTextLengthW(hwnd) + 1;
	wchar_t* text = new wchar_t[len];

	GetWindowTextW(hwnd, text, len);

	const wchar_t* delimiter = L" | Потребител: ";
	text = wcscat(text, delimiter);
	text = wcscat(text, new_title);

	SetWindowTextW(hwnd, text);
}

wchar_t* Convert_Message_To_Unicode(char* message)
{
	strcat(message, "\r\n");

	const int chars_num = MultiByteToWideChar(CP_UTF8, 0, message, -1, NULL, 0);

	wchar_t* wide_chars = new wchar_t[chars_num];

	MultiByteToWideChar(CP_UTF8, UNICODE, message, -1, wide_chars, chars_num);

	return wide_chars;
}

void Append_New_Message_To_Chat(wchar_t* new_text)
{
	const HWND& hwnd = FindWindowExW(NULL, NULL, _T("MainWindow"), NULL);

	HWND hwnd_output = GetDlgItem(hwnd, ID_EDITCHILD1);

	int out_length = GetWindowTextLengthW(hwnd_output) + lstrlenW(new_text) + 1;

	std::vector<wchar_t> buf(out_length);
	wchar_t* pbuf = &buf[0];

	GetWindowTextW(hwnd_output, pbuf, out_length);

	wcscat_s(pbuf, out_length, new_text);

	SetWindowTextW(hwnd_output, pbuf);
}
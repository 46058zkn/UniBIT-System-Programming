/*
 * Курсов проект по "Системно програмиране" от Красимир Банчев, Фак. №46058зкн
 * Winsock 2.0 клиент/сървър приложение за мрежов чат
 * C++ Win32 API графичен клиент v. 1.0
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

#pragma optimize("a",on)
#pragma comment(linker, "/STACK:2000000")
#pragma comment(linker, "/HEAP:2000000")

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
const wchar_t* main_window_title = nullptr;
unsigned int __stdcall  Client_Thread(void* p);
LRESULT CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK LoginWndProc(HWND, UINT, WPARAM, LPARAM);
void Append_New_Message_To_Chat(wchar_t* new_text);
void Set_Username_To_Window_Title(wchar_t* new_title);
char* Convert_Outgoing_Message_To_MultiByte(wchar_t* message);
wchar_t* Convert_Incoming_Message_To_WideChar(char* message);
int Startup_Check();

/*
 * Създаване на инстанцията на приложението и на прозоречните класове.
 * Ако проверките на функцията Startup_Check са преминали успешно,
 * тук се стартира и нишката, "слушаща" за нови съобщения.
 */
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	PWSTR lpCmdLine, int nCmdShow) {
	MSG  sys_messages;
	WNDCLASSW main_window_class = { 0 };
	main_window_class.lpszClassName = L"MainWindow";
	main_window_class.hInstance = hInstance;
	main_window_class.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
	main_window_class.lpfnWndProc = MainWndProc;
	main_window_class.hCursor = LoadCursor(nullptr, IDC_ARROW);

	RegisterClassW(&main_window_class);
	CreateWindowW(main_window_class.lpszClassName, L"Winsock Чат клиент",
		WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE,
		120, 120, 597, 500, nullptr, nullptr, hInstance, nullptr);

	Startup_Check();

	if (network_success == true && socket_success == true)
	{
		CreateThread(nullptr, 0, LPTHREAD_START_ROUTINE(Client_Thread), nullptr, 0, nullptr);
	}

	WNDCLASSW login_windows_class = { 0 };
	login_windows_class.lpszClassName = L"LoginWindow";
	login_windows_class.hInstance = hInstance;
	login_windows_class.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
	login_windows_class.lpfnWndProc = LoginWndProc;
	login_windows_class.hCursor = LoadCursor(nullptr, IDC_ARROW);

	RegisterClassW(&login_windows_class);
	CreateWindow(login_windows_class.lpszClassName, L"Как се казвате?",
		WS_CAPTION | WS_VISIBLE | WS_POPUP,
		270, 290, 270, 100, nullptr, nullptr, hInstance, nullptr);

	while (GetMessage(&sys_messages, nullptr, 0, 0)) {
		TranslateMessage(&sys_messages);
		DispatchMessage(&sys_messages);
	}

	return int(sys_messages.wParam);
}

/*
 * "Логин" прозорец на програмата. Приема се вход с никнейма на потребителя.
 * Прозореца не реагира на празен вход. След изчитане на въведеното от
 * потребителя, то се подава на сървъра за "регистрация" и на функцията
 * Set_Username_To_Window_Title, а прозореца се затваря. Ако всичко е наред,
 * bool глобалната променлива username_success се сетва на true
 *
 */
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
					MessageBoxW(nullptr, _T("Сървърът е недостъпен!"), _T("Грешка при свързване!"), MB_ICONERROR | MB_OK);
					closesocket(client_connect);
					WSACleanup();
					socket_success = false;
					return 1;
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

/*
 * Основен прозорец на приложението. Тук се визуализира "чат диалога" и се
 * обработва потребителският вход. Съдържа два текстови контрола и един бутон.
 * Написаното от потребителя, което нативно е wchar_t се подава на функцията
 * Convert_Outgoing_Message_To_MultiByte за да се преобразува в char, след което
 * се изпраща към сървъра
 */
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
		hwnd_messages = CreateWindowW(L"Edit", L"ChatBox",
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
				const int len = GetWindowTextLengthW(hwnd_edit) + 1;
				wchar_t* const text = new wchar_t[len];
				GetWindowTextW(hwnd_edit, text, len);

				const char* empty = "";
				char* chat_msg = Convert_Outgoing_Message_To_MultiByte(text);

				if (strcmp(chat_msg, empty) != 0)
				{
					c_result = send(client_connect, chat_msg, 256, NULL);
					if (c_result == SOCKET_ERROR)
					{
						MessageBoxW(nullptr, _T("Не може да бъде изпратено съобщение!"), _T("Грешка при изпращане!"), MB_ICONERROR | MB_OK);
						closesocket(client_connect);
						WSACleanup();
						return 1;
					}
				}

				SetWindowTextW(hwnd_edit, nullptr);
			}
			else
			{
				MessageBoxW(nullptr, _T("Проверете сървъра и рестартирайте програмата!"), _T("Системна грешка!"), MB_ICONERROR | MB_OK);
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

/*
 * Функцията Startup_Check подготвя програмата за началото на нейната работа.
 * Най-напред се определя текущия път от който е стартирано приложението, от
 * който се изчислява местоположението на конфигурационния файл. След парсване
 * на същия клиента разбира на кой ip адрес и порт трябва да търси сървъра си.
 * Проверява се системата за наличие на съответната версия на winsock, при успех
 * се създава сокет и се осъществява връзка със сървъра. При неуспех на всяка
 * стъпка на потребителя се показва MessageBox със съответното съобщение.
 * При успех двете глобални bool променливи network_success и
 * socket_success се сетват на true.
 */
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
		MessageBoxW(nullptr, _T("Winsock липсва или е стара версия!"), _T("Грешка!"), MB_ICONWARNING | MB_OK);
		network_success = false;
		return 1;
	}

	client_connect = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (client_connect == INVALID_SOCKET)
	{
		MessageBoxW(nullptr, _T("Невалиден сокет!"), _T("Грешка!"), MB_ICONWARNING | MB_OK);
		WSACleanup();
		network_success = false;
		return 1;
	}

	c_result = connect(client_connect, reinterpret_cast<SOCKADDR*>(&c_address), sizeof(sockaddr_in));
	if (c_result)
	{
		MessageBoxW(nullptr, _T("Сървърът е недостъпен!"), _T("Грешка при свързване!"), MB_ICONERROR | MB_OK);
		closesocket(client_connect);
		WSACleanup();
		network_success = false;
		return 1;
	}

	socket_success = true;
	return 0;
}

/*
 * __stdcall създава отделна нишка, в която се върти постоянно while цикъл,
 * който проверява през 50 милисекунди за новопристигнали съобщения. Те се
 * подават на функцията Convert_Incoming_Message_To_WideChar за конвертиране
 * в wchar_t, след което се препредават на функция Append_New_Message_To_Chat,
 * която ги "залепва" под вече съществуващите записи в графичния интерфейс.
 */
unsigned int __stdcall Client_Thread(void*)
{
	char* buffer = new char[256]{ 0 };

	int size = 0;
	while (true)
	{
		SecureZeroMemory(buffer, 256);
		if ((size = recv(client_connect, buffer, 256, NULL) > 0))
		{
			wchar_t* msg = Convert_Incoming_Message_To_WideChar(buffer);
			Append_New_Message_To_Chat(msg);
		}
		Sleep(50);
	}
}

/*
 * Функцията Set_Username_To_Window_Title "залепва" името на потребителя
 * към името на приложението в заглавния ред на прозореца му. Освен това
 * новополучения текстов низ се запазва в глобална променлива, която се
 * използва за откриване на нужния прозорец от функцията, която пише по
 * текстовия контрол /Append_New_Message_To_Chat/
 */
void Set_Username_To_Window_Title(wchar_t* new_title)
{
	const HWND& hwnd = FindWindowExW(nullptr, nullptr, _T("MainWindow"), nullptr);

	const int len = GetWindowTextLengthW(hwnd) + 1;
	wchar_t* text = new wchar_t[len];

	GetWindowTextW(hwnd, text, len);

	const wchar_t* delimiter = L" | Потребител: ";
	text = wcscat(text, delimiter);
	text = wcscat(text, new_title);
	main_window_title = text;
	SetWindowTextW(hwnd, text);
}

/*
 * Функцията Convert_Outgoing_Message_To_MultiByte преобразува
 * изходящите съобщения от wchar_t в char, за да може да преминат
 * през winsock.
 */
char* Convert_Outgoing_Message_To_MultiByte(wchar_t* message)
{
	char* multi_byte = new char[256];

	const int wide_chars_number = WideCharToMultiByte(CP_UTF8, 0, message, -1, nullptr, 0, nullptr, nullptr);

	WideCharToMultiByte(CP_UTF8, 0, message, -1, multi_byte, wide_chars_number, nullptr, nullptr);

	return multi_byte;
}

/*
 * Функцията Convert_Incoming_Message_To_WideChar преобразува
 * входящите съобщения от char в wchar_t, за да може да бъдат
 * изведени коректно в текстовия контрол.
 */
wchar_t* Convert_Incoming_Message_To_WideChar(char* message)
{
	strcat(message, "\r\n");

	const int chars_number = MultiByteToWideChar(CP_UTF8, UNICODE, message, -1, nullptr, 0);

	wchar_t* wide_chars = new wchar_t[chars_number];

	MultiByteToWideChar(CP_UTF8, UNICODE, message, -1, wide_chars, chars_number);

	return wide_chars;
}

/*
 * Функцията Append_New_Message_To_Chat "залепва" новоприетите съобщения
 * под вече пристигналите в текстовия контрол с ID_EDITCHILD1 101.
 * Открива се нужния прозорец по името на неговия прозоречен клас и
 * и неговия WindowTite. Изчита се съществуващото съдържание на текстовия
 * контрол, изичислява се неговата дължина плюс тази на новото съобщение,
 * създава се буфер със съответния размер, в него се записва съдържанието
 * на прозореца в момента, към буфера се долепва новото съобщение и всичко
 * се подава към текстовия контрол като WindowText чрез SetWindowTextW.
 */
void Append_New_Message_To_Chat(wchar_t* new_text)
{
	const HWND& hwnd = FindWindowExW(nullptr, nullptr, _T("MainWindow"), main_window_title);

	const HWND& hwnd_output = GetDlgItem(hwnd, ID_EDITCHILD1);

	const int out_length = GetWindowTextLengthW(hwnd_output) + lstrlenW(new_text) + 1;

	std::vector<wchar_t> buf(out_length);
	wchar_t* pbuf = &buf[0];

	GetWindowTextW(hwnd_output, pbuf, out_length);

	wcscat_s(pbuf, out_length, new_text);

	SetWindowTextW(hwnd_output, pbuf);
}
/*
 * Курсова работа по "Системно програмиране" от Красимир Банчев, Фак. №46058зкн
 * Winsock 2.0 клиент/сървър приложение за мрежов чат
 * C++ Win32 конзолен сървър v. 0.9
 */

#ifndef UNICODE
#define UNICODE
#endif

#include <WS2tcpip.h>
#include <process.h>
#include <comdef.h>
#include <cstdlib>
#include <tchar.h>
#include <cwchar>
#include <string>
#include <ios>
#include <fstream>

using namespace std;

#pragma comment(lib, "ws2_32.lib" )
#pragma comment(lib,"User32.lib")
#pragma warning(disable : 4996)
#pragma execution_character_set("utf-8")

SOCKET server_connect, server_socket, * server_connections;
WSADATA wsa_data;
int s_result;
int counter = 0;
sockaddr_in s_address;
char* s_name[256];
wchar_t* ini_path = nullptr;
wchar_t* log_path = nullptr;
const char* ip = nullptr;
int port = 0;
unsigned int __stdcall  Server_Thread(void* p);
static void Write_Log(wchar_t* log_filename, char* line);

int _tmain(int argc, _TCHAR* argv[])
{
    SetConsoleOutputCP(65001);

    SetConsoleTitleW(_T("Курсова работа по \"Системно програмиране\" от Красимир Банчев, Фак. №46058зкн"));

    const wchar_t* ini_filename = _T("\\Server.ini");
    const wchar_t* log_filename = _T("\\Server.log");

    wchar_t i_path[MAX_PATH];
    GetCurrentDirectoryW(MAX_PATH, i_path);
    ini_path = wcscat(i_path, ini_filename);

    wchar_t l_path[MAX_PATH];
    GetCurrentDirectoryW(MAX_PATH, l_path);
    log_path = wcscat(l_path, log_filename);

    wchar_t addr[32];
    GetPrivateProfileStringW(_T("host"), _T("ip"), _T("127.0.0.1"), addr, sizeof(addr), ini_path);
    const _bstr_t _addr(addr);
    ip = _addr;

    port = GetPrivateProfileIntW(_T("host"), _T("port"), 22220, ini_path);

    s_address.sin_family = AF_INET;
    s_address.sin_port = htons(port);
    s_address.sin_addr.S_un.S_addr = inet_addr(ip);

    s_result = WSAStartup(MAKEWORD(2, 2), &wsa_data);

    if (s_result)
    {
        printf("Winsock липсва или е стара версия!");
        Write_Log(log_path, "Winsock липсва или е стара версия!");
        return 1;
    }

    server_connect = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (server_connect == INVALID_SOCKET)
    {
        printf("Невалиден сокет!");
        Write_Log(log_path, "Невалиден сокет!");
        WSACleanup();
        return 1;
    }

    s_result = bind(server_connect, reinterpret_cast<sockaddr*>(&s_address), sizeof(sockaddr_in));

    if (s_result)
    {
        printf("Грешка при създаване на сокет: %lu", GetLastError());
        Write_Log(log_path, "Грешка при създаване на сокет!");
        closesocket(server_connect);
        WSACleanup();
        return 1;
    }

    s_result = listen(server_connect, SOMAXCONN);

    if (s_result)
    {
        printf("Грешка при свързване: %lu", GetLastError());
        Write_Log(log_path, "Грешка при свързване!");
        closesocket(server_connect);
        WSACleanup();
        return 1;
    }

    printf("Winsock чат сървър. Адрес: %s:%d\n", ip, port);
    printf("Конфигурационен файл: %ls\n", ini_path);
    printf("Лог файл: %ls\n\n", log_path);

    server_connections = static_cast<SOCKET*>(calloc(64, sizeof(SOCKET)));

    while ((server_socket = accept(server_connect, nullptr, nullptr)))
    {
        if (server_socket == INVALID_SOCKET)
        {
            printf("Грешка при свързване с клиент: %lu", GetLastError());
            Write_Log(log_path, "Грешка при свързване с клиент!");
            continue;
        }

        server_connections[counter] = server_socket;
        s_name[counter] = new char[256];

        const char* empty = "";
        SecureZeroMemory(s_name[counter], 256);

        recv(server_connections[counter], s_name[counter], 256, NULL);

        if (strcmp(s_name[counter], empty) != 0)
        {
            printf("%s влезе в системата\n", s_name[counter]);
            char* enter = " влезе в системата";
            char who_enter[256];
            strcpy_s(who_enter, s_name[counter]);
            strcat_s(who_enter, enter);
            Write_Log(log_path, who_enter);

            char* salutation = "Влязохте като ";
            char greeting[256];
            strcpy_s(greeting, salutation);
            strcat_s(greeting, s_name[counter]);

            send(server_connections[counter], greeting, 256, NULL);

            CreateThread(nullptr, 0, LPTHREAD_START_ROUTINE(Server_Thread), reinterpret_cast<void*>(counter), 0, nullptr);
            counter++;
        }
    }

    for (int i = 0; i < counter; i++)
    {
        s_result = shutdown(server_connections[i], SD_SEND);
        if (s_result)
        {
            printf("Грешка при затваряне на сесия: %d\n", WSAGetLastError());
            Write_Log(log_path, "Грешка при затваряне на сесия!");
            closesocket(server_connections[i]);
            WSACleanup();
            return 1;
        }
    }

    return 0;
}

unsigned int __stdcall Server_Thread(void* p)
{
    int id = (int)p;
    char buffer[256];
    char* message = new char[256];
    int size = 0;

    while (true)
    {
        SecureZeroMemory(message, 256);
        if ((size = recv(server_connections[id], message, 256, NULL) > 0))
        {
            for (int i = 0; i < counter; i++)
            {
                if (i == id)
                {
                    SecureZeroMemory(buffer, 256);
                    sprintf_s(buffer, "%s: %s", s_name[i], message);
                    Write_Log(log_path, buffer);
                    puts(buffer);
                    send(server_connections[i], buffer, 256, NULL);
                }
                else
                {
                    SecureZeroMemory(buffer, 256);
                    sprintf_s(buffer, "%s: %s", s_name[id], message);
                    send(server_connections[i], buffer, 256, NULL);
                }
            }
        }
    }
}

static void Write_Log(wchar_t* log_filename, char* line)
{
    strcat(line, "\n");

    DWORD dw_bytes_to_write = strlen(line);
    DWORD dw_bytes_written;
    BOOL before_error_flag = FALSE;

    HANDLE log_file = CreateFileW(log_filename,
        FILE_APPEND_DATA,
        FILE_SHARE_READ,
        nullptr,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

    if (log_file == INVALID_HANDLE_VALUE)
    {
        printf("Файла \"%ls\" е недостъпен!\n", log_filename);
    }

    while (dw_bytes_to_write > 0)
    {
        before_error_flag = WriteFile(log_file, line, dw_bytes_to_write, &dw_bytes_written, nullptr);

        if (!before_error_flag)
        {
            printf("Възникна грешка при запис в лог файла!\n");
        }

        dw_bytes_to_write -= dw_bytes_written;
    }

    CloseHandle(log_file);
}
/*
 * Курсов проект по "Системно програмиране" от Красимир Банчев, Фак. №46058зкн
 * Winsock 2.0 клиент/сървър приложение за мрежов чат
 * C++ Win32 API конзолен сървър v. 1.0
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

#pragma optimize("a",on)
#pragma comment(linker, "/STACK:2000000")
#pragma comment(linker, "/HEAP:2000000")

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

/*
 * Функцията wmain е основната в това конзолно приложение.
 * Най-напред се определя текущия път от който е стартирано приложението, от
 * който се изчислява местоположението на конфигурационния файл, от който
 * сървъра разбира на кой ip адрес и порт трябва да "слуша" за клиентски конекции.
 * По същия начин се оперира с местоположението на текстовия лог файл.
 * Проверява се системата за наличие на съответната версия на winsock, при успех
 * се създава сокет и се осъществява връзка със сървъра. При неуспех на всяка
 * стъпка на конзолата се изписва съответното съобщение.
 */
int wmain(int argc, _TCHAR* argv[])
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

    /*
     * Win32 API функциите GetPrivateProfileStringW и GetPrivateProfileIntW
     * правят относително лесна работата с конфигурационни (.ini) файлове.
     * Дават възможност за включване на дефолтна стойност, която да се
     * използва при отсъствие на съответния файл.
     */
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

    /*
     * Проверките са приключили успешно и сървъра е готов за работа.
     * На конзолата се изписва името на програмата, на кой адрес и порт
     * очаква клиенти и пълните имена с пътя на конфигурационния и лог файловете.
     */
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

        /*
         * В първото съобщение потребителя изпраща никнейма си, което
         * в нашия случай се явява "регистрация" в чат системата.
         */
        recv(server_connections[counter], s_name[counter], 256, NULL);

        /*
         * Ако съобщението не е празно, се съставя съобщение до потребителя,
         * включващо и неговото име, което му се подава обратно.
         */
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

            /*
             * Тук се стартира сървърната нишка, която "слуша"
             * на указаните адрес и порт за клиентски съобщения
             */
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

/*
 * __stdcall създава нишката, която приема съобщенията на клиентите.
 * В нея работи while, цикъл, който през 50ms проверява за нови събития.
 * Той работи като ехо услуга. Клиентската програма визуализира на екран
 * своите собствени съобщения едва след като ги е получила обратно от сървъра.
 * Съобщенията се записват също така и във текстов лог файл и се изписват на
 * конзолата на сървъра.
 */
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

        Sleep(50);
    }
}

/*
 * Функцията "Write_Log" записва в текстов файл събития като "влизането"
 * на потребител в "чата" и новопостъпилите съобщения на всички потребители
 */
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
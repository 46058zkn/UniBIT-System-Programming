# C++ Win32 API клиент/сървър Winsock 2 приложение
Курсов проект по "Системно програмиране" от Красимир Банчев, Фак. №46058зкн
 
Приложението представлява клиент/сървър система за чат, като сървъра е конзолно приложение,
а клиента е проста графична програма, написана с използване на Microsoft Win32 API, без контроли.

Разработено е на Visual Studio 2019 с Platform Toolset v. 14.23 и Windows SDK 10.0.18362.0
За правилната им работа и двете трябва да се компилират за x86 Win32 платформа.
Във Vusual Studio е необходимо в Startup Project да се укаже "Current selection" и
двата компонента на проекта да се стартират поотделно, като сървърната програма
трябва да се стартира преди клиентската.

По подразбиране сървърът ще слуша на адрес 127.0.0.1:22220, там ще го търси и клиентът.
При нужда да се тества работата на програмата с други адреси и портове, следва
да се използват приложените конфигурационни .ini файлове.

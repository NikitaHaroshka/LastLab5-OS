#include <iostream>
#include <windows.h>

using namespace std;

struct Employee {
    int num;
    char name[10];
    double hours;
};

int main()
{
    char c[20];
    char command[20];
    int thisEmp;
    Employee emp;
    DWORD dw;
    bool b;
    
    HANDLE hSemaphore = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, L"WriteSemaphore");
    HANDLE hMutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, L"SyncMutex");
    HANDLE hServer = CreateFileA(
        "\\\\.\\pipe\\server_pipe",
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        (LPSECURITY_ATTRIBUTES)NULL,
        OPEN_EXISTING,
        0,
        (HANDLE)NULL
    );
    if (hServer == INVALID_HANDLE_VALUE) {
        cout << "Error";
        return 1;
    }

    while (true) {
        cout << "Enter a command (read / write / exit): ";
        cin >> command;
        if (strcmp(command, "exit") == 0) {
            break;
        }
        if (!WriteFile(hServer, &command, sizeof(command), &dw, NULL)) {
            cerr << "Error";
            return 1;
        }

        else if (strcmp(command, "read") == 0) {
            cout << "Enter number of employee: ";
            cin >> thisEmp;
            if (!WriteFile(hServer, &thisEmp, sizeof(thisEmp), &dw, NULL)) {
                cerr << "Error";
                return 1;
            }

            if (!ReadFile(hServer, &b, sizeof(bool), &dw, NULL)) {
                cerr << "Error";
                CloseHandle(hServer);
                return 1;
            }

            if (b == false) {
                cout << "Employee not found" << endl;
                continue;
            }
            else {
                if (!ReadFile(hServer, &emp, sizeof(emp), &dw, NULL)) {
                    cerr << "Error";
                    CloseHandle(hServer);
                    return 1;
                }
                cout << "Emloyee: " << emp.num << " " << emp.name
                    << " " << emp.hours << endl;
            }
            cout << "Press any key to finish reading: ";
            cin >> c;
        }
        else if (strcmp(command, "write") == 0) {
            cout << "Enter number of employee: ";
            cin >> thisEmp;
            //currEmployee = thisEmp;
            if (!WriteFile(hServer, &thisEmp, sizeof(thisEmp), &dw, NULL)) {
                cerr << "Error";
                return 1;
            }

            if (!ReadFile(hServer, &b, sizeof(bool), &dw, NULL)) {
                cerr << "Error";
                CloseHandle(hServer);
                return 1;
            }

            if (b == false) {
                cout << "Employee not found" << endl;
                continue;
            }
            else {
                if (!ReadFile(hServer, &emp, sizeof(emp), &dw, NULL)) {
                    cerr << "Error";
                    CloseHandle(hServer);
                    return 1;
                }
                cout << "Employee: " << emp.num << " " << emp.name
                    << " " << emp.hours << endl;
                cout << "Rewrite: ";
                cin >> emp.num >> emp.name >> emp.hours;

                cout << "Press any key to send data to the server: ";
                cin >> c;

                if (!WriteFile(hServer, &emp, sizeof(emp), &dw, NULL)) {
                    cerr << "Error";
                    return 1;
                }
            }

            cout << "Press any to finish writing: ";
            cin >> c;
            thisEmp = 0;
        }
    }

    CloseHandle(hServer);
    cout << "Press any key to exit: ";
    cin >> c;
    return 0;
}
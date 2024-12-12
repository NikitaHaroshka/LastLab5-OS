#include <iostream>
#include <stdio.h>
#include <windows.h>

using namespace std;

struct Employee {
    int num;
    char name[10];
    double hours;
};

Employee* Employees;
HANDLE hSemaphore, hMutex;
int empsNum;
int readerCount = 0;
char fileName[50];
int currEmployee = -1;
void WriteEmployeesToFile() {
    FILE* file;
    fopen_s(&file, fileName, "wb");
    if (file != NULL) {
        fwrite(Employees, sizeof(Employee), empsNum, file);
        fclose(file);
        printf("Employees data written to binary file '%s'.\n", fileName);
    }
    else {
        printf("Error opening file '%s' for writing.\n", fileName);
    }
}

void ReadEmployeesFromFile() {
    FILE* file;
    fopen_s(&file, fileName, "rb");
    if (file != NULL) {
        Employee tempEmp;
        printf("Employees in file '%s':\n", fileName);
        while (fread(&tempEmp, sizeof(Employee), 1, file)) {
            printf("Num: %d, Name: %s, Hours: %.2f\n", tempEmp.num, tempEmp.name, tempEmp.hours);
        }
        fclose(file);
    }
    else {
        printf("Error opening file '%s' for reading.\n", fileName);
    }
}

DWORD WINAPI Threads(LPVOID param) {
    HANDLE hPipe = (HANDLE)param;
    int empNum, fEmpNum;
    char command[20] = "";
    DWORD dwBytesRead, dwBytesWrote;
    BOOL fSuccess = FALSE;
    bool exists = false;
    while (true) {
        fSuccess = ReadFile(hPipe, &command, sizeof(command), &dwBytesRead, NULL);

        if (strcmp(command, "read") == 0) {
            WaitForSingleObject(hMutex, INFINITE);
            readerCount++;
            if (readerCount == 1)
                WaitForSingleObject(hSemaphore, INFINITE);
            ReleaseMutex(hMutex);

            fSuccess = ReadFile(hPipe, &empNum, sizeof(empNum), &dwBytesRead, NULL);
            

            exists = false;
            for (int i = 0; i < empsNum; i++) {
                if (Employees[i].num == empNum) {
                    exists = true;
                    fEmpNum = i;
                }
            }

            fSuccess = WriteFile(hPipe, &exists, sizeof(bool), &dwBytesWrote, NULL);
            

            if (exists) {
                fSuccess = WriteFile(hPipe, &Employees[fEmpNum], sizeof(Employee), &dwBytesWrote, NULL);
                
            }

            WaitForSingleObject(hMutex, INFINITE);
            readerCount--;
            if (readerCount == 0)
                ReleaseSemaphore(hSemaphore, 1, NULL);
            ReleaseMutex(hMutex);
        }
        else if (strcmp(command, "write") == 0) {
            WaitForSingleObject(hSemaphore, INFINITE);
            fSuccess = ReadFile(hPipe, &empNum, sizeof(empNum), &dwBytesRead, NULL);
            

            exists = false;
            for (int i = 0; i < empsNum; i++) {
                if (Employees[i].num == empNum) {
                    exists = true;
                    fEmpNum = i;
                }
            }

            fSuccess = WriteFile(hPipe, &exists, sizeof(bool), &dwBytesWrote, NULL);

            if (exists) {
                fSuccess = WriteFile(hPipe, &Employees[fEmpNum], sizeof(Employee), &dwBytesWrote, NULL);
                

                fSuccess = ReadFile(hPipe, &Employees[fEmpNum], sizeof(Employee), &dwBytesRead, NULL);
               
            }

            ReleaseSemaphore(hSemaphore, 1, NULL);
        }
    }

    DisconnectNamedPipe(hPipe);
    CloseHandle(hPipe);
    printf("Thread stopped.\n");
    return 1;
}

int main() {
    PROCESS_INFORMATION pi;
    STARTUPINFO cb;
    wchar_t commandLine[] = L"Client.exe";
    int clientsNum, readerCount = 0;
    int thisEmpInUse = -1;
    printf("Enter the name of the binary file: ");
    scanf_s("%s", fileName, 50);

    printf("Type number of employees: ");
    scanf_s("%d", &empsNum);
    Employees = new Employee[empsNum];

    printf("Type your employees (num, name, hours):\n");
    for (int i = 0; i < empsNum; i++) {
        scanf_s("%d %s %lf", &Employees[i].num, &Employees[i].name, 20, &Employees[i].hours);
    }

    WriteEmployeesToFile();

    ReadEmployeesFromFile();

    printf("Type number of clients: ");
    scanf_s("%d", &clientsNum);

    hSemaphore = CreateSemaphore(NULL, 1, clientsNum, L"WriteSemaphore");
    hMutex = CreateMutex(NULL, FALSE, L"SyncMutex");

    HANDLE hPipe;
    HANDLE* hThreads;
    hThreads = new HANDLE[clientsNum];
    DWORD dwThreadId;
    for (int i = 0; i < clientsNum; i++) {
        hPipe = CreateNamedPipe(
            L"\\\\.\\pipe\\server_pipe",
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            clientsNum, 0, 0,
            INFINITE,
            NULL);

        ZeroMemory(&cb, sizeof(STARTUPINFO));
        cb.cb = sizeof(STARTUPINFO);
        if (!CreateProcess(NULL, commandLine, NULL, NULL, FALSE, CREATE_NEW_CONSOLE,
            NULL, NULL, &cb, &pi)) {
            printf("Process %d creation failed.\n", i + 1);
            break;
        }
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);

        if (ConnectNamedPipe(hPipe, NULL)) {
            printf("Client %d ready.\n", i + 1);
            hThreads[i] = CreateThread(NULL, 0, Threads, (LPVOID)hPipe, 0, &dwThreadId);
        }
        else {
            CloseHandle(hPipe);
        }
    }

    WaitForMultipleObjects(clientsNum, hThreads, TRUE, INFINITE);
    for (int i = 0; i < clientsNum; i++) {
        CloseHandle(hThreads[i]);
    }
    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <psapi.h>

/* Message Colors */
#define eColor "\033[0;31m" /* Error Color:  Red*/
#define sColor "\033[0;32m" /* Successful Color:  Green*/
#define iColor "\033[0;36m" /* Information Color:  Cyan*/
#define rColor "\033[0;37m" /* Reset Color:  White*/

/* Messages */
char e[] = "[-]";
char s[] = "[+]";
char i[] = "[*]";

wchar_t* programPath;
wchar_t* GetProcessFullPath(DWORD pid) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (hProcess == NULL) {
        printf("Failed to open process.\n");
        return NULL;
    }

    wchar_t* fullPath = NULL;
    DWORD size = MAX_PATH;
    do {
        fullPath = (wchar_t*)malloc(size * sizeof(wchar_t));
        if (!fullPath) {
            printf("Failed to allocate memory.\n");
            CloseHandle(hProcess);
            return NULL;
        }
        if (GetModuleFileNameExW(hProcess, NULL, fullPath, size) == 0) {
            free(fullPath);
            fullPath = NULL;
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                size *= 2;
            }
            else {
                printf("Failed to retrieve process path.\n");
                CloseHandle(hProcess);
                return NULL;
            }
        }
        else {
            break;
        }
    } while (1);
    CloseHandle(hProcess);
    return fullPath;
}

void RemoveFileNameFromPath(wchar_t* path) {
    wchar_t* lastSlash = wcsrchr(path, L'\\');
    if (lastSlash != NULL) {
        *lastSlash = L'\0';
    }
}

int main(int argc, char* argv[]) {
    DWORD PID, bytesRead = 0;
    DWORD filterFlags = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_ATTRIBUTES |
        FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_SECURITY;
    FILE_NOTIFY_INFORMATION* notifyInfo;
    HANDLE hMalwareDirectory = NULL;
    char buf[1024];

    if (argc < 2) {
        printf("Usage: .\\programViewer.exe <PID>\n");
        return EXIT_FAILURE;
    }
    PID = atoi(argv[1]);
    wchar_t* fullPath = GetProcessFullPath(PID);
    if (fullPath == NULL) {
        printf("%s%s Failed to get process full path. Error Code: %lx%s\n", eColor, e, GetLastError(), rColor);
        return EXIT_FAILURE;
    }
    programPath = fullPath;
    RemoveFileNameFromPath(programPath);
    printf("%s%s Program Path: %ls%s\n\n", iColor, i, programPath, rColor);

    hMalwareDirectory = CreateFileW(programPath, FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (hMalwareDirectory == INVALID_HANDLE_VALUE) {
        printf("Failed to open directory: %ls\n", programPath);
        free(fullPath);
        return EXIT_FAILURE;
    }
    while (ReadDirectoryChangesW(hMalwareDirectory, buf, sizeof(buf), TRUE, filterFlags, &bytesRead, NULL, NULL)) {
        notifyInfo = (FILE_NOTIFY_INFORMATION*)buf;
        while (notifyInfo) {
            if (notifyInfo->Action == FILE_ACTION_ADDED) {
                printf("%s%s New File Added. File Name: %.*ls%s\n", iColor, i, notifyInfo->FileNameLength / sizeof(wchar_t), notifyInfo->FileName, iColor);
            }
            else if (notifyInfo->Action == FILE_ACTION_REMOVED) {
                printf("%s%s File Removed:%.*ls%s\n", eColor, e, notifyInfo->FileNameLength / 2, notifyInfo->FileName, rColor);
            }
            else if (notifyInfo->Action == FILE_ACTION_RENAMED_NEW_NAME) {
                printf("%s%s File Changed Name: %.*ls%s\n", sColor, i, notifyInfo->FileNameLength / 2, notifyInfo->FileName, rColor);
            }
            else if (notifyInfo->Action == FILE_ACTION_RENAMED_OLD_NAME) {
                printf("%s%s File Old Name: %.*ls%s\n", sColor, i, notifyInfo->FileNameLength / 2, notifyInfo->FileName, rColor);
            }
            else if (notifyInfo->Action == FILE_ACTION_MODIFIED) {
                printf("%s%s File Modified:%.*ls%s\n", iColor, i, notifyInfo->FileNameLength / 2, notifyInfo->FileName, rColor);
            }
            notifyInfo = notifyInfo->NextEntryOffset > 0 ? (FILE_NOTIFY_INFORMATION*)((LPBYTE)notifyInfo + notifyInfo->NextEntryOffset) : NULL;
        }
    }
    free(fullPath);
    CloseHandle(hMalwareDirectory);

    return 0;
}

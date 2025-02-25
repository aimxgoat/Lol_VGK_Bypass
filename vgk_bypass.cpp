#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <map>

// Open a thread with required permissions
HANDLE OpenThread(DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwThreadId) {
    return ::OpenThread(dwDesiredAccess, bInheritHandle, dwThreadId);
}

// Get thread cycle time
ULONGLONG GetThreadCycleTime(DWORD threadID) {
    HANDLE hThread = OpenThread(THREAD_QUERY_INFORMATION, FALSE, threadID);
    if (!hThread) return 0;

    ULONGLONG cycleTime = 0;
    if (!::QueryThreadCycleTime(hThread, &cycleTime)) {
        CloseHandle(hThread);
        return 0;
    }

    CloseHandle(hThread);
    return cycleTime;
}

// Suspend a thread
bool SuspendThreadByID(DWORD threadID) {
    HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, threadID);
    if (!hThread) return false;

    DWORD result = SuspendThread(hThread);
    CloseHandle(hThread);
    return (result != -1);
}

// Find active threads for a process
std::map<DWORD, ULONGLONG> FindActiveThreads(const std::string& processName) {
    std::map<DWORD, ULONGLONG> activeThreads;
    
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return activeThreads;

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnapshot, &pe)) {
        do {
            if (processName == pe.szExeFile) {
                DWORD pid = pe.th32ProcessID;
                HANDLE hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
                if (hThreadSnap != INVALID_HANDLE_VALUE) {
                    THREADENTRY32 te;
                    te.dwSize = sizeof(THREADENTRY32);
                    if (Thread32First(hThreadSnap, &te)) {
                        do {
                            if (te.th32OwnerProcessID == pid) {
                                ULONGLONG cycles = GetThreadCycleTime(te.th32ThreadID);
                                if (cycles > 10000) {
                                    activeThreads[te.th32ThreadID] = cycles;
                                }
                            }
                        } while (Thread32Next(hThreadSnap, &te));
                    }
                    CloseHandle(hThreadSnap);
                }
            }
        } while (Process32Next(hSnapshot, &pe));
    }

    CloseHandle(hSnapshot);
    return activeThreads;
}

// Ensure `vgc.exe` is running
bool EnsureVGCRunning() {
    system("tasklist > tasks.txt");
    std::ifstream taskFile("tasks.txt");
    std::string line;

    while (std::getline(taskFile, line)) {
        if (line.find("vgc.exe") != std::string::npos) {
            return true;
        }
    }

    std::cout << "[INFO] vgc.exe not running. Starting...\n";
    system("sc start vgc");
    std::this_thread::sleep_for(std::chrono::seconds(3));

    return true;
}

// Display menu
void DisplayMenu() {
    system("cls");
    std::cout << "\n1. Bypass\n2. Exit\nSelect an option: ";
}

// Main function
int main() {
    while (true) {
        DisplayMenu();
        int option;
        std::cin >> option;

        if (option == 1) {
            if (!EnsureVGCRunning()) {
                std::cout << "[ERROR] Could not start vgc.exe.\n";
                continue;
            }

            std::cout << "[INFO] Processing...\n";
            std::this_thread::sleep_for(std::chrono::seconds(3));

            auto threads = FindActiveThreads("vgc.exe");
            if (threads.size() < 4) {
                std::cout << "[ERROR] Could not find at least four active threads.\n";
                continue;
            }

            std::cout << "[INFO] Open League or Valorant: 01:00\n";
            for (int i = 60; i > 0; --i) {
                std::cout << "\r[INFO] Open League or Valorant: 00:" << (i < 10 ? "0" : "") << i << std::flush;
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }

            for (const auto& thread : threads) {
                SuspendThreadByID(thread.first);
            }

            std::cout << "[INFO] Wait 3 min then inject...\n";
            std::this_thread::sleep_for(std::chrono::minutes(3));

            std::cout << "[INFO] You can use the cheat now!\n";
        } else if (option == 2) {
            std::cout << "Exiting...\n";
            break;
        } else {
            std::cout << "[ERROR] Invalid option.\n";
        }
    }

    return 0;
}


#include <iostream>
#include <algorithm>
#include <conio.h>

#include "RingBuffer.h"

#include <Windows.h>
#include <process.h>
#include <string>
#include <list>

#include <fstream>

#include <unordered_map>
#include <array>
#include <vector>

std::unordered_map<int, std::vector<int>> g_JobQueueSize;


// std::wstring�� char*�� ��ȯ
std::string wstringToCharArray(const std::wstring& wStr) {
	// ��ȯ�� �ʿ��� ���� ũ�� ���
	int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wStr.c_str(), static_cast<int>(wStr.length()), nullptr, 0, nullptr, nullptr);
	std::string charArray(sizeNeeded, 0);

	// ��ȯ
	WideCharToMultiByte(CP_UTF8, 0, wStr.c_str(), static_cast<int>(wStr.length()), &charArray[0], sizeNeeded, nullptr, nullptr);

	return charArray;
}

// char*�� std::wstring���� ��ȯ
std::wstring charArrayToWstring(const char* charArray) {
	// ��ȯ�� �ʿ��� ���� ũ�� ���
	int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, charArray, -1, nullptr, 0);
	std::wstring wStr(sizeNeeded, 0);

	// ��ȯ
	MultiByteToWideChar(CP_UTF8, 0, charArray, -1, &wStr[0], sizeNeeded);

	return wStr;
}





using namespace std;

typedef struct st_MSG_HEAD
{
	short shType;
	short shPayloadLen;

	st_MSG_HEAD(short _shType = 0, short _shPayloadLen = 0)
	{
		shType = _shType;
		shPayloadLen = _shPayloadLen;
	}

}MSG_HEAD;

#define dfJOB_ADD	0
#define dfJOB_DEL	1
#define dfJOB_SORT	2
#define dfJOB_FIND	3
#define dfJOB_PRINT	4	// << ��� or ���� / �б⸸ �ϴ� ���� �ൿ
#define dfJOB_QUIT	5

//-----------------------------------------------
// ������ ��, ���ڿ� ����Ʈ
//-----------------------------------------------
list<wstring>		g_List;

//-----------------------------------------------
// ������ �޽��� ť (������ �˳��ϰ� ũ�� 4~5�� ����Ʈ)
//-----------------------------------------------
CRingBuffer 		g_msgQ{ 50000 };



// critical section
CRITICAL_SECTION cs;
CRITICAL_SECTION cs_gList;


// �̺�Ʈ ��ü
HANDLE ghEventWakeUp;

bool bContinue = true;


unsigned int _stdcall FuncObserve(void* pArg)
{
	unsigned int threadID[3];

	threadID[0] = ((unsigned int*)pArg)[0];
	threadID[1] = ((unsigned int*)pArg)[1];
	threadID[2] = ((unsigned int*)pArg)[2];
	
	/*
		@ Ư�� ������� 1�ʸ��� �� ���� ����͸� 

		* �޽��� ��ť �� ������ (��뷮)  ���
		* �ʴ� �޽��� �� ��ü ó�� �� (TPS) + ������ �� ó����
		* �ʴ� Ÿ�Ժ� �� ó�� �� (�޽��� �� TPS)
		
		+ ���
		�޽��� ť�� ��뷮�� ������ �̻� ���޽� �޽����� �����θ� �ǵ������� ������ �����ϴ� ���
	*/
	
	int accmlThread0[dfJOB_QUIT]{};
	int accmlThread1[dfJOB_QUIT]{};
	int accmlThread2[dfJOB_QUIT]{};

	int tempAccmlThread0[dfJOB_QUIT]{};
	int tempAccmlThread1[dfJOB_QUIT]{};
	int tempAccmlThread2[dfJOB_QUIT]{};

	// �������̱⿡ �����ϰ� �ִ� �����忡 ������ ��ġ�� �ȵ�.
	while (bContinue)
	{
		system("cls");

		tempAccmlThread0[dfJOB_ADD] = g_JobQueueSize[(int)threadID[0]][dfJOB_ADD] - accmlThread0[dfJOB_ADD];
		tempAccmlThread0[dfJOB_DEL] = g_JobQueueSize[(int)threadID[0]][dfJOB_DEL] - accmlThread0[dfJOB_DEL];
		tempAccmlThread0[dfJOB_SORT] = g_JobQueueSize[(int)threadID[0]][dfJOB_SORT] - accmlThread0[dfJOB_SORT];
		tempAccmlThread0[dfJOB_FIND] = g_JobQueueSize[(int)threadID[0]][dfJOB_FIND] - accmlThread0[dfJOB_FIND];
		tempAccmlThread0[dfJOB_PRINT] = g_JobQueueSize[(int)threadID[0]][dfJOB_PRINT] - accmlThread0[dfJOB_PRINT];
		
		tempAccmlThread1[dfJOB_ADD] = g_JobQueueSize[(int)threadID[1]][dfJOB_ADD] - accmlThread1[dfJOB_ADD];
		tempAccmlThread1[dfJOB_DEL] = g_JobQueueSize[(int)threadID[1]][dfJOB_DEL] - accmlThread1[dfJOB_DEL];
		tempAccmlThread1[dfJOB_SORT] = g_JobQueueSize[(int)threadID[1]][dfJOB_SORT] - accmlThread1[dfJOB_SORT];
		tempAccmlThread1[dfJOB_FIND] = g_JobQueueSize[(int)threadID[1]][dfJOB_FIND] - accmlThread1[dfJOB_FIND];
		tempAccmlThread1[dfJOB_PRINT] = g_JobQueueSize[(int)threadID[1]][dfJOB_PRINT] - accmlThread1[dfJOB_PRINT];
		
		tempAccmlThread2[dfJOB_ADD] = g_JobQueueSize[(int)threadID[2]][dfJOB_ADD] - accmlThread2[dfJOB_ADD];
		tempAccmlThread2[dfJOB_DEL] = g_JobQueueSize[(int)threadID[2]][dfJOB_DEL] - accmlThread2[dfJOB_DEL];
		tempAccmlThread2[dfJOB_SORT] = g_JobQueueSize[(int)threadID[2]][dfJOB_SORT] - accmlThread2[dfJOB_SORT];
		tempAccmlThread2[dfJOB_FIND] = g_JobQueueSize[(int)threadID[2]][dfJOB_FIND] - accmlThread2[dfJOB_FIND];
		tempAccmlThread2[dfJOB_PRINT] = g_JobQueueSize[(int)threadID[2]][dfJOB_PRINT] - accmlThread2[dfJOB_PRINT];

		// �޽��� ť�� ������ ���
		std::cout << "ť ������ : " << g_msgQ.GetUseSize() << "\n";
		std::cout << "������ 0 TPS : " << tempAccmlThread0[dfJOB_ADD] + tempAccmlThread0[dfJOB_DEL] + tempAccmlThread0[dfJOB_SORT] + tempAccmlThread0[dfJOB_FIND] + tempAccmlThread0[dfJOB_PRINT] << "\n";
		std::cout << "������ 1 TPS : " << tempAccmlThread1[dfJOB_ADD] + tempAccmlThread1[dfJOB_DEL] + tempAccmlThread1[dfJOB_SORT] + tempAccmlThread1[dfJOB_FIND] + tempAccmlThread1[dfJOB_PRINT] << "\n";
		std::cout << "������ 2 TPS : " << tempAccmlThread2[dfJOB_ADD] + tempAccmlThread2[dfJOB_DEL] + tempAccmlThread2[dfJOB_SORT] + tempAccmlThread2[dfJOB_FIND] + tempAccmlThread2[dfJOB_PRINT] << "\n";
		std::cout << "��ü ADD : " << tempAccmlThread0[dfJOB_ADD] + tempAccmlThread1[dfJOB_ADD] + tempAccmlThread2[dfJOB_ADD] << "\n";
		std::cout << "��ü DEL : " << tempAccmlThread0[dfJOB_DEL] + tempAccmlThread1[dfJOB_DEL] + tempAccmlThread2[dfJOB_DEL] << "\n";
		std::cout << "��ü SORT : " << tempAccmlThread0[dfJOB_SORT] + tempAccmlThread1[dfJOB_SORT] + tempAccmlThread2[dfJOB_SORT] << "\n";
		std::cout << "��ü FIND : " << tempAccmlThread0[dfJOB_FIND] + tempAccmlThread1[dfJOB_FIND] + tempAccmlThread2[dfJOB_FIND] << "\n";
		std::cout << "��ü PRINT : " << tempAccmlThread0[dfJOB_PRINT] + tempAccmlThread1[dfJOB_PRINT] + tempAccmlThread2[dfJOB_PRINT] << "\n\n";


		accmlThread0[dfJOB_ADD] += tempAccmlThread0[dfJOB_ADD];
		accmlThread0[dfJOB_DEL] += tempAccmlThread0[dfJOB_DEL];
		accmlThread0[dfJOB_SORT] += tempAccmlThread0[dfJOB_SORT];
		accmlThread0[dfJOB_FIND] += tempAccmlThread0[dfJOB_FIND];
		accmlThread0[dfJOB_PRINT] += tempAccmlThread0[dfJOB_PRINT];

		accmlThread1[dfJOB_ADD] += tempAccmlThread1[dfJOB_ADD];
		accmlThread1[dfJOB_DEL] += tempAccmlThread1[dfJOB_DEL];
		accmlThread1[dfJOB_SORT] += tempAccmlThread1[dfJOB_SORT];
		accmlThread1[dfJOB_FIND] += tempAccmlThread1[dfJOB_FIND];
		accmlThread1[dfJOB_PRINT] += tempAccmlThread1[dfJOB_PRINT];

		accmlThread2[dfJOB_ADD] += tempAccmlThread2[dfJOB_ADD];
		accmlThread2[dfJOB_DEL] += tempAccmlThread2[dfJOB_DEL];
		accmlThread2[dfJOB_SORT] += tempAccmlThread2[dfJOB_SORT];
		accmlThread2[dfJOB_FIND] += tempAccmlThread2[dfJOB_FIND];
		accmlThread2[dfJOB_PRINT] += tempAccmlThread2[dfJOB_PRINT];


		// �޽��� �� TPS
		Sleep(1000);
	}

	std::cout << "Observe ������ ����\n";

	return 0;
}


unsigned int _stdcall FuncWorker(void* pArg)
{
	DWORD threadID = GetCurrentThreadId();

	std::cout << "������ " << threadID << " �۵�\n";

	MSG_HEAD header;
	char tempStr[1000];
	std::wstring strMsg;

	while (true)
	{
		WaitForSingleObject(ghEventWakeUp, INFINITE);

		memset(tempStr, 0, 1000);

		//EnterCriticalSection(&cs);

		while (g_msgQ.GetUseSize() != 0)
		{
			EnterCriticalSection(&cs);

			if (g_msgQ.GetUseSize() >= sizeof(MSG_HEAD))
			{
				// �޽��� ť���� �޽��� ��� �̱�
				g_msgQ.Dequeue((char*)&header, sizeof(MSG_HEAD));

				g_JobQueueSize[(int)threadID][(int)header.shType]++;

				switch (header.shType)
				{
				case 0:
				{
					// ADD

					// �޽����� �ִ� ���ڿ� ����
					int payloadLen = g_msgQ.Dequeue(tempStr, header.shPayloadLen);
					LeaveCriticalSection(&cs);

					tempStr[payloadLen] = '\0';

					// ���ڿ��� �����ؼ� wstring���� ����
					strMsg = charArrayToWstring(tempStr);

					// ����� ���ڿ��� g_List�� �߰�
					EnterCriticalSection(&cs_gList);
					g_List.push_back(strMsg);
					LeaveCriticalSection(&cs_gList);
				}
				break;

				case 1:
				{
					// DEL

					// �޽����� �ִ� ���ڿ� ����
					int payloadLen = g_msgQ.Dequeue(tempStr, header.shPayloadLen);
					LeaveCriticalSection(&cs);

					tempStr[payloadLen] = '\0';

					// ���ڿ��� �����ؼ� wstring���� ����
					strMsg = charArrayToWstring(tempStr);

					// ����� ���ڿ��� g_List���� �˻�
					EnterCriticalSection(&cs_gList);
					auto iter = std::find_if(g_List.begin(), g_List.end(), [&strMsg](const std::wstring& str) {
						if (strMsg == str)
							return true;
						else
							return false;
						});

					if (iter != g_List.end())
					{
						//std::wcout << *iter << "\n";

						g_List.erase(iter);
					}
					LeaveCriticalSection(&cs_gList);
				}
				break;

				case 2:
					// SORT
				{
					LeaveCriticalSection(&cs);

					EnterCriticalSection(&cs_gList);
					g_List.sort();
					LeaveCriticalSection(&cs_gList);
				}
				break;
				case 3:
					// FIND
				{
					// �޽����� �ִ� ���ڿ� ����
					int payloadLen = g_msgQ.Dequeue(tempStr, header.shPayloadLen);
					LeaveCriticalSection(&cs);

					tempStr[payloadLen] = '\0';

					// ���ڿ��� �����ؼ� wstring���� ����
					strMsg = charArrayToWstring(tempStr);

					// ����� ���ڿ��� g_List���� �˻�
					EnterCriticalSection(&cs_gList);
					auto iter = std::find_if(g_List.begin(), g_List.end(), [&strMsg](const std::wstring& str) {
						if (strMsg == str)
							return true;
						else
							return false;
						});

					if (iter != g_List.end())
						// ã�Ҵٸ� ã�Ҵٰ� ���
						;// std::wcout << *iter << "ã��\n";
					LeaveCriticalSection(&cs_gList);
				}
				break;

				case 4:
					// PRINT
				{
					LeaveCriticalSection(&cs);

					EnterCriticalSection(&cs_gList);
					//std::cout << "print : ";

					unsigned int i = 0;
					for (unsigned int i = 0; i < 1; ++i)
						i++;
					/*for (const auto& iter : g_List)
						std::wcout << iter << " ";*/
						//std::cout << "\n";
					LeaveCriticalSection(&cs_gList);

				}
				break;

				case 5:
					// QUIT
				{
					// ������ ����
					std::cout << "thread : " << threadID << " ����\n";

					LeaveCriticalSection(&cs);
					SetEvent(ghEventWakeUp);
					return 0;
				}
				break;
				default:
					break;
				}
			}
			else
				LeaveCriticalSection(&cs);
		}

		//LeaveCriticalSection(&cs);
	}
}



int main(void)
{
	ghEventWakeUp = CreateEvent(nullptr, false, false, nullptr);

	InitializeCriticalSection(&cs);
	InitializeCriticalSection(&cs_gList);

	// ��Ŀ ������ 3��
	HANDLE threadHandle[3];
	unsigned int threadID[3];
	threadHandle[0] = (HANDLE)_beginthreadex(nullptr, 0, FuncWorker, nullptr, 0, nullptr);
	threadID[0] = GetThreadId(threadHandle[0]);

	threadHandle[1] = (HANDLE)_beginthreadex(nullptr, 0, FuncWorker, nullptr, 0, nullptr);
	threadID[1] = GetThreadId(threadHandle[1]);

	threadHandle[2] = (HANDLE)_beginthreadex(nullptr, 0, FuncWorker, nullptr, 0, nullptr);
	threadID[2] = GetThreadId(threadHandle[2]);

	g_JobQueueSize.emplace(threadID[0], 0);
	g_JobQueueSize.emplace(threadID[1], 0);
	g_JobQueueSize.emplace(threadID[2], 0);

	g_JobQueueSize[threadID[0]].resize(6, 0);
	g_JobQueueSize[threadID[1]].resize(6, 0);
	g_JobQueueSize[threadID[2]].resize(6, 0);

	// ���� ������ 1��
	HANDLE ObserveThreadHandle;
	ObserveThreadHandle = (HANDLE)_beginthreadex(nullptr, 0, FuncObserve, threadID, 0, nullptr);

	srand((unsigned int)time(nullptr));

	std::wstring tempWStr1{ L"Hello World!1" };
	std::wstring tempWStr2{ L"Hello World!2" };
	std::wstring tempWStr3{ L"Hello World!3" };
	std::wstring tempWStr4{ L"Hello World!4" };
	std::wstring tempWStr5{ L"Hello World!5" };

	


	bool bCreateJob = true;
	bool bSlow = false;
	while (bContinue)
	{
		if (bCreateJob)
		{
			//int randStr = rand() % 5 + 1; // 1 ~ 5
			int randStr = rand() % 3 + 1; // 1 ~ 3
			std::string tempStr;

			switch (randStr)
			{
			case 1:
				tempStr = wstringToCharArray(tempWStr1);
				break;
			case 2:
				tempStr = wstringToCharArray(tempWStr2);
				break;
			case 3:
				tempStr = wstringToCharArray(tempWStr3);
				break;
			case 4:
				tempStr = wstringToCharArray(tempWStr4);
				break;
			case 5:
				tempStr = wstringToCharArray(tempWStr5);
				break;
			default:
				break;
			}

			// ������ �� ���� ( 0 ~ 4 )
			short randInt = rand() % 5;

			// �޽��� ����
			MSG_HEAD header{ randInt, 0 };

			switch (randInt)
			{
			case 0:	// add
			case 1:	// delete
			case 3:	// find
			{
				header.shPayloadLen = tempStr.size();
			}
			break;

			default:
				break;
			}

			EnterCriticalSection(&cs);

			g_msgQ.Enqueue((char*)&header, sizeof(MSG_HEAD));

			if (randInt == 0 || randInt == 1 || randInt == 3)
				g_msgQ.Enqueue((char*)tempStr.c_str(), header.shPayloadLen);

			if (1.f * g_msgQ.GetUseSize() / g_msgQ.GetBufferSize() >= 0.7f)
				bSlow = true;

			if (1.f * g_msgQ.GetUseSize() / g_msgQ.GetBufferSize() <= 0.3f)
				bSlow = false;

			LeaveCriticalSection(&cs);

			// �̺�Ʈ ��ü�� ����� ������ �����

			SetEvent(ghEventWakeUp);
		}

		if (_kbhit())
		{
			char ch = _getch();
			switch (ch)
			{
			case 's':
			case 'S':
				// job ���� ���
			{
				bCreateJob = !bCreateJob;
			}
			break;

			case 'q':
			case 'Q':
				// ���� �޽��� enq
			{
				MSG_HEAD header{ dfJOB_QUIT, 0 };
				g_msgQ.Enqueue((char*)&header, sizeof(MSG_HEAD));
				g_msgQ.Enqueue((char*)&header, sizeof(MSG_HEAD));
				g_msgQ.Enqueue((char*)&header, sizeof(MSG_HEAD));

				SetEvent(ghEventWakeUp);

				bContinue = false;
			}
			break;

			case 'w':
			case 'W':
				// ��Ŀ ������ �ƿ��
				// ���� ���� �̺�Ʈ Ȱ��ȭ
				SetEvent(ghEventWakeUp);
				break;

			default:
				break;
			}
		}

		if (bSlow)
			Sleep(1);
	}
	
	WaitForMultipleObjects(3, threadHandle, true, INFINITE);

	for (int i = 0; i < 3; ++i)
		CloseHandle(threadHandle[i]);

	DeleteCriticalSection(&cs);
	DeleteCriticalSection(&cs_gList);

	WaitForSingleObject(ObserveThreadHandle, INFINITE);
	CloseHandle(ObserveThreadHandle);

	// �޽��� ť�� ������ ���
	std::cout << "\nmain ���� ���� ���� ť ������ : " << g_msgQ.GetUseSize() << "\n";
}




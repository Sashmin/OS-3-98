#include <stdio.h>
#include <Windows.h>

int const sleepTime = 5;

CRITICAL_SECTION critSection;

int* numArray;
int numArrayLength;
int numberOfMarkers;

HANDLE* hUnableToProceed;
HANDLE* hExitThread;
HANDLE hResumeThread;
HANDLE hStartEvent;



DWORD WINAPI marker(LPVOID lpParam)
{
	int id;
	int markedCounter = 0;

	EnterCriticalSection(&critSection);		//ensuring that the curID variable is not altered by other threads
	int* pCurID = (int*)lpParam;
	id = *pCurID + 1;
	(*pCurID)++;
	LeaveCriticalSection(&critSection);

	WaitForSingleObject(hStartEvent, INFINITE);

	bool* isMarked = new bool[numArrayLength];
	for (int i = 0; i < numArrayLength; i++)
	{
		isMarked[i] = false;
	}

	HANDLE resumeOrExit[] = {hResumeThread, hExitThread[id-1]};

	srand(id);

	int randIndex;
	while (true)
	{
		randIndex = rand() % numArrayLength;

		EnterCriticalSection(&critSection);
		if (numArray[randIndex] == 0)
		{
			Sleep(sleepTime);

			numArray[randIndex] = id;
			markedCounter++;
			isMarked[randIndex] = true;

			Sleep(sleepTime);
			LeaveCriticalSection(&critSection);
		}
		else
		{
			LeaveCriticalSection(&critSection);
			printf(
				"id: %d, marked %d elements, unable to mark element %d \n",
				id,
				markedCounter,
				randIndex
			);
			SetEvent(hUnableToProceed[id - 1]);

			if (WaitForMultipleObjects(2, resumeOrExit, 0, INFINITE) == WAIT_OBJECT_0 + 1) 
				break;
		}
	}

	EnterCriticalSection(&critSection);
	for (int i = 0; i < numArrayLength; i++)
	{
		if (isMarked[i] == true) {
			numArray[i] = 0;
		}
	}
	LeaveCriticalSection(&critSection);

	return 0;

}

int main()
{
	printf("Enter the length of the array: ");
	scanf_s("%d", &numArrayLength);
	numArray = new int[numArrayLength];
	for (int i = 0; i < numArrayLength; i++)
	{
		numArray[i] = 0;
	}

	printf("Enter the number of Marker threads to be created: ");
	scanf_s("%d", &numberOfMarkers);

	HANDLE* hMarkers = new HANDLE[numberOfMarkers];
	hUnableToProceed = new HANDLE[numberOfMarkers];
	hExitThread = new HANDLE[numberOfMarkers];
	hResumeThread = new HANDLE[numberOfMarkers];

	bool* isClosed = new bool[numberOfMarkers];
	for (int i = 0; i < numberOfMarkers; i++)
	{
		isClosed[i] = false;
	}

	for (int i = 0; i < numberOfMarkers; i++)
	{
		hUnableToProceed[i] = CreateEvent(
			nullptr,
			false,
			false,
			nullptr
		);
		hExitThread[i] = CreateEvent(
			nullptr,
			false,
			false,
			nullptr
		);
	}

	hResumeThread = CreateEvent(
		nullptr,
		false,
		false,
		nullptr
	);

	hStartEvent = CreateEvent(
		nullptr,
		false,
		false,
		nullptr
	);

	InitializeCriticalSection(&critSection);

	int curID = 0;
	for (int i = 0; i < numberOfMarkers; i++)
	{
		hMarkers[i] = CreateThread(
			nullptr,
			0,
			&marker,
			&curID,
			0,
			nullptr
		);
	}

	SetEvent(hStartEvent);
	CloseHandle(hStartEvent);

	for (int i = 0; i < numberOfMarkers; i++) 
	{
		WaitForMultipleObjects(numberOfMarkers, hUnableToProceed, 1, INFINITE);

		int toBeClosedID = 0;
		while (true)
		{
			printf("Enter the number of the thread to be closed: ");
			scanf_s("%d", toBeClosedID);

			if (isClosed[toBeClosedID - 1] == true) {
				printf("This thread is already closed\n");
				continue;
			}

			break;
		}

		SetEvent(hExitThread[toBeClosedID - 1]);
		isClosed[toBeClosedID - 1] = true;
		CloseHandle(hMarkers[toBeClosedID - 1]);
		SetEvent(hResumeThread);
	}

	for (int i = 0; i < numberOfMarkers; i++)
	{
		CloseHandle(hExitThread[i]);
		CloseHandle(hUnableToProceed[i]);
	}
	DeleteCriticalSection(&critSection);

	delete[] numArray;
	delete[] hUnableToProceed;
	delete[] hExitThread;
	delete[] hMarkers;
	delete[] isClosed;

		
}
#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <string>

using namespace std;

// Variables
int processID;							//	The ID of the process
int numOfResources = 0;					//	The total number of resource nodes
int numOfProcesses = 0;					//	The total number of processes
struct Resource							//	Structure of the resource node
{
	int ID;
	int amount;
};
Resource *resources;					//	Array that contains all the structures of the resrouce nodes
int *available;							//	Array that contains the number of resource amounts with index as resource IDS 
struct Process
{
	int ID;
	int deadline;
	int computeTime;
};
Process **processes;					//	Array that contains all the structures of the processes and the if it is read/write
int **maxResourcePerProcess;


//	Methods
void ReadFromFile(string inputFileName);
int GetFirstIntInString(string inputString);

int main(int argc, char* argv[])
{
	//	Read, Evaluate, and Assign variables based in the input .txt file
	ReadFromFile(argv[1]);

	//	Create the process. Created process is clone of this process
	processID = fork();
	
	//	Error creating a fork
	if (processID == -1)
	{
		cout << "ERROR: creating process using fork: " << processID << endl;
		//exit();
	}
	//	Process is a child
	if (processID == 0)
	{
		cout << "I am CHILD process ID: " << processID << endl;
	}
	//	Process is a parent
	else
	{
		cout << "I am PARENT process ID: " << processID << endl;
	}

	return 0;
}

#pragma region ReadFromFile(): Read, Evaluate, and Assign variables based in the input .txt file
void ReadFromFile(string inputFileName)
{
	//	input the string argument of the input file
	fstream inputFile(inputFileName);

	//	Evaluate the input file
	if (inputFile.is_open())
	{
		cout << "\n" << "Opened file: " << inputFileName << "\n\n";

		//	Get the first line
		string currentLine;

		//	Find & Assign the amount of resources
		getline(inputFile, currentLine);
		numOfResources = GetFirstIntInString(currentLine);
		
		//	Initialize size of resources array & avaliable array
		resources = new Resource[numOfResources];
		available = new int[numOfResources];
		cout << "Resources: " << numOfResources << endl;

		//	Find & Assign the amount of processes
		getline(inputFile, currentLine);
		numOfProcesses = GetFirstIntInString(currentLine);
		
		//	Initialize size of processes array and pipe read/write size which is 2
		processes = new Process*[numOfProcesses];
		for (int i = 0; i < numOfProcesses; i++)
			processes[i] = new Process[2];
		cout << "Processes: " << numOfProcesses << "\n\n";

		//	Find & Assign the size of demands for each resource per process
		maxResourcePerProcess = new int*[numOfProcesses];
		for (int i = 1; i <= numOfProcesses * numOfResources; i++)
			maxResourcePerProcess[i] = new int[numOfResources];
		
		for (int i = 1; i <= numOfProcesses; i++)
		{
			for (int j = 1; j <= numOfResources; j++)
			{
				maxResourcePerProcess[i][j] = 7;
				cout << "[" << i << "," << j << "]" << ": " << maxResourcePerProcess[i][j] << endl;
			}
		}

		cout << endl;	//	Skip a line for neatness

		//	Determine the ID and amount of resources each resource has
		for (int i = 0; i < numOfResources; i++)
		{
			getline(inputFile, currentLine);

			//	Create new resource struct and add it to array of resources
			Resource resource;
			resources[i] = resource;
			resources[i].ID = i;
			resources[i].amount = GetFirstIntInString(currentLine);
			available[i] = resources[i].amount;
			cout << "Resource " << resources[i].ID << " has " << resources[i].amount << " amount of resources." << endl;
		}

		//	Processes
		for (int i = 0; i < numOfProcesses; i++)
		{
			//	Create new process struct and add it to array of resources
		}

		cout << endl;	//	Skip a line for neatness
	}
	else
	{
		cout << "ERROR: invalid file input or file not found." << endl;
		terminate();
	}
}
#pragma endregion

#pragma region GetFirstIntInString(): Returns the first integer in the given string
int GetFirstIntInString(string inputString)
{
	//	Loop through the string until the next char is not a number
	int i;
	for (i = 0; i < inputString.length(); i++)
		if (inputString[i] < '0' || inputString[i] >= '9')
			break;

	//	Convert the number string into an integer using string length of i
	string numberString;
	for (int j = 0; j < i; j++)
		numberString += inputString[j];

	return stoi(numberString);
}
#pragma endregion
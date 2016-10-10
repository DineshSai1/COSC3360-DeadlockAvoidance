#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <string>
#include <string.h>
#include <sstream>

using namespace std;

// Variables
int numOfResources = 0;						//	The total number of resource nodes
int numOfProcesses = 0;						//	The total number of processes
struct Resource							//	Structure of the resource node
{
	int ID;
	int amount;
};
Resource *resources;					//	Array that contains all the structures of the resrouce nodes
//int *available;							//	Array that contains the number of resource amounts with index as resource IDs 
struct Process
{
	int ID;
	int deadline;
	int computeTime;					//	Integer equal to number of requests and releases plus the parenthesized values in the calculate and useresources instructions.
	int *allocatedResources;			//	The amount of resources that is currently allocated to the process
	int *maxDemandPerResource;			//	Array representing the max amount of resources a process can demand from each resource
	string *instructions;				//	Array of instructions for the processor
	int pipeFile[2];					
};
Process *processes;						//	Array that contains all the structures of the processes
//int **maxResourcePerProcess;			//	Multidim array representing the max amount of resources a process can take from each resource
char buffer[80];						//	Character buffer length of write message
int bufferLength;

//	Methods
void ReadFromFile(string inputFileName);
int GetFirstIntInString(string inputString);
int GetMaxResourcePerProcessorValue(string inputString);
void SortProcessesByDeadline(Process arr[], int left, int right);
void CreatePipesForProcesses();
string ReadFromPipe(Process process);
void WriteToPipe(Process process, string message);
void EvaluateMessage(Process process, string message);
void calculate(Process process, int computeTime);
void request(Process process, int requestInts[]);
void release(Process process, int releaseInts[]);
void useresources(Process process, int amount);


int main(int argc, char* argv[])
{
	//	Read, Evaluate, and Assign variables based in the input .txt file supplied by command argument
	ReadFromFile(argv[1]);

	//	Sort processes by deadline and computation time by Longest Job First - LJF
	SortProcessesByDeadline(processes, 0, numOfProcesses-1);
	cout << "Sorting sequence of process execution by lowest deadline first & highest computation time..." << endl;
	for (int i = 0; i < numOfProcesses; i++)
		cout << " " << i+1 << ") Process " << processes[i].ID << " with deadline: " << processes[i].deadline << " and computation time: " << processes[i].computeTime << endl;

	cout << endl;	//	Skip a line for neatness

	//	Initialize pipes for parent-child communication
	CreatePipesForProcesses();

	cout << endl;	//	Skip a line for neatness

	for (int i = 0; i < numOfProcesses; i++)
	{
		//	Create the process and assign an temp ID for evaluation
		int processID = fork();
		
		if (processID == -1)
		{
			perror("ERROR: unable to create process.");
			exit(0);
		}

		//	Process is a CHILD
		if (processID == 0)
		{
			//	Loop through each instruction...
			for (int j = 0; j < sizeof(processes[i].instructions); j++)
			{
				WriteToPipe(processes[i], processes[i].instructions[j]);
				cout << "Child Process " << processes[i].ID << " sent instruction: " << processes[i].instructions[j] << endl;
			}

			exit(0);
		}
		//	Process is a PARENT
		else
		{
			for (int j = 0; j < sizeof(processes[i].instructions); j++)
			{
				string message = ReadFromPipe(processes[i]);
				cout << "Parent received instruction: " << message << " from Child Process " << processes[i].ID << endl;

				EvaluateMessage(processes[i], message);
			}
			
		}

		cout << endl;	//	Skip a line for neatness
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
		//available = new int[numOfResources];
		cout << "Total Resources: " << numOfResources << endl;

		//	Find & Assign the amount of processes
		getline(inputFile, currentLine);
		numOfProcesses = GetFirstIntInString(currentLine);
		
		//	Initialize size of processes array and pipe read/write size which is 2
		processes = new Process[numOfProcesses];
		//for (int i = 0; i < numOfProcesses; i++)
			//processes[i] = new Process[2];
		cout << "Total Processes: " << numOfProcesses << "\n\n";

		//	Determine the ID and amount of resources each resource has
		for (int i = 0; i < numOfResources; i++)
		{
			getline(inputFile, currentLine);

			//	Create new resource struct and add it to array of resources
			Resource resource;
			resources[i] = resource;
			resources[i].ID = i;
			resources[i].amount = GetFirstIntInString(currentLine);
			//available[i] = resources[i].amount;
			cout << "Resource " << resources[i].ID + 1 << " has " << resources[i].amount << " amount of resources." << endl;
		}

		cout << endl;	//	Skip a line for neatness

		//	Find & Assign the size of resource related parameters for the process
		for (int i = 0; i < numOfProcesses; i++)
		{
			processes[i].allocatedResources = new int[numOfResources];
			processes[i].maxDemandPerResource = new int[numOfResources];
		}
		//maxResourcePerProcess = new int*[numOfProcesses];
		//for (int i = 0; i < numOfProcesses * numOfResources; i++)
		//	maxResourcePerProcess[i] = new int[numOfResources];
		
		//	Loop through the each process and assign the value of the max value processor can demand from each resource
		for (int i = 0; i < numOfProcesses; i++)
		{
			cout << "Max resources Process " << i + 1 << " that can demand from:" << endl;
			
			for (int j = 0; j < numOfResources; j++)
			{
				//	Get new line and find value in string
				getline(inputFile, currentLine);
				processes[i].maxDemandPerResource[j] = GetMaxResourcePerProcessorValue(currentLine);
				//maxResourcePerProcess[i][j] = GetMaxResourcePerProcessorValue(currentLine);
				
				//	Display result
				cout << " Resource " << j + 1 << ": " << processes[i].maxDemandPerResource[j] << endl;
				//cout << " Resource " << j + 1 << ": " << maxResourcePerProcess[i][j] << endl;
			}
		}

		cout << endl;	//	Skip a line for neatness

		//	Loop through each process and cache their parameters
		for (int i = 0; i < numOfProcesses; i++)
		{
			//	Skip all lines until next process
			while (true)
			{
				getline(inputFile, currentLine);
				if (currentLine.find("process_") != string::npos)
					break;
			}
			
			cout << "Fetching parameters for " << currentLine << "..." << endl;

			//	ID
			processes[i].ID = i + 1;

			//	Deadline
			getline(inputFile, currentLine);
			processes[i].deadline = GetFirstIntInString(currentLine);
			cout << "Process " << i+1 << " deadline: " << processes[i].deadline << endl;
			
			//	Compute time
			getline(inputFile, currentLine);
			processes[i].computeTime = GetFirstIntInString(currentLine);
			cout << "Process " << i+1 << " compute time: " << processes[i].computeTime << endl;

			//	Calculate & Assign the amount of instructions for this process
			int instructionAmount = 0;			
			streampos originalPos = inputFile.tellg();		//	Cache line position
			while (true)
			{
				getline(inputFile, currentLine);

				//	Break loop if a "end" line is found & assign the length of instructions array
				if (currentLine.find("end") != string::npos)
				{
					processes[i].instructions = new string[instructionAmount];
					inputFile.seekg(originalPos, ios::beg);			//	Set the getline back to the original position
					break;
				}
				instructionAmount++;
			}

			cout << "Process " << i+1 << " instructions:" << endl;

			//	Loop through instructions and cache them into process string array
			for (int j = 0; j < instructionAmount; j++)
			{
				getline(inputFile, currentLine);
				processes[i].instructions[j] = currentLine;

				cout << " " << j+1 << ") " << processes[i].instructions[j] << endl;
			}

			cout << endl;	//	Skip a line for neatness
		}

		inputFile.close();
	}
	else
	{
		cout << "ERROR: invalid file input or file not found." << endl;
		exit(0);
	}
}
#pragma endregion

#pragma region GetFirstIntInString(): Returns the first integer in the given string
int GetFirstIntInString(string inputString)
{
	//	Loop through the string until the next char is not a number
	/*int i;
	for (i = 0; i < inputString.length(); i++)
		if (inputString[i] < '0' || inputString[i] >= '9')
			break;

	//	Convert the number string into an integer using string length of i
	string numberString;
	for (int j = 0; j < i; j++)
		numberString += inputString[j];

	return stoi(numberString);*/

	return stoi(inputString);			//	Updated input files no longer has comments, thus no need to do line parsing evaluation
}
#pragma endregion

#pragma region GetMaxResourcePerProcessorValue(): Returns the integer of the max value that the processor can demand from each resource node
int GetMaxResourcePerProcessorValue(string inputString)
{
	//	Find the first '=' sign which indicates that the desired value is after it
	int pos = inputString.find("=");

	//	Create a sub string for everything after the '=' sign
	string intSubString = inputString.substr(pos + 1);

	return stoi(intSubString);
}
#pragma endregion

#pragma region SortProcessesByDeadline():  Sort processes by deadline using Quick Sort Algorithm. If tied, the longer compute time takes prioity
void SortProcessesByDeadline(Process arr[], int left, int right)
{
	int i = left, j = right;
	Process temp;
	Process pivot = arr[(left + right) / 2];

	//	Partioning
	while (i <= j) 
	{
		while (arr[i].deadline < pivot.deadline)
			i++;
		while (arr[j].deadline > pivot.deadline)
			j--;
		while (arr[j].deadline == pivot.deadline && arr[j].computeTime < pivot.computeTime)
			j--;

		if (i <= j) 
		{
			temp = arr[i];
			arr[i] = arr[j];
			arr[j] = temp;
			i++;
			j--;
		}
	};

	//	Recursion
	if (left < j)
		SortProcessesByDeadline(arr, left, j);
	if (i < right)
		SortProcessesByDeadline(arr, i, right);
}
#pragma endregion

#pragma region CreatePipesForProcesses(): Creates a pipe for each process
void CreatePipesForProcesses()
{
	cout << "Creating pipes for Processes..." << endl;
	for (int i = 0; i < numOfProcesses; i++)
	{
		pipe(processes[i].pipeFile);
		
		//	If process failed to create a pipe... exit
		if (pipe(processes[i].pipeFile) == -1)
		{
			perror("ERROR: unable to create pipe.");
			exit(0);
		}
		cout << "  Created pipe for Process " << processes[i].ID << endl;
	}

	//	Initialize buffer size of char array for pipe
	bufferLength = sizeof(buffer) / sizeof(buffer[0]);
}
#pragma endregion

#pragma region ReadFromPipe():
string ReadFromPipe(Process process)
{
	close(process.pipeFile[1]);
	read(process.pipeFile[0], buffer, bufferLength);
	close(process.pipeFile[0]);
	return buffer;
}
#pragma endregion

#pragma region WriteToPipe(): Write to the 
void WriteToPipe(Process process, string message)
{
	close(process.pipeFile[0]);
	write(process.pipeFile[1], message.c_str(), bufferLength);
}
#pragma endregion

#pragma region EvaluateMessage():  Evaluate the message and run their respective methods
void EvaluateMessage(Process process, string message)
{
	if (message.find("calculate") != string::npos)
	{
		calculate(process, 1);
	}
	else if (message.find("request") != string::npos)
	{
		//cout << "Process " << process.ID << " " << message << " resources" << endl;
		
		//	Find all integers in the message and store it as an array
		int intsInMessage[numOfResources];
		int intIndexInString = 0;
		string number;
		for (int i = 0; i < message.length(); i++)
		{
			if (isdigit(message[i]))
			{	//found a digit, get the int
				for (int j = i; ; j++)
				{
					if (isdigit(message[j]))		//consecutive digits
						number += message[j];
					else
					{
						i = j - 1;		//set i to the index of the last digit
						break;
					}
				}
				intsInMessage[intIndexInString] = atoi(number.c_str());
				number = "";
				intIndexInString++;
			}
		}


		//	Perform request function and pass the parameter
		request(process, intsInMessage);
	}
	else if (message.find("release") != string::npos)
	{
		//	Find all integers in the message and store it as an array
		int intsInMessage[numOfResources];
		int intIndexInString = 0;
		string number;
		for (int i = 0; i < message.length(); i++)
		{
			if (isdigit(message[i]))
			{	//found a digit, get the int
				for (int j = i; ; j++)
				{
					if (isdigit(message[j]))		//consecutive digits
						number += message[j];
					else
					{
						i = j - 1;		//set i to the index of the last digit
						break;
					}
				}
				intsInMessage[intIndexInString] = atoi(number.c_str());
				number = "";
				intIndexInString++;
			}
		}

		//	Perform release function and pass the parameter
		release(process, intsInMessage);
	}
	else if (message.find("useresources") != string::npos)
	{
		useresources(process, 1);
	}
	else
	{
		cout << "ERROR: invalid message." << endl;
	}
}
#pragma endregion

#pragma region calculate():  calculate without using resources. wait?
void calculate(Process process, int computeTime)
{
	cout << "calculate " << computeTime << endl;
}
#pragma endregion

#pragma region request():  request vector, m integers
void request(Process process, int requestInts[])
{
	//	Cache original resource incase of failure
	int tempResourceArray[numOfResources];
	memcpy(tempResourceArray, process.allocatedResources, numOfResources);

	for (int i = 0; i < numOfResources; i++)
	{
		if (requestInts[i] > process.maxDemandPerResource[i])
		{
			cout << "Process " << process.ID << " is requesting more resources than it is allowed to demand from " << " Resource " << i+1 << endl;
			
			//	Reset the allocatedResources to their original values
			process.allocatedResources = tempResourceArray;
			break;
		}
		else
		{
			process.allocatedResources[i] += requestInts[i];
		}
	}
	
	string values = "(";
	for (int i = 0; i < numOfResources; i++)
		values += to_string(process.allocatedResources[i]) + ", ";
	values += ")";
	cout << "Process " + process.ID << " has " << values << " allocated resources." << endl;

}
#pragma endregion

#pragma region release():  release vector, m integers
void release(Process process, int releaseInts[])
{
	cout << "release resources " << endl;
}
#pragma endregion

#pragma region useresources():  use allocated resources
void useresources(Process process, int amount)
{
	cout << "use resources " << endl;
}
#pragma endregion
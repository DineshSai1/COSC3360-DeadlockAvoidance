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

/*
	Assignment 1
	COSC 3360 - Fundamentals of Operating Systems
	University of Houston
	Chad Hoang
	1176413
*/

// Variables
int numOfResources = 0;						//	The total number of resource nodes
int numOfProcesses = 0;						//	The total number of processes
struct Resource							//	Structure of the resource node
{
	int ID;
	int available;
};
Resource *resources;					//	Array that contains all the structures of the resrouce nodes
struct Process
{
	int ID;
	int deadline;
	int computeTime;					//	Integer equal to number of requests and releases plus the parenthesized values in the calculate and useresources instructions.
	int *allocatedResources;			//	The amount of resources that is currently allocated to the process per resource
	int *maxResources;					//	Array representing the max amount of resources a process need to complete process from each resource
	int *neededResources;				//	Array of how much resources needed left to complete execution per resouce
	string *instructions;				//	Array of instructions for the processor
	int pipe_ParentWriteToChild[2];			
	int pipe_ChildWriteToParent[2];			
};
Process *processes;						//	Array that contains all the structures of the processes
char buffer[1000];						//	Character buffer length of write message
int bufferLength;
int instructionsToProcess = 0;
int childProcessIndex = 0;

//	Methods
void ReadFromFile(string inputFileName);
int GetFirstIntInString(string inputString);
int GetMaxResourcePerProcessorValue(string inputString);
void SortProcessesByDeadline(Process arr[], int left, int right);
void CreatePipesForProcesses();
void EvaluateMessage(Process process, string message);
bool Safe();
void calculate(string message, Process process, int computeTime);
void request(string message, Process process, int requestInts[]);
void release(string message, Process process, int releaseInts[]);
void useresources(string message, Process process, int amount);


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

	//	Create each process with fork() evaluate its id
	int processID;
	int mainParentProcessID = getpid();
	
	//	Create child fork() processes & assign a Process variable
	Process currentProcess;
	for (int i = 0; i < numOfProcesses; i++)
	{
		currentProcess = processes[i];
		
		processID = fork();

		if (getpid() != mainParentProcessID)
			break;
	}

	//	Process failed to create
	if (processID == -1)
	{
		perror("ERROR: unable to create process.");
		exit(0);
	}
	//	Process is a CHILD
	else if (processID == 0)
	{
		//cout << endl;	//	Skip a line for neatness
		cout << "Forked Child Process: " << currentProcess.ID << endl << endl;

		//int instructionsLeft = sizeof(currentProcess.instructions);
		for (int i = 0; i < sizeof(currentProcess.instructions); i++)
		{
			//close(currentProcess.pipe_ChildWriteToParent[0]);
			write(currentProcess.pipe_ChildWriteToParent[1], currentProcess.instructions[i].c_str(), bufferLength);

			cout << "Process " << currentProcess.ID << " sent instruction: " << currentProcess.instructions[i] << endl;		
			
			//close(currentProcess.pipe_ParentWriteToChild[1]);

			while (true)
			{
				//string message = ReadFromPipe(currentProcess);
				
				read(currentProcess.pipe_ParentWriteToChild[0], buffer, bufferLength);

				string instructionFeedBack = buffer;
				
				if (instructionFeedBack.find("SUCCESS") != string::npos)
				{
					cout << "Process " << currentProcess.ID << " Completed instruction: " << instructionFeedBack << endl << endl;
					//instructionsLeft--;
					break;
				}
				else if (instructionFeedBack.find("TERMINATE") != string::npos)
				{
					cout << "Process " << currentProcess.ID << " terminated." << endl;	//	Skip a line for neatness
					exit(0);																										//instructionsLeft--;
				}
				else
					cout << "Process " << currentProcess.ID << " is listening..." << endl;
			}

			//	Close the Read end
			//close(currentProcess.pipe_ParentWriteToChild[0]);
		}

		//	Send termination message
		write(currentProcess.pipe_ChildWriteToParent[1], "TERMINATED", bufferLength);
		
		cout << "Process " << currentProcess.ID << " has no more instructions. Process terminated." << endl;	//	Skip a line for neatness

		exit(0);
	}
	//	Process if a PARENT
	else if(getpid() == mainParentProcessID)
	{
		//cout << "I am parent with fork ID: " << getpid() << endl;
		//cout << endl;	//	Skip a line for neatness

		//	Process is a PARENT
		//while(processes[0].instructions > 0)
		//for (int i = 0; i < 10; i++)
		while(numOfProcesses > 0)
		{
			//string message = ReadFromPipe(processes[0]);
			
			for (int i = 0; i < 5; i++)
			{
				//close(processes[0].pipe_ChildWriteToParent[1]);	
				read(processes[i].pipe_ChildWriteToParent[0], buffer, bufferLength);

				string instructionMessage = buffer;

				//	if 
				if (sizeof(instructionMessage) > 0)
				{
					cout << "Main Process received instruction: " << instructionMessage << " from Process " << processes[i].ID << endl;
					EvaluateMessage(processes[i], instructionMessage);
				}
			}
		}
		//close(processes[0].pipe_ChildWriteToParent[0]);
	}

	cout << endl;	//	Skip a line for neatness
	cout << "No more instructions left to process. Main Process terminating..." << endl;

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
			resources[i].available = GetFirstIntInString(currentLine);
			//available[i] = resources[i].amount;
			cout << "Resource " << resources[i].ID + 1 << " has " << resources[i].available << " amount of resources." << endl;
		}

		cout << endl;	//	Skip a line for neatness

		//	Find & Assign the size of resource related parameters for the process
		for (int i = 0; i < numOfProcesses; i++)
		{
			processes[i].allocatedResources = new int[numOfResources];
			processes[i].maxResources = new int[numOfResources];
			processes[i].neededResources = new int[numOfResources];
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

				processes[i].maxResources[j] = GetMaxResourcePerProcessorValue(currentLine);
				processes[i].neededResources[j] = processes[i].maxResources[j];
				
				//	Display result
				cout << " Resource " << j + 1 << ": " << processes[i].maxResources[j] << endl;
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

				//	increment the total amount of instructions
				instructionsToProcess++;

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
		//	If process failed to create a pipe... exit
		if (pipe(processes[i].pipe_ChildWriteToParent) == -1)
		{
			perror("ERROR: unable to create Child to Parent pipe.");
			exit(0);
		}
		if (pipe(processes[i].pipe_ParentWriteToChild) == -1)
		{
			perror("ERROR: unable to create Parent to Child pipe.");
			exit(0);
		}

		cout << " Pipe created for Process " << processes[i].ID << endl;
	}

	//	Initialize buffer size of char array for pipe
	bufferLength = sizeof(buffer) / sizeof(buffer[0]);
	cout << endl << "Character buffer length for pipes: " << bufferLength << endl;
}
#pragma endregion

#pragma region EvaluateMessage():  Evaluate the message and run their respective methods
void EvaluateMessage(Process process, string message)
{
	if (message.find("calculate") != string::npos)
	{
		calculate(message, process, 1);
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
			{	
				//found a digit, get the int
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
		request(message, process, intsInMessage);
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
		release(message, process, intsInMessage);
	}
	else if (message.find("useresources") != string::npos)
	{
		useresources(message, process, 1);
	}
	//	if process sent termination message... tell main process that it has been terminated
	else if (message.find("TERMINATED") != string::npos)
	{
		numOfProcesses--;
	}
	else
	{
		cout << "ERROR: invalid instruction message." << endl;
		exit(0);
	}
}
#pragma endregion

#pragma region Safe():  Checks if bankers algorithm is safe to continue
bool Safe()
{
	bool isSafe = true;

	return isSafe;
}
#pragma endregion

#pragma region calculate():  calculate without using resources. wait?
void calculate(string message, Process process, int computeTime)
{
	cout << "calculate stuff for Process: " << process.ID << endl;
	
	cout << message << " SUCCESS message written to Process " << process.ID << endl;

	message += "=SUCCESS";
	
	//close(process.pipe_ParentWriteToChild[0]);
	write(process.pipe_ParentWriteToChild[1], message.c_str(), bufferLength);
}
#pragma endregion

#pragma region request():  request vector, m integers
void request(string message, Process process, int requestInts[])
{
	//	Cache original resource incase of failure
	int tempResourceArray[numOfResources];
	//	Copies the array to the temp array
	memcpy(tempResourceArray, process.allocatedResources, numOfResources);
	
	//	Perform Banker's Algorithm for deadlock avoidance for each resource request
	//	First perform first 2 checks for amount allocations.
	for (int i = 0; i < numOfResources; i++)
	{
		//	if the requested resources is higher then the need...
		if (requestInts[i] > process.neededResources[i])
		{
			//	Reset the allocatedResources to their original values
			process.allocatedResources = tempResourceArray;
			
			cout << "Process " << process.ID << " is requesting more resources than it needs from Resource " << i+1 << ". Process is terminated"  << endl;
			//	Send termination message
			write(process.pipe_ChildWriteToParent[1], "TERMINATE", bufferLength);
			break;
		}
		//	if the request is more then the available...
		else if (requestInts[i] > resources[i].available)
		{
			//	Process waits
			break;
		}
	}
	//	No errors in allocation, begin the actual allocation.
	for (int i = 0; i < numOfResources; i++)
	{
		resources[i].available -= requestInts[i];
		process.allocatedResources[i] += requestInts[i];
		process.neededResources -= requestInts[i];
	}
	
	//	Check for Safe
	if (Safe())
	{
		//	Complete transaction
	}
	else
	{
		//	Process must wait
	}

	//	Create a string of the array
	string values = "(";
	for (int i = 0; i < numOfResources; i++) 
	{
		values += to_string(process.allocatedResources[i]);
		if (i < numOfResources - 1)
			values += ",";
	}
	values += ")";

	cout << "Process " << process.ID << " currently has " << values << " allocated resources." << endl;
	
	cout << message << " SUCCESS message written to Process " << process.ID << endl;
	
	message += "=SUCCESS";
	
	//close(process.pipe_ParentWriteToChild[0]);
	write(process.pipe_ParentWriteToChild[1], message.c_str(), bufferLength);

}
#pragma endregion

#pragma region release():  release vector, m integers
void release(string message, Process process, int releaseInts[])
{
	cout << "release resources for Process: " << process.ID << endl;
	
	cout << message << " SUCCESS message written to Process " << process.ID << endl;
	
	message += "=SUCCESS";
	
	//close(process.pipe_ParentWriteToChild[0]);
	write(process.pipe_ParentWriteToChild[1], message.c_str(), bufferLength);
}
#pragma endregion

#pragma region useresources():  use allocated resources
void useresources(string message, Process process, int amount)
{
	cout << "use resources for Process: " << process.ID << endl;
	

	cout << message << " SUCCESS message written to Process " << process.ID << endl;
	
	message += "=SUCCESS";

	write(process.pipe_ParentWriteToChild[1], message.c_str(), bufferLength);
}
#pragma endregion
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
	int computeTime;					//	Integer equal to number of requests and releases plus the parenthesized values in the calculate and useresources instructions.
	string *instructions;				//	Array of instructions for the processor
};
Process *processes;						//	Array that contains all the structures of the processes
int **maxResourcePerProcess;

//	Methods
void ReadFromFile(string inputFileName);
int GetFirstIntInString(string inputString);
int GetMaxResourcePerProcessorValue(string inputString);
void calculate(int computeTime);
void request();
void release();
void useresources(int value);



int main(int argc, char* argv[])
{
	//	Read, Evaluate, and Assign variables based in the input .txt file supplied by command argument
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
		processes = new Process[numOfProcesses];
		//for (int i = 0; i < numOfProcesses; i++)
			//processes[i] = new Process[2];
		cout << "Processes: " << numOfProcesses << "\n\n";

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
			cout << "Resource " << resources[i].ID + 1 << " has " << resources[i].amount << " amount of resources." << endl;
		}

		cout << endl;	//	Skip a line for neatness

		//	Find & Assign the size of demands for each resource per process
		maxResourcePerProcess = new int*[numOfProcesses];
		for (int i = 0; i < numOfProcesses * numOfResources; i++)
			maxResourcePerProcess[i] = new int[numOfResources];
		
		//	Loop through the each process and assign the value of the max value processor can demand from each resource
		for (int i = 0; i < numOfProcesses; i++)
		{
			cout << "Max resources Process " << i + 1 << " that can demand from:" << endl;
			
			for (int j = 0; j < numOfResources; j++)
			{
				//	Get new line and find value in string
				getline(inputFile, currentLine);
				maxResourcePerProcess[i][j] = GetMaxResourcePerProcessorValue(currentLine);
				
				//	Display result
				cout << "Resource " << j + 1 << ": " << maxResourcePerProcess[i][j] << endl;
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
			
			cout << "Parameters for " << currentLine << endl << endl;

			//	Deadline
			getline(inputFile, currentLine);
			processes[i].deadline = GetFirstIntInString(currentLine);
			cout << "Process " << i+1 << " deadline: " << processes[i].deadline << endl;
			
			//	Compute time
			getline(inputFile, currentLine);
			processes[i].computeTime = GetFirstIntInString(currentLine);
			cout << "Process " << i+1 << " compute time: " << processes[i].computeTime << endl << endl;

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

			cout << "Instructions:" << endl;

			//	Loop through instructions and cache them into process string array
			for (int j = 0; j < instructionAmount; j++)
			{
				getline(inputFile, currentLine);
				processes[i].instructions[j] = currentLine;

				cout << " " << j+1 << ") " << processes[i].instructions[j] << endl;
			}

			cout << endl;	//	Skip a line for neatness
		}
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

#pragma region calculate():  calculate without using resources. wait?
void calculate(int computeTime)
{
	
}
#pragma endregion

#pragma region request():  request vector, m integers
void request()
{

}
#pragma endregion

#pragma region release():  release vector, m integers
void release()
{

}
#pragma endregion

#pragma region useresources():  use allocated resources
void useresources(int value)
{

}
#pragma endregion
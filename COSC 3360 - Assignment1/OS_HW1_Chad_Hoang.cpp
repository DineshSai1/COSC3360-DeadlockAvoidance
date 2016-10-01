#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <fstream>
#include <string>

using namespace std;

// Variables
int processID;
int numOfResources = 0;
int numOfProcesses = 0;
struct Resource 
{
	int ID;
	int amount;
};
Resource *Resources;
struct Process
{
	int ID;
	int deadline;
	int computeTime;
};

int GetFirstIntInString(string inputString);

int main(int argc, char* argv[])
{

	//	input the string argument of the input file
	fstream inputFile(argv[1]);
	
	//	Evaluate the input file
	if (inputFile.is_open())
	{
		cout << "\n" << "Opened file: " << argv[1] << "\n\n";
		
		//	Get the first line
		string currentLine;	
		
		//	Find & Assign the amount of resources
		getline(inputFile, currentLine);
		numOfResources = GetFirstIntInString(currentLine);
		Resources = new Resource[numOfResources];
		cout << "Resources: " << sizeof(Resources)/ sizeof(Resources[0]) << endl;

		//	Find & Assign the amount of processes
		getline(inputFile, currentLine);
		numOfProcesses = GetFirstIntInString(currentLine);
		cout << "Processes: " << numOfProcesses << "\n\n";

		//	Determine the ID and amount of resources each resource has
		for (int i = 0; i < numOfResources; i++)
		{
			getline(inputFile, currentLine);
			
			//	Create new resource struct and add it to array of resources
			Resource resource;
			Resources[i] = resource;
			Resources[i].ID = i;
			Resources[i].amount = GetFirstIntInString(currentLine);
			cout << "Resource " << Resources[i].ID << " has " << Resources[i].amount << " amount of resources." << endl;
		}

		cout << endl;	//	Skip a line for neatness
	}
	else
	{
		cout << "ERROR: invalid file input or file not found." << endl;
		return 0;
	}


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
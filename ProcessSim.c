#include <cctype> // For toupper()
#include <cstdlib> // For EXIT_SUCCESS and EXIT_FAILURE
#include <cstring> // For strerror()
#include <cerrno> // For errno
#include <deque> // For deque (used for ready and blocked queues)
#include <fstream> // For ifstream (used for reading simulated programs)
#include <iostream> // For cout, endl, and cin
#include <sstream> // For stringstream (used for parsing simulated programs)
#include <sys/wait.h> // For wait()
#include <unistd.h> // For pipe(), read(), write(), close(), fork(), and _exit()
#include <vector> // For vector (used for PCB table)

using namespace std;

class Instruction {
public:
    char operation;
    int intArg;
    string stringArg;
};

class Cpu {
public:
    vector<Instruction> *pProgram;
    int programCounter;
    int value;
    int timeSlice;
    int timeSliceUsed;
};

enum State {
    STATE_READY,
    STATE_RUNNING,
    STATE_BLOCKED,
    STATE_TERMINATED
};

class PcbEntry {
public:
    int processId = -1;
    int parentProcessId;
    vector<Instruction> program;
    unsigned int programCounter = 0;
    int value = 0;
    unsigned int priority = 0;
    State state;
    unsigned int startTime = 0;
    unsigned int timeUsed = 0;
};


PcbEntry pcbEntry[10];
unsigned int timestamp = 0;
Cpu cpu;

// For the states below, -1 indicates empty (since it is an invalid index).
int runningState = -1;
deque<int> readyState;
deque<int> blockedState;

// In this implementation, we'll never explicitly clear PCB entries and the index in
// the table will always be the process ID. These choices waste memory, but since this
// program is just a simulation it the easiest approach. Additionally, debugging is
// simpler since table slots and process IDs are never re-used.

double cumulativeTimeDiff = 0;
int numTerminatedProcesses = 0;


string trim(const string &str) { //Helper function to trim whitespace from a string, used in createProgram
    size_t first = str.find_first_not_of(' ');
    if (string::npos == first) {
        return "";
    }
    size_t last = str.find_last_not_of(' ');
    return str.substr(first, last - first + 1);
}

bool createProgram(const string &filename, vector<Instruction> &program) {
    ifstream file;
    int lineNum = 0;

    file.open(filename.c_str());

    if(!file.is_open()) {   
        cout << "Error opening file " << filename << endl;
        return false;
    }

    while(file.good()) {
        string line;
        getline(file, line);
        
        line = trim(line);

        if(line.size() > 0) {
            Instruction instruction;
            instruction.operation = toupper(line[0]);
            instruction.stringArg = trim(line.erase(0, 1));

            stringstream argStream(instruction.stringArg);

            switch(instruction.operation) {
                case 'S': // Integer argument
                case 'A': // Integer argument
                case 'D': // Integer argument
                case 'F': // Integer argument
                    if(!(argStream >> instruction.intArg)) {
                        cout << filename << ":" << lineNum
                            << " - Invalid integer argument "
                            << instruction.stringArg << " for "
                            << instruction.operation << " operation"
                            << endl;
                        file.close();
                        return false;
                    }
                    break;
                case 'B': // No argument
                case 'E': // No argument
                    break;
                case 'R': // Integer argument
                    // Note that since the string is trimmed on both ends, filenames
                    // with leading or trailing whitespace(unlikely) will not work
                    if(instruction.stringArg.size() == 0) {
                        cout << filename << ":" << lineNum << " - Missing string argument"
                            << endl;
                        file.close();
                        return false;
                    }
                    break;
                default:
                    cout << filename << ":" << lineNum << " - Invalid operation, "
                        << instruction.operation << endl;
                    file.close();
                    return false;
            }

            program.push_back(instruction);
        }

        lineNum++;
    }

    file.close();
    return true;
}


// Implements the S operation
void set(int value) {
    // TODO: Implement
    // 1. Set the CPU value to the passed-in value
    cpu.value = value;
}


// Implements the A operation
void add(int value) {
    // TODO: Implement
    // 1. Add the passed-in value to the CPU value
    cpu.value += value;
}


// Implements the D operation
void decrement(int value) {
    // TODO: Implement
    // 1. Subtract the integer value from the CPU value;
    cpu.value -= value;
}


// Perfor,s scheduling
void schedule() {
    // TODO: Implements
    // 1. Return if there is still a processing running (runningState != -1)
    // There is no need to schedule if a process is already running (at least until iLab 3)
    if (runningState != -1) {
        return;
    }
    // 2. Get a new process to run, if possible, from the ready queue.
    if (readyState.empty()) {
        return;
    }
    // 3. If we were able to get a new process to run:
    //      a. Mark the processing as running (update the new process's PCB state)
    //      b. Update the CPU structure with the PCB entry details (program, program counter, value, etc.)
    int new_process = readyState.front();
    readyState.pop_front();
    pcbEntry[new_process].state = STATE_RUNNING;

    cpu.pProgram = &(pcbEntry[new_process].program);
    cpu.programCounter = pcbEntry[new_process].programCounter;
    cpu.value = pcbEntry[new_process].value;
    runningState = new_process;
}


// Implements the B operation
void block() {
    // TODO: Implement
    // 1. Add the PCB index of the running process (stored in runningState) to the blocked queue
    if (runningState != -1) {
        blockedState.push_back(runningState);
    

    // 2. Update the process's PCB entry
    //      a. Change the PCB's state to blocked
    //      b. Store the CPU program counter in the PCB's program counter
    //      c. Store the CPU's value in the PCB's value
        pcbEntry[runningState].state = STATE_BLOCKED;
        pcbEntry[runningState].programCounter = cpu.programCounter;
        pcbEntry[runningState].value = cpu.value;
    

    // 3. Update the running state to -1 (basically mark no process as running)
        runningState = -1;
    }

    // - Note that a new process will be chosen to run later (via the Q command code calling the schedule() function)
}


// Implements the E operation
void end() {
    // TODO: Implement

    // Return if no process is running
    if (runningState == -1) {
        cout << "No processes are running" << endl;
        return;
    }

    // 1. Get the PCB entry of the running process
    PcbEntry &runningProcess = pcbEntry[runningState];

    // 2. Update the cumulative time difference (increment it by timestamp + 1 - start time of the process)
    cumulativeTimeDiff += timestamp + 1 - runningProcess.startTime;

    // 3. Increment the number of terminated processes
    runningProcess.state = STATE_TERMINATED;
    numTerminatedProcesses++;

    // 4. Update the running state to -1 (basically mark no process as running)
    runningState = -1;

    // - Note that a process will be chosen to run later (via the Q command code calling the schedule function)
}


// Implements the F operation
void fork(int value) {
    // TODO: Implement
    // 1. Get a free PCB index (pcbTable.size())
    int freePCB = -1;
    for (int i = 0; i < 10; ++i) {
        if (pcbEntry[i].processId == -1) {
            freePCB = i;
            break;
        }
    }
    if (freePCB == -1){
        cout << "No free PCBs available." << endl;
        return;
    }

    // 2. Get the PCB entry for the current running process
    PcbEntry &runningProcess = pcbEntry[runningState];

    // 3. Ensure the passed-in value is not out of bounds
    if (value < 0 || value >= cpu.pProgram->size()) {
        cout << "Invalid fork value." << endl;
        return;
    }

    // 4. Populate the PCB entry obtained in #1
    //      a. Set the process ID to the PCB index obtained in #1
    //      b. Set the parent process ID to the process ID of the running process
    //          (use the running process's PCB entry to get this)
    //      c. Set the program counter to the cpu program counter
    //      d. Set the value to the cpu value
    //      e. Set the priority to the same as the parent process's priority
    //      f. Set the state to the ready state
    //      g. Set the start time to the current timestamp
    pcbEntry[freePCB].processId = freePCB;
    pcbEntry[freePCB].parentProcessId = runningProcess.processId;
    pcbEntry[freePCB].programCounter = cpu.programCounter;
    pcbEntry[freePCB].value = cpu.value;
    pcbEntry[freePCB].priority = runningProcess.priority;
    pcbEntry[freePCB].state = STATE_READY;
    pcbEntry[freePCB].startTime = timestamp;
    pcbEntry[freePCB].timeUsed = 0;


    // 5. Add the pcb index to the ready queue
    readyState.push_back(freePCB);

    // 6. Increment the cpu's program counter by the value read in #3
    cpu.programCounter += value;

}


// Implements the R operation
void replace(string &argument) {
    // TODO: Implement
    // 1. Clear the CPU's program (cpu.pProgram->clear())
    cpu.pProgram->clear();

    // 2. Use createProgram() to read in the filename specified by argurment into the CPU (*cpu.pProgram)
    //      a. Consider what to do if createProgram fails. I printed an error, incremented the cpu program
    //          counter and then returned.
    //          - Note that createProgram can fail if the file could not be opened or did not exist
    if (!createProgram(argument, *cpu.pProgram)) {
        cout << "Failed to load program from file: " << argument << endl;
        ++cpu.programCounter;
        return;
    }

    // 3. Set the program counter to 0
    cpu.programCounter = 0;

}


// Implements the Q command
void quantum() {
    Instruction instruction;
    cout << "In quantum";

    if(cpu.programCounter < cpu.pProgram->size()) {
        instruction = (*cpu.pProgram)[cpu.programCounter];
        ++cpu.programCounter;
    }
    else {
        cout << " End of program reached without E operation" << endl;
        instruction.operation = 'E';
    }
    switch (instruction.operation) {
        case 'S':
            set(instruction.intArg);
            cout << " instruction S " << instruction.intArg << endl;
            break;
        case 'A':
            add(instruction.intArg);
            cout << " instruction A " << instruction.intArg << endl;
            break;
        case 'D':
            decrement(instruction.intArg);
            cout << " instruction D " << instruction.intArg << endl;
            break;
        case 'B':
            block();
            cout << " instruction B, block " << endl;
            break;
        case 'E':
            end();
            cout << " instruction E, end " << endl;
            break;
        case 'F':
            fork(instruction.intArg);
            cout << " instruction F, fork " << endl;
            break;
        case 'R':
            replace(instruction.stringArg);
            cout << " instruction R, replace " << endl;
            break;
    }

    ++timestamp;
    schedule();
}


// Implements the U command
void unblock() {
    // TODO: Implement
    // 1. If the blocked queue contains any processes:
    //      a. Remove a process form the front of the blocked queue.
    //      b. Add the process to the ready queue.
    //      c. Change the state of the process to ready (update its PCB entry).
    if (!blockedState.empty()) {
        int unblockedProcess = blockedState.front();
        blockedState.pop_front();
        readyState.push_back(unblockedProcess);
        pcbEntry[unblockedProcess].state = STATE_READY;
    }

    // 2. Call the schedule() function to give an unblocked process a chance to run (if possible).
    schedule();
}


// Implement the P command
void print() {
    printf("CURRENT TIME: %u\n", timestamp);

    printf("\nRUNNING PROCESS:\n");
    if (runningState != -1) {
        PcbEntry *runningProcess = &pcbEntry[runningState];
        printf("%d, %d, %u, %d, %u, %u\n", 
               runningProcess->processId, 
               runningProcess->parentProcessId, 
               runningProcess->priority, 
               runningProcess->value, 
               runningProcess->startTime, 
               runningProcess->timeUsed);
    } else {
        printf("None\n");
    }

    printf("\nBLOCKED PROCESSES:\n");
    if (!blockedState.empty()) {
        printf("Queue of blocked processes:\n");
        for (size_t i = 0; i < blockedState.size(); ++i) {
            int pid = blockedState[i];
            PcbEntry *blockedProcess = &pcbEntry[pid];
            printf("%d, %d, %u, %d, %u, %u\n", 
                   blockedProcess->processId, 
                   blockedProcess->parentProcessId, 
                   blockedProcess->priority, 
                   blockedProcess->value, 
                   blockedProcess->startTime, 
                   blockedProcess->timeUsed);
        }
    } else {
        printf("None\n");
    }

    printf("\nPROCESSES READY TO EXECUTE:\n");
    if (!readyState.empty()) {
        unsigned int priorityQueues[10][10]; // max 10 priorities with max 10 processes each
        size_t priorityCounts[10] = {0};

        for (size_t i = 0; i < readyState.size(); ++i) {
            int pid = readyState[i];
            unsigned int priority = pcbEntry[pid].priority;
            priorityQueues[priority][priorityCounts[priority]++] = pid;
        }

        for (unsigned int priority = 0; priority < 10; ++priority) {
            if (priorityCounts[priority] > 0) {
                printf("Queue of processes with priority %u:\n", priority);
                for (size_t i = 0; i < priorityCounts[priority]; ++i) {
                    int pid = priorityQueues[priority][i];
                    PcbEntry *readyProcess = &pcbEntry[pid];
                    printf("%d, %d, %d, %u, %u\n", 
                           readyProcess->processId, 
                           readyProcess->parentProcessId, 
                           readyProcess->value, 
                           readyProcess->startTime, 
                           readyProcess->timeUsed);
                }
            }
        }
    } else {
        printf("None\n");
    }
}


void calculateTurnaroundTime() {
    // Calculate the average turnaround time
    int avgTurnaroundTime = cumulativeTimeDiff / numTerminatedProcesses;
    cout << "Average turnaround time: " << avgTurnaroundTime << endl;
}

// Function that implements the process manager
int runProcessManager(int fileDescriptor) {
    //vector<PcbEntry> pcbTable;
    // Attempt to create the init process.
    if(!createProgram("init", pcbEntry[0].program)) {
        return EXIT_FAILURE;
    }

    pcbEntry[0].processId = 0;
    pcbEntry[0].parentProcessId = -1;
    pcbEntry[0].programCounter = 0;
    pcbEntry[0].value = 0;
    pcbEntry[0].priority = 0;
    pcbEntry[0].state = STATE_RUNNING;
    pcbEntry[0].startTime = 0;
    pcbEntry[0].timeUsed = 0;

    runningState = 0;
    
    cpu.pProgram = &(pcbEntry[0].program);
    cpu.programCounter = pcbEntry[0].programCounter;
    cpu.value = pcbEntry[0].value;

    timestamp = 0;
    double avgTurnaroundTime = 0;

    // Loop until a 'T' is read, then terminate
    char ch;

    do {
        // Read a command character from the pipe.
        if (read(fileDescriptor, &ch, sizeof(ch)) != sizeof(ch)) {
            // Assume the parent process exited, breaking the pipe.
            break;
        }
        // TODO: Write a switch statement
        switch(ch) {
            case 'Q': 
                quantum();    
                break;
            case 'U':
                unblock();
                break;
            case 'P':
                print();
                break;
            case 'T':
                // Calculate the average turnaround time
                calculateTurnaroundTime();
                break;
            default:
                cout << "You entered an invalid character!" << endl;
        }
    } while(ch != 'T');
    // following three lines is how it was written in file; not sure if thats how it's suupposed to be:
    // }while(ch != 'T');
    // return EXIT_SUCCESS;
    // }

    // the do-while is kind of weird, i think its something like thiis so its like do____ while (condition LOL)

    return EXIT_SUCCESS;
}



int main(int argc, char *argv[]) {
    int pipeDescriptors[2];
    pid_t processMgrPid;
    char ch;
    int result;

    //TODO: Create a pipe
    pipe(pipeDescriptors);
    
    
    //USE fork() SYSTEM CALL to create the child process and save the value returned in processMgrPid variable
    if((processMgrPid = fork()) == -1) exit(1); /* FORK FAILED */
    if(processMgrPid == 0) {
        // The process manager process is running.
        // Close the unused write end of the pipe for the process manager 
       close(pipeDescriptors[1]);
        
        // Run the process manager.
        result = runProcessManager(pipeDescriptors[0]);
        
        // Close the read end of the pipe for the process manager process (for cleanup purposes).
        close(pipeDescriptors[0]);
        _exit(result);
    }
    else {
        // The commander process is running.
        
        // Close the unused read end of the pipe for the commander process.
        close(pipeDescriptors[0]);
        
        // Loop until a 'T' is written or until the pipe is broken.
        do { 
            cout << "Enter Q, P, U or T" << endl;
            cout << "$ ";
            cin >> ch ;
        
            // Pass commands to the process manager process via the pipe.
            if (write(pipeDescriptors[1], &ch, sizeof(ch)) != sizeof(ch)) {
                // Assume the child process exited, breaking the pipe.
                break;
            }
        } 
        while (ch != 'T');
            write(pipeDescriptors[1], &ch, sizeof(ch));

        // Close the write end of the pipe for the commander process (for cleanup purposes).
        close(pipeDescriptors[1]);
    
        // Wait for the process manager to exit.
        
        wait(&result);
    }

    return result;
}

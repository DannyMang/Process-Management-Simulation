Main tasks are:

Commander Process:

Create a pipe for communication.
Create the process manager process.
Read commands (Q, U, P, T) from standard input and pass them to the process manager via the pipe.


Process Manager Process:

Simulate five process management functions:

Process creation
Replacing process image
Process state transition
Process scheduling
Context switching


Maintain data structures for time, CPU, process control blocks (PCBs), ready queue, blocked queue, and running process.
Process commands from the commander:

Q: Execute next instruction of the running process, update time, and perform scheduling.
U: Move the first process from the blocked queue to the ready queue.
P: Spawn a new reporter process to print the system state.
T: Spawn a reporter process and terminate after its completion.


Execute instructions of the simulated processes (set, add, subtract, block, terminate, fork, replace image).
Implement a scheduling policy (e.g., multiple queues with priority classes and time slices).
Perform context switching when switching between processes.


Reporter Process:

Print the current state of the system (time, CPU details, PCB table, ready queue, blocked queue, running process) to standard output.
Terminate after printing the state.
# Threads and Processes
This document explains what threads and process are, and how they are implemented in Hamster. The code snippets within are simplified for clarity, and may not represent the actual code exactly.

## The Thread
A thread is the smallest unit of execution that can be scheduled by the operating system. In Hamster, this is called a `Task`. Each task has its own CPU context (registers, program counter, etc.), and it shares resources with other tasks in the same process, such as memory space and file descriptors.

Every Task has a unique Task ID (TID) that identifies it within the system. Tasks can be created, scheduled, and terminated independently. In Hamster, the thread ID is not reused in the same run of the OS.
```cpp
// src/process/task.hpp
class Task {
private:
    // This is the emulator. It stores its own CPU context for the task
    RiscVEmulator emulator;

    // The memory space of the task is here, usually shared with other tasks in the same process
    SharedPtr<Memory> memory;

    // The file descriptor table of the task holds its open files. This is also usually shared with other tasks in the same process
    SharedPtr<TaskFDTable> fd_table;

    // The process that this task belongs to. See below.
    SharedPtr<Process> process;

    // The Task's ID. Unique for each task. Not recycled.
    int tid;
};
```

## `Process`
A process, represented by the `Process` class, is a collection of one or more tasks that share the same resources. Each process also has its own unique ID, called the Process ID (PID).

Every process also has a leader task, which is the first task created in the process. Usually, the leader task is the one that manages the process's lifecycle, such as starting and terminating it. The PID is the same as the TID of the leader task.
```cpp
// src/process/task.hpp
class Process {
private:
    // The leader task of the process
    Task *leader;

    // The Process's ID. Same as the TID of the leader task
    int pid;

    // Other tasks in the process
    Set<Task*> tasks;
};
```

## Summary
- A **Task** is a thread of execution with its own CPU context, sharing resources with other tasks in the same process.
- A **Process** is a collection of tasks that share resources, with a unique PID and a leader task.
- Both tasks and processes have unique IDs that are not reused during the OS's runtime.

## Additional Reading
Hamster's process and thread management is similar to that of Linux. For more detailed information, see Linux documentation

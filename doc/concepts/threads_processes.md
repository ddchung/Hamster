# Threads and Processes

Threads are the smallest unit of execution within a process. A process can contain multiple threads, all sharing the same memory space but executing independently. This allows for concurrent execution of tasks within the same application. Applications can create multiple threads to perform different tasks simultaneously, for example, handling user input while performing background computations.

A process is a group of one or more threads that share some system resources, such as a virtual memory space, open files, and signal handlers.

```
------------------------------------------------
|  Process                                     |
|   ------------------   ------------------    |
|   | Virtual Memory |   | Resources      |    |
|   ------------------   | (Files, etc.)  |    |
|                        ------------------    |
|   ------------------                         |
|   | Signal Handlers|                         |
|   ------------------                         |
|==============================================|
|   Thread 1          Thread 2          ...    |
|  (Main Thread)     (Worker Thread)           |
------------------------------------------------
```


## Creating Threads

A process starts off with one thread. Additional threads can be created using various functions provided by the C library, such as `pthread_create`.

## Creating Processes

Processes are able to duplicate themselves using the `fork` function. This creates a new process, which is known as the child of the original process, known as the parent. The child process starts off as an exact copy of the parent process, but has an independent memory space and resources.

Usually, after a `fork`, the child process will load a new program, using the `exec` family of functions. This replaces the child's memory space with a new program.

`exec()` will destroy all threads in a process, and will start the new program from the beginning, with a single main thread.

```
-------------
| Process A |
| PID: 1234 |
| (bash)    |
-------------

# Fork: duplicates a process
# Here, Process A forks to create an identical Process B
-------------                    -------------
| Process A |   === fork() ===>  | Process B |
| PID: 1234 |                    | PID: 1235 |
| (bash)    |                    | (bash)    |
-------------                    -------------

# Exec: loads a new program into a process
# Now, Process B loads a new program (ls) using exec
-------------                    -------------
| Process A |                    | Process B |
| PID: 1234 |                    | PID: 1235 |
| (bash)    |                    | (ls)      |
-------------                    -------------
```

## Process Termination

When programs complete their execution, they terminate using the `exit()` function. This function performs necessary cleanup and releases resources held by the process. The operating system then reclaims the memory and other resources used by the terminated process.

In addition to termination, `exit()` also notifies the parent process of the termination, allowing it to know that its child has finished.

For example, when you type `ls` into a shell, the shell process forks a new process. This new process will remember that it needs to run `ls`, and will call `exec()` to load the new program.

When `ls` finishes listing the files, it will call `exit()`, which will clean up and delete the `ls` process. The kernel will also notify the shell of `ls`'s termination, so the shell can continue to print a prompt and wait for more input.

```
$ ls   # When you press enter here, the shell forks a new process
# The new process will run `exec()` to load the `ls` program
# When `exec()` is called, it stops the current process and replaces it with `ls`, starting from the beginning.
# Now, `ls` will do its job and run normally
file1.txt  file2.txt  README.md
# Here, `ls` is done running, so it calls `exit()`
# The kernel cleans up the `ls` process and notifies the shell that `ls` has finished
$      # The shell sees that `ls` has finished, and prints a new prompt
```

## Thread scheduling

From their perspective, all threads appear to run simultaneously. However, in reality, there are not enough CPU cores to run all of the thousands of possible threads at the same time. Thus, the kernel uses a technique known as time slicing, which runs each thread for a short amount of time (a time slice), switching rapidly between all the threads.

In this example, there are three threads: A, B, and C, which all share a single CPU core. The times labeled are purely hypothetical, and for the example only.
```

Time: 0ms    3ms     7ms    10ms    16ms    21ms
      |------|-------|-------|-------|-------|
      | A    | B     | C     | A     | B     | ...
      |------|-------|-------|-------|-------|
```

This rapid switching between threads gives the illusion that all threads are running at the same time, even though only one thread is actually executing on the CPU core at any given moment. This allows for efficient multitasking and responsiveness in a multi-threaded environment.

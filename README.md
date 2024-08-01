# Mercator Series Calculation with semaphores
This C program calculates the Mercator series for ln(1 + x) using concurrent processes. The program utilizes shared memory and semaphores for synchronization between processes. It divides the work of calculating the series terms among multiple processes, each computing a portion of the series in parallel. (For linux systems because of the used libraries)
**Key Features:**
+ Concurrent Processing: Utilizes fork() to create multiple child processes to calculate the series in parallel.
- Shared Memory: Uses shared memory to store partial sums calculated by each process.
Semaphores: Implements semaphores to coordinate the start and completion of processes, ensuring proper synchronization.
Time Measurement: Measures the elapsed time for the entire computation process.
Memory Cleanup: Includes proper cleanup of shared memory and semaphores at the end of the execution.

# Mercator Series Calculation with Message Queues
This C program calculates the Mercator series for ln(1 + x) using concurrent processes and message queues for inter-process communication. The program distributes the task of calculating the series across multiple processes (slaves), which send their partial results to a master process using a message queue. The master process then aggregates these results to compute the final value.

**Key Features:**
  Concurrent Processing: Utilizes fork() to create multiple child processes (slaves), each responsible for calculating a portion of the series.
  Message Queues: Implements message queues (mqueue.h) for efficient communication between the slave processes and the master process.
  Dynamic Calculation: Each slave process calculates specific terms of the Mercator series and sends the results to the master process.
  Time Measurement: Measures the elapsed time for the entire computation process.
  Clean Resource Management: Properly handles the opening, closing, and unlinking of the message queue to prevent resource leaks.

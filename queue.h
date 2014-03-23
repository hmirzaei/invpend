//---------------------------------------------------------------
// File: Code130_Queue.h
// Purpose: Header file for a demonstration of a queue implemented 
//        as an array.  Data type: Character
// Programming Language: C
// Author: Dr. Rick Coleman
//---------------------------------------------------------------
#ifndef CODE130_QUEUE_H
#define CODE130_QUEUE_H


#define MAX_SIZE    10000        // Define maximum length of the queue

// List Function Prototypes
void InitQueue();             // Initialize the queue
void ClearQueue();            // Remove all items from the queue
int Enqueue(long data);         // Enter an item in the queue
long Dequeue();               // Remove an item from the queue
int isEmpty();                // Return true if queue is empty
int isFull();                 // Return true if queue is full

// Define TRUE and FALSE if they have not already been defined
#ifndef FALSE
#define FALSE (0)
#endif
#ifndef TRUE
#define TRUE (!FALSE)
#endif

#endif // End of queue header

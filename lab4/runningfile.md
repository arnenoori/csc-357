# Inactivity Detector

This program detects inactivity on the keyboard input and prints a message when no activity is detected for at least 10 seconds as per the requirements of the lab

## How to compile and run the program

1. Save the source code in a file named `inactivity_detector.c`.
2. Open a terminal and navigate to the directory containing the source file.
3. Compile the program using the following command: 
gcc -o inactivity_detector inactivity_detector.c
4. Run the compiled program with the following command:
./inactivity_detector`

## How the code works

The program creates a parent process and a child process. The parent process is responsible for reading words from the keyboard, updating the text, and printing it. The child process is responsible for detecting inactivity.

The parent and child processes communicate using a shared memory flag (`activity_flag`) and a pipe. The shared memory flag is used to indicate whether any activity has been detected or not. The pipe is used to temporarily replace the stdin when the child process detects inactivity and prints the inactivity message.

The parent process reads the input using `fgets()`, updates the text by adding exclamation marks at the beginning and the end of the word, and then prints the updated text. If the user enters "quit", the parent process terminates the child process, waits for it to finish, and then exits.

The child process waits for 10 seconds using `sleep()`. After 10 seconds, it checks the shared memory flag to see if there was any activity from the parent process. If there was no activity, it overwrites stdin using `dup2()` and prints "Inactivity detected!". If there was activity, it resets the shared memory flag and continues waiting for another 10 seconds.

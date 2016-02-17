/*
Project 4 CS3013 A Term 2015 
Jacob Watson, JRWatson
JRWatson@wpi.edu
THIS PROGRAM MUST BE RUN ON ONE OF THE UBUNTU VIRTUAL MACHINES FROM PROJECT 2
This program provides information on the files in the current directory in the form of a printed message to the terminal screen. It can be run in two modes:
./proj4
which runs it in serial mode, and:
./proj4 thread #
which runs it in multithreading mode and where # is a number between 1 and 15 which determines how many threads will be used

the program can be compiled using:
g++ -std=c++11 proj4.cpp -o proj4 -lpthread

When the program runs, it will not show any output until it gets an EOF. Until then, it will just allow the user to enter lines of text, which should be filenames.
This program may leak memory.

This program either runs a loop which continues to pull in from cin a filename, and run stat on it and adds the results into a running count, or it uses multithreading, where it assigns a filename to a dynamically allocated char array, one for each possible thread, and then creates a thread to examine that char array for a file with the same name, which it then uses to run stat. It than takes the results from that stat, classifies the file, and tries to access the global total files, which are the same ones used in the serial mode, as they are global. The thread code that allows modifying the globals is mutex locked, so the thread must wait its turn to change any of the globals. Only one thread may modify the globals at once. The main thread intializes all possible threads, then waits for the first one to finish, reassigns it, and so on till all files have been looked at. Afterwards, the main thread waits for every thread to finish, then prints out the global counts. Then the program terminates.
*/

#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string>
#include <stdio.h>
#include <cstring>
#include <mutex>
char filename[256]; //holds the file name that is read in
  int badfile; //holds number of bad files
  int direct; //holds number of directories
  int regfiles; //holds number of regular files
  int specfiles; //holds number of special files
  long bytecount; //holds number of bytes used by regular files
  int textfiles; //holds number of files with text
  long bytestext; //holds number of bytes used by text files
  int isbad; //holds the return value of stat
  struct stat filestat; //holds the output of the stat command
std::mutex threadlock; //mutex which prevents the threads from adding to the totals at the same time
  int fdIn; //input file fd
  char charcheck[1]; //character check buffer
  int cnt; //holds return value of read
  int filesgood; //used to check if a file is a text file
int usethreads; //set to indicate threads should be used
int threadcount; //keeps track of all the active threads
int totalthreads; //keeps track of the total number of threads


char **ptr[15]; //points to the filename[#] char array for each thread

//these pointers are dynamically allocated to hold the filename for each thread
char *filename1 = NULL;
char *filename2 = NULL;
char *filename3 = NULL;
char *filename4 = NULL;
char *filename5 = NULL;
char *filename6 = NULL;
char *filename7 = NULL;
char *filename8 = NULL;
char *filename9 = NULL;
char *filename10 = NULL;
char *filename11 = NULL;
char *filename12 = NULL;
char *filename13 = NULL;
char *filename14 = NULL;
char *filename15 = NULL;

pthread_t threads[15]; //pointers to the fifteen threads

//this routine handles the operations of the threads, it takes in a filename, checks that stat information, and then adds it to the totals before terminating
void *threadroutine (void *ptr){
  char *message; //holds incoming file name
  int badfilethread = 0; //badfiles info found by thread
  int directthread = 0; //same for directories
  int regfilesthread = 0; //same for regular files
  int specfilesthread = 0; //same for special files
  long bytecountthread = 0; //holds the number of bytes found by thread
  int textfilesthread = 0; //holds number of text files found by thread
  long bytestextthread = 0; //holds the number of bytes in text files found by thread
  int isbadthread = 0; //used to determine if the filename the thread is looking at is bad
  struct stat filestatthread; //holds output of stat() for thread
  int fdInthread; //used to read threads file
  char charcheckthread[1]; //used to check if thread is a text file
  int cntthread = 0; //holds return value for threads read
  int filesgoodthread = 0; //used as boolean to tell if the threads file is a text file
  isbadthread = 0; //setting default

  message = (char *) ptr; //set the file name to message
  isbadthread = stat(message, &filestatthread); //call stat
  if (isbadthread == -1){ //if this is a bad file
    badfilethread++; //increment bad file
  } else if(S_ISREG(filestatthread.st_mode)){ //if this is a regular file
    regfilesthread++; //increment regular files
    bytecountthread = bytecountthread + filestatthread.st_size; //count its bytes
    if((fdInthread = open(message, O_RDONLY)) < 0) //open the file
      {
	std::cout << "file open error \n"; //if the file cant be read, mark it as unreadable
	filesgoodthread = 1; //file is unreadable
      } else { //if the file is readable
      filesgoodthread = 0; //default filesgood to "readable"
      while(((cntthread = read(fdInthread, charcheckthread, 1)) > 0) && (filesgoodthread == 0)){ //read through the file
	if((isprint(charcheckthread[0]) == 0) && (isspace(charcheckthread[0]) == 0)) //if a character isnt text....
	{
	  filesgoodthread = 1; //...mark the file as unreadable
	}
    }
    }
    if(filesgoodthread == 0) { //if the file is a text file
      textfilesthread++; //increment text file count
      bytestextthread = bytestextthread + filestatthread.st_size; //add the bytes to the text file bytes count
      close(fdInthread); //close the read
    }
  } else if(S_ISDIR(filestatthread.st_mode)){ //if its a directory
    directthread++; //increment the directory count
  } else { //if its a special file
    specfilesthread++; //increment the special file count
  }
  threadlock.lock(); //try the lock to get access to modify the total counts, and then add the threads totals to the overall totals
  badfile = badfile + badfilethread;
  direct = direct + directthread;
  regfiles = regfiles + regfilesthread;
  specfiles = specfiles + specfilesthread;
  bytecount = bytecount + bytecountthread;
  textfiles = textfiles + textfilesthread;
  bytestext = bytestext + bytestextthread;
  threadlock.unlock(); //unlock the total lock
} //end thread

std::string threadstring ("thread"); //thread routine initializer, just in case

//main takes in (optionally) the word thread, and a number between 1 and 15 for the number of threads, and then prints out stats on all the files in the current directory using either multithreading or a serial running mode
int main(int argc, const char* argv[]) {
  //initialize globals
  badfile = 0;
  direct = 0;
  regfiles = 0;
  specfiles = 0;
  bytecount = 0;
  textfiles = 0;
  bytestext = 0;
  usethreads = 0;
  threadcount = 0;

  if (argc == 3 ) { //if there are arguements on the commandline
    if (threadstring.compare(argv[1]) == 0) { //if the word on the command line is thread
      usethreads = 1; //set threadmode to yes
      totalthreads = atoi(argv[2]); //set the requested number of threads
      if ((totalthreads < 1) || (totalthreads > 15)){ //check to make sure thread count is valid, and terminate if it is not
	std::cout << "bad thread count. \n";
	exit(1);
      }
      //initalize the thread filename arrays
      filename1 = new char[256];
      filename2 = new char[256];
      filename3 = new char[256];
      filename4 = new char[256];
      filename5 = new char[256];
      filename6 = new char[256];
      filename7 = new char[256];
      filename8 = new char[256];
      filename9 = new char[256];
      filename10 = new char[256];
      filename11 = new char[256];
      filename12 = new char[256];
      filename13 = new char[256];
      filename14 = new char[256];
      filename15 = new char[256];
      //link all the thread filename arrays to their corrisponding pointer in ptr
      ptr[1] = &filename1;
      ptr[2] = &filename2;
      ptr[3] = &filename3;
      ptr[4] = &filename4;
      ptr[5] = &filename5;
      ptr[6] = &filename6;
      ptr[7] = &filename7;
      ptr[8] = &filename8;
      ptr[9] = &filename9;
      ptr[10] = &filename10;
      ptr[11] = &filename11;
      ptr[12] = &filename12;
      ptr[13] = &filename13;
      ptr[14] = &filename14;
      ptr[15] = &filename15;
    }
  }
  if (usethreads == 1) { //if threadmode has been requested
    std::cout << "Thread Mode \n"; //inform the user
    while(threadcount < totalthreads){ //while there are still threads left to run for the first time
      if(threadcount < totalthreads){ //double check above condition
	if(!(std::cin.getline(filename, 256))){ //read in a file name, if there is an error break out of while
	  break; //break out
	} else { //if the read is good
	  strcpy(*ptr[threadcount + 1], filename); //copy the filename to the filename for the next thread to run        
	  pthread_create(&threads[threadcount], NULL, threadroutine, (void*) *ptr[threadcount + 1]); //run next thread
          threadcount++; //increment thread count
	}
      }
    } //all threads have been intitalized
    while (!(std::cin.eof())){ //while there are still more files to check
      threadcount = 0; //reset thread count
      for(threadcount = 0; threadcount <  totalthreads; threadcount++) { //for each of the threads possible to run...
	pthread_join(threads[threadcount], NULL); //..wait for the next queued thread to finish
	if(threadcount < totalthreads){ //check to make sure not going over thread limit
	  if(!(std::cin.getline(filename, 256))){ //try to read in the next filename, and break out if there is an error
	    break; //break out of while loop
	  } else { //if there is a file name...
	    strcpy(*ptr[threadcount + 1], filename); //...read it into the recently joined thread's filename...
	    pthread_create(&threads[threadcount], NULL, threadroutine, (void*) *ptr[threadcount + 1]); //...and restart that thread
	  }
	}
      }
    }//when all files have been processed or an EOF has been issued
    threadcount = 0; //reset thread count
    for(threadcount = 0; threadcount < 15; threadcount++){ //make sure all threads have finished
      pthread_join(threads[threadcount], NULL); //wait for all threads
    }//print out final results
  std::cout << "bad files: " << badfile << "\n";
  std::cout << "directories: " << direct << "\n";
  std::cout << "regular files: " << regfiles << "\n";
  std::cout << "special files: " << specfiles << "\n";
  std::cout << "regular byte count: " << bytecount << "\n";
  std::cout << "text files: " << textfiles << "\n";
  std::cout << "text file count: " << bytestext << "\n";
  } else { //if the program should run in serial mode
    while( std::cin.getline(filename, 256)){ //use a while to keep reading in filenames until an error occurs
      isbad = 0; //set isbad to default
      isbad = stat(filename, &filestat); //call stat
    if (isbad == -1){ //if this file is bad
      badfile++; //add this bad file to the count
    } else if(S_ISREG(filestat.st_mode)){ //if file is regular
          regfiles++; //add this file to the count
          bytecount = bytecount + filestat.st_size; //add the bytes to the bytecount
	  if((fdIn = open(filename, O_RDONLY)) < 0) //try to open the file, and if it cant be opened inform the user and move on to the next file
	    {
	      std::cout << "file open error \n";
	      continue; //move on to the next file via the while loop
	    }
          filesgood = 0; //set files text to a default of yes
	  while(((cnt = read(fdIn, charcheck, 1)) > 0) && (filesgood == 0)) { //read through the file
	    if((isprint(charcheck[0]) == 0) && (isspace(charcheck[0]) == 0)) //if the character is not readable
	      {
		filesgood = 1; //mark the file as not text
	      }
	  }
	  if(filesgood == 0) { //if the file is text
            textfiles++; //increment the count
            bytestext = bytestext + filestat.st_size; //add its bytes to the textfile byte count
	  }
	  close(fdIn); //close the file
    } else if(S_ISDIR(filestat.st_mode)){ //if file is a directory
      direct++; //increment the directory count
    } else { //if the file isnt any of the above, it must be special
      specfiles++; //add this file to the count
    }
    }//end while loop
    if(std::cin.eof()){ //if cin got an eof, do nothing, but if it did not, inform the user of a read error
  } else {
    std::cout << "READ ERROR!!! \n";
    } //print out the results
  std::cout << "bad files: " << badfile << "\n";
  std::cout << "directories: " << direct << "\n";
  std::cout << "regular files: " << regfiles << "\n";
  std::cout << "special files: " << specfiles << "\n";
  std::cout << "regular byte count: " << bytecount << "\n";
  std::cout << "text files: " << textfiles << "\n";
  std::cout << "text file count: " << bytestext << "\n";
}
}//end program

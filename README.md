JSonAction.c
This program contains two functions.

1. addAction (string) returning error
   This function accepts a json serialized or indiviual pair string of the form below and maintains an average time
   for each action. 

Sample inputs:

{"action":"jump", "time":100}
{"action":"run", "time":75}
{"action":"jump", "time":200}


2. getStats () returning statistics as a string
getStats requires no input and returns a serialized json array of the average
time for each action that has been provided to the addAction function.

Output after the 3 sample calls from AddAction would be:
  [
    {"action":"jump", "avg":150},
    {"action":"run", "avg":75}
  ]

This program can be called concurrently.  In order to maintain ACIDitiy (Atomicity, Consistency, Isolation, and Durability),
it maintains three files on disk as follows.  This program can be enhanced to utilize other methods for durability of data,
such as SQLLite or MySQL databases.

a) action_hist.txt:  A history log of all calls to AddAction function.  It is used to verify getStats results are correct which are stored in the action_stats.txt file.

Sample contents:
jump,100
run,75
jump,200

b) action_stats.txt: A file containing running average tally of unique actions and their average timing.

   Each record in the file has comma delimited fields in the follwoing order.
     action: Name of action, such as jump, run
     timing: Total timing of the action
     num_times: Number of times an action has been made
     avg_timing: The average timing of the action

Sample contents:

jump,300,2,150
run,75,1,75


c) action_stats_tag.txt: A temporary tag file created while action_stats.txt is getting updated.
   Other concurrent calls to AddAction wait for a few seconds until the tag file is deleted by
  the updating instance of the program.

Initally, when action_hist.txt and action_stats.txt do not exist, they are created by this program automatically.
action_stats_tag.txt is automatically deleted when AddAction is done updating the action_stats.txt file with averaging data.

Example calls to the program:
a) JSonAction addAction {"action":"jump", "time":100}
b) JSonAction addAction {"action":"jump", "time":100} {"action":"run", "time":75} {"action":"jump", "time":200}
c) JSonAction getStats

========================
Error Handling
========================
Some rebost error handling and error checking needs to be added, especially for invalid input.
Currently the program handles basic error checking as follows:

Test1:
C:>JSonAction addAction {"action":"jump",
Second parameter is missing.
Please check and resubmit.

Test2:
C:>JSonAction addAction {"action":"jump", "time"
Second parameter is missing.
Please check and resubmit.

Test3: 
When tag file which disables multip writes to the same action_stats.txt is in use.  
Created the tag file action_stats_tag.txt manually and run the program
C:>JSonAction addAction {"action":"jump", "time":100}
Waiting for Tag file action_stats_tag.txt to release...
Waiting for Tag file action_stats_tag.txt to release...
Waiting for Tag file action_stats_tag.txt to release...
Waiting for Tag file action_stats_tag.txt to release...
Waiting for Tag file action_stats_tag.txt to release...
Tag file action_stats_tag.txt on hold for too long.
Please check and resubmit.

Test4:
Created the tag file action_stats_tag.txt manually and ran the program. While the program was waiting deleted tag file manually.
JSonAction addAction {"action":"jump", "time":100}
Waiting for Tag file action_stats_tag.txt to release...
Waiting for Tag file action_stats_tag.txt to release...
Waiting for Tag file action_stats_tag.txt to release...
Program then completed.

This program uses standard C language libraries and does not require any specialized libraries.

========================
How to compile the code:
========================

The program compiles with most C compiler on Unix/Linux, and Windows.

Examples:
   a) Windows using Microsoft Visual Studio Developer Command Line Prompt:
      C:>cl JSonAction.c
      Microsoft (R) C/C++ Optimizing Compiler Version 19.28.29336 for x86
      Copyright (C) Microsoft Corporation.  All rights reserved.
   
      JSonAction.c
      Microsoft (R) Incremental Linker Version 14.28.29336.0
      Copyright (C) Microsoft Corporation.  All rights reserved.

      /out:JSonAction.exe
      JSonAction.obj

      C:>JSonAction getStats

   b) Linux
      gcc -o JSonAction JSonAction.c
      chmod a+x JSonAction
      ./JSonAction getStats

Libraries used:
stdio.h
stdlib.h
string.h
time.h


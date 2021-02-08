/*
 * JSonAction.c
 
   Program Name:  JSonAction.c
   Create Date:   02/01/2021
   Author:        Kourosh Ashkon
   Description:   This program contains two functions.

1. addAction (string) returning error
   This function accepts a json serialized or individual pair string of the form below and maintains an average time
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

   Each record in the file has comma delimited fields in the following order.
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

Initially, when action_hist.txt and action_stats.txt do not exist, they are created by this program automatically.
action_stats_tag.txt is automatically deleted when AddAction is done updating the action_stats.txt file with averaging data.

                    :::::::::::::::::::::::::::::::::::::::::::::
                        M O D I F I C A T I O N  H I S T O R Y
                    :::::::::::::::::::::::::::::::::::::::::::::
   ---------- ------------- ---------------------------------------------
    DATE        NAME          DESCRIPTION
   ---------- ------------- --------------------------------------------
   02/01/2021 KAshkon       Initial version
   ===================================================================================================

Example calls to the program:
a) JSonAction addAction {"action":"jump", "time":100}
b) JSonAction addAction {"action":"jump", "time":100} {"action":"run", "time":75} {"action":"jump", "time":200}
c) JSonAction getStats

========================
Error Handling
========================
Some robust error handling and error checking needs to be added, especially for invalid input.
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
When tag file which disables multiple writes to the same action_stats.txt is in use.  
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

Test5:
C:\repos\JSonAction>JSonAction addAction {"action":"jump", "time":100}
C:\repos\JSonAction>JSonAction addAction {"action":"run", "time":75}
C:\repos\JSonAction>JSonAction addAction {"action":"jump", "time":200}
C:\repos\JSonAction>JSonAction getStats
        [
                {"action":"jump", "avg":150}
                {"action":"run", "avg":75}
        ]

C:\repos\JSonAction>JSonAction addAction {"action":"jump", "time":100} {"action":"run", "time":75} {"action":"jump", "time":200}
C:\repos\JSonAction>JSonAction getStats
        [
                {"action":"jump", "avg":150}
                {"action":"run", "avg":75}
        ]

C:\repos\JSonAction>JSonAction addAction {"action":"jump", "time":400}
C:\repos\JSonAction>JSonAction getStats
        [
                {"action":"jump", "avg":200}
                {"action":"run", "avg":75}
        ]

C:\repos\JSonAction>


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


 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define  MAX_LINE_SIZE   1024

char  action_history_file [MAX_LINE_SIZE];   /* actions history variable file name */
char  action_stats_file [MAX_LINE_SIZE];     /* actions statistics variable file name */
char  action_stats_file_tag [MAX_LINE_SIZE]; /* Temporary tag variable file name */

short exit_code;

/* The following structure stores the Jason input pair of action and its timing */
struct action_struct_rec
{
    char action[25];
    int timing;
    struct action_struct_rec *next;
};

/* Linked list structure pointers for storing actinos and their timings */
struct action_struct_rec *action_list;
struct action_struct_rec *action_top_list;
struct action_struct_rec *action_prev_list;


/* The following structure stores the unique action, its total timing, number of times it has been called, and its running average tally. */
struct stats_struct_rec
{
    char action[25];
    int timing;
    int num_times;
    int avg_timing;
    struct stats_struct_rec *next;
};

/* Linked list structure pointers for storing action, and their averages */
struct stats_struct_rec *stats_list;
struct stats_struct_rec  *stats_top_list;
struct stats_struct_rec  *stats_prev_list;

/* A function to make the program wait for a speicfied number of seconds */
void sleep (int number_of_seconds) 
{ 
    // Converting time into milli_seconds 
    int milli_seconds = 1000 * number_of_seconds; 
  
    // Storing start time 
    clock_t start_time = clock(); 
  
    // looping till required time is not achieved 
    while (clock() < start_time + milli_seconds) 
        ; 
} 
  

/* The following remvoes any extra leading white space */
/* ---------------------------------------------------- */
char *remove_leading_white_space (s)
char   *s;
{
  while (*s == ' ' ||
         *s == '\t')
       s++;
  return (s);
}


/* The following remvoes any extra trailing white space */
/* ---------------------------------------------------- */
char *remove_trailing_white_space (s)
char   *s;
{
  short    s_len, i;

  s_len = strlen (s);

  if (s_len == 0)
     return NULL;

  for (i = s_len - 1; i >= 0; i--) {
      if (s[i] == ' ' ||
          s[i] == '\t' ||
          s[i] == '\n')
          s[i] = '\0';
      else
          break;
  }
  return (s);
}


/* The following function returns a requested field number from a record string.
 * The record needs to be delimited by comma.
 * It is used to parse statistical fields from the action_stats.txt file.
 */

/****************************************/
char *get_field_num_in_line (s, field_num)
char      *s;
short     field_num;
{
  short          i, j;
  static  char   field[80];
  static short   number_of_chars_traversed;
  short     field_len;
  short   curr_field_num;


  curr_field_num = 1;
  while (curr_field_num != field_num ) {
     while (*s != ',' && *s != '\0' && *s != '\n')
        s++;

     if (*s == ',') {
        curr_field_num++;
        s++;
     }
  }

  while (*s == ' ' || *s == '\t')
     s++;

  memset (field, 0, sizeof (field));
  j = 0;
  while (*s != ' ' &&
     *s != 44 &&
     *s != '\t' &&
     *s != '\n' &&
     *s != '\0')
     field [j++] = *s++;
 
  field [j++] = '\0';
  field_len = strlen (field);

  number_of_chars_traversed += field_len;
  number_of_chars_traversed++;

  return (field);
}


/* The following function checks whether a file exists on disk */
short file_exists (char * filename)
{
    FILE *fp;
    if (fp= fopen(filename, "r")){
        fclose(fp);
        return 1;
    }
    return 0;
}


/* The following function parses an Input jSon pair of acting and its timing 
 * and returns a structure point of action_struct_rec type.
 */

struct action_struct_rec *parseAddAction (char *s) {
  char  *sp;
  struct action_struct_rec action_rec;

  char v_action_str_caption[25];
  char v_action_str[25];
  char v_timing_str_caption[25];
  char v_timing_str[25];
  int i;

   memset (v_action_str_caption, 0, sizeof(v_action_str_caption));
   memset (v_action_str, 0, sizeof(v_action_str));
   memset (v_timing_str_caption, 0, sizeof(v_timing_str_caption));
   memset (v_timing_str, 0, sizeof(v_timing_str));


   if (s == NULL)
        return NULL;

   if ((int) strlen (s) == 0)
        return NULL;

   sp = s;

   while (*sp == ' ' || *sp == '\t' || *sp == '{' || *sp == 34 || *sp == 39)
        sp++;

   i = 0;
   while (*sp != ' ' && *sp != '\t' && *sp != '{' && *sp != 34  && *sp != 39 && *sp != ':' && *sp != ',' && i < 25)
        v_action_str_caption[i++] = tolower ((char) *sp++);


   /* skip */
   while (*sp == ' ' || *sp == '\t' || *sp == '{' || *sp == 34  || *sp == 39 || *sp == ':')
        sp++;

   i = 0;
   while (*sp != ' ' && *sp != '\t' && *sp != '{' && *sp != 34  && *sp != 39 && *sp != ':' && *sp != ',' && i < 25)
        v_action_str[i++] = tolower ((char) *sp++);


   /* skip */
   while (*sp == ' ' || *sp == '\t' || *sp == '{' || *sp == 34 || *sp == 39 || *sp == ':' || *sp == ',')
        sp++;

   i = 0;
   while (*sp != ' ' && *sp != '\t' && *sp != '{' && *sp != 34 && *sp != 39 && *sp != ':' && *sp != ',' && i < 25)
        v_timing_str_caption[i++] = tolower ((char) *sp++);

   if (strlen (v_timing_str_caption) == 0) {
     printf("Second parameter is missing.\n");
     printf("Please check and resubmit.\n");
     exit_code=1;
     return NULL;
   }

   /* skip */
   while (*sp == ' ' || *sp == '\t' || *sp == '{' || *sp == 34 || *sp == 39 || *sp == ':')
        sp++;

   i = 0;
   while (*sp != ' ' && *sp != '\t' && *sp != '}' && *sp != 34 && *sp != 39 && *sp != ':' && *sp != ',' && i < 25) {
	if (isdigit ((char) *sp)) { 
           v_timing_str[i] = tolower ((char) *sp);
	}
        else {
        }
	i++;
	sp++;
   }

   if (strlen (v_timing_str) == 0) {
     printf("Second parameter is missing.\n");
     printf("Please check and resubmit.\n");
     exit_code=1;
     return NULL;
   }

   action_list = (struct action_struct_rec *) calloc ((sizeof (struct action_struct_rec)), (sizeof (struct action_struct_rec)));
 
   if (action_top_list == NULL) {
         action_top_list = action_list;
   }
   else {
         action_prev_list->next = action_list;
   }

   action_list->next = NULL;
   action_prev_list = action_list;

   strncpy (action_list->action, v_action_str, 25);
   action_list->timing = atoi(v_timing_str);

   return (action_list);

}

/* This function coverts(ensures) all characters of a string to (are) lower case
 * It is useful if input data has mixed inconsistent cases
 */

char *convert_to_lower_case (char *s)
{
   int i, str_len;
   char  *str;
   str = s;

   str_len = strlen(str);

   for (i=0; i<=str_len; i++) {

      if(str[i]>=65 && str[i]<=90)
         str[i]=str[i]+32;
   }

   return (str);

}

/* The following function add a jSon actino and timing pair to the 
 * action_hist.txt actions history file.
 * It also updates and keeps an average running timing tally of timing for each action,
 * which is stored in the action_stats.txt file.
 */

void addAction(struct action_struct_rec *action_list)
{
    FILE* fp;
    FILE* fp2;
    int record_found=0;
    int loop_count;
    char  in_line [MAX_LINE_SIZE];
    short  found = 0;

    struct action_struct_rec temp_action_rec;

    fp = fopen(action_history_file, "a");

    if (fp == NULL)
        printf("Error opening action_history_File %s\n!!!", action_history_file);
    else
    {
      fprintf(fp, "%s,%d\n", action_list->action, action_list->timing);
      fclose(fp);
    }

    record_found=0;
    if (file_exists(action_stats_file) == 1) {
	loop_count=0;
        while (file_exists(action_stats_file_tag) == 1) {
           printf("Waiting for Tag file %s to release...\n", action_stats_file_tag);
	   sleep (5); /* Sleep for 5 seconds */
	   if (loop_count == 4) {
              printf("Tag file %s on hold for too long.\n", action_stats_file_tag);
	      printf("Please check and resubmit.\n");
	      exit_code=1;
	      return;
           }
	   loop_count++;
        }

        fp = fopen(action_stats_file, "r");
        if (fp == NULL) {
            fp = fopen(action_stats_file, "a");
            if (fp == NULL)
                printf("Error opening action_stats_file %s\n for write.", action_stats_file);
            else
            {
                fp2 = fopen(action_stats_file_tag, "w");
                if (fp2 != NULL) {
                   fprintf(fp2, "Updating\n");
                   fclose(fp2);
                }

                fprintf(fp, "%s,%d,%d,%d\n", action_list->action, 
                                           action_list->timing,
                                           1,
                                           action_list->timing);
                fclose(fp);
                unlink (action_stats_file_tag);
            }
        }
        else
        {
            fp2 = fopen(action_stats_file_tag, "w");
            if (fp2 == NULL)
                printf("Error opening tag file %s\n!!!", action_stats_file_tag);
	    else {
               fprintf(fp2, "Updating\n");
               fclose(fp2);
            }

            stats_top_list = NULL;
            stats_prev_list = NULL;
            while (fgets (in_line, MAX_LINE_SIZE, fp) != NULL) {
              strcpy(in_line, remove_trailing_white_space (in_line));
              strcpy(in_line, remove_leading_white_space (in_line));

              stats_list = (struct stats_struct_rec *) calloc ((sizeof (struct stats_struct_rec)), (sizeof (struct stats_struct_rec)));
 
              if (stats_top_list == NULL)
                 stats_top_list = stats_list;
              else
                 stats_prev_list->next = stats_list;

              strcpy (stats_list->action, get_field_num_in_line (in_line, 1));
              stats_list->timing =  atoi (get_field_num_in_line (in_line, 2));
              stats_list->num_times = atoi (get_field_num_in_line (in_line, 3));
              stats_list->avg_timing = atoi (get_field_num_in_line (in_line, 4));
              stats_list->next = NULL;
              stats_prev_list = stats_list;

            }
            fclose(fp);


            stats_list = stats_top_list;
	    found = 0;
            while (stats_list != NULL && found == 0 ) {
               if (!strcmp (stats_list->action, action_list->action)) {
                      stats_list->timing += action_list->timing; 
                      stats_list->num_times++;
                      stats_list->avg_timing =  stats_list->timing / stats_list->num_times;
	              found = 1;
               }
               stats_list = stats_list->next;
            }

            fp = fopen(action_stats_file, "w");
            if (fp == NULL)
                printf("Error opening action_stats_file %s\n!!!", action_stats_file);
            else
            {
               stats_list = stats_top_list;
               while (stats_list != NULL) {
                  fprintf(fp, "%s,%d,%d,%d\n", stats_list->action, 
                                                stats_list->timing,
                                                stats_list->num_times,
                                                stats_list->avg_timing);
                  stats_list = stats_list->next;
               }
               if (found == 0) {
                  fprintf(fp, "%s,%d,%d,%d\n", action_list->action, 
                                                action_list->timing,
                                                1,
                                                action_list->timing);
               }
               fclose(fp);
               unlink (action_stats_file_tag);
            }
        }
    }
    else {
        fp = fopen(action_stats_file, "a");
        if (fp == NULL)
            printf("Error opening action_stats_file %s\n!!!", action_stats_file);
        else
        {
            fp2 = fopen(action_stats_file_tag, "a");
            if (fp2 != NULL) {
               fprintf(fp2, "Updating\n");
               fclose(fp2);
            }

            fprintf(fp, "%s,%d,%d,%d\n", action_list->action, 
                                       action_list->timing,
                                       1,
                                       action_list->timing);
            fclose(fp);
            unlink (action_stats_file_tag);
        }
    }
}

/* This function opens the action_stats.txt file and prints its contents
 * in the following format
 * for example:
  [
    {"action":"jump", "avg":150},
    {"action":"run", "avg":75}
  ]
*/

void getStats ()
{
    FILE* fp;
    char  in_line [MAX_LINE_SIZE];

    struct stats_struct_rec  *stat_list;

    fp = fopen(action_stats_file, "r");
    if (fp == NULL)
        printf("There are no stats.\nEnter some data first.\n");
    else
    {
        printf ("\t[\n");
        while (fgets (in_line, MAX_LINE_SIZE, fp) != NULL) {
          strcpy(in_line, remove_trailing_white_space (in_line));
          strcpy(in_line, remove_leading_white_space (in_line));

          printf ("\t\t{%caction%c:%c%s%c, %cavg%c:%d}\n", 34, 34, 34, 
			 get_field_num_in_line (in_line, 1),34, 34, 34,
                         atoi (get_field_num_in_line (in_line, 4)));
        }
        printf ("\t]\n");
        fclose(fp);
    }
}

/* This is the main function of the program.
 * It only accepts two function calls of addAction and getStats.
 * If an unrecognized function name is specified, it prints an error message.
 */
 
short main(int argc, char* argv[])
{
    int choice;
    short     i,j;
    char      a_string[100];
    char      v_jason_pair[100];
    struct action_struct_rec *action_list = NULL;

    memset (action_history_file, 0, sizeof(action_history_file));
    memset (action_stats_file, 0, sizeof(action_stats_file));
    memset (action_stats_file_tag, 0, sizeof(action_stats_file_tag));

    strcpy (action_history_file, "action_hist.txt");
    strcpy (action_stats_file,  "action_stats.txt");
    strcpy (action_stats_file_tag,  "action_stats_tag.txt");

    exit_code = 0;

    action_list = NULL;
    action_top_list = NULL;
    action_prev_list = NULL;
    stats_list = NULL;
    stats_top_list = NULL;
    stats_prev_list = NULL;

    for (i = 1; i < argc; i++) {

	if (i == 1) {
           if (strcmp (convert_to_lower_case (argv[i]), "addaction") &&
               strcmp (convert_to_lower_case (argv[i]), "getstats")) {
              printf("Missing operation addAction or getStats.\n");
	      exit (1);
           }
        }

        if (!strcmp (convert_to_lower_case (argv[i]), "addaction")) {
           for (j = i+1; j <argc; j++) {
              memset (v_jason_pair, 0, sizeof(v_jason_pair));
 	      sprintf(v_jason_pair, "%s%s", argv[j], argv[j+1]);
              action_list = parseAddAction (v_jason_pair);
              addAction (action_list);
	      if ( exit_code != 0)
                 return (exit_code);
	      j++;
	      i=j;
	   }
        }
        if (!strcmp (convert_to_lower_case (argv[i]), "getstats")) {
           memset (v_jason_pair, 0, sizeof(v_jason_pair));
           getStats (stats_top_list);
        }
    }
}


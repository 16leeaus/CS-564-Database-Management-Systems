/*******************************************************
* Name:       Austin Lee
* Student ID: 9073532831
* Wisc ID:    alee88@wisc.edu
* File:       wl.cpp
********************************************************
*
* This file implements the word locator program and uses the class data 
* strucutre that is implemented in "wl.h".
*
*/ //***************************************************

#include "wl.h"

/**
 * @brief Main function of the word locator program. Handles all of the 
 * individual commands that a user will enter.
 * 
 * @param argc Number of command line arguments inputted by user.
 * @param argv Elements of the user inputted command line arguments.
 * @return Returns 0 as a way to terminate the program.
 */
int main(int argc, char** argv) {
  char command[514];
  char* inputCommand = command, *token = command;
  FILE* fin = stdin, *loadFile = NULL;
  char* cmpVal = NULL;
  command_enum userCommand = ERROR_enum;
  genDelimiters();
  while (1) {
    userCommand = ERROR_enum;
    wl_printf(">");
    cmpVal = fgets(command, sizeof(command), fin);
    if (cmpVal == NULL) {
      continue;
    }

    char* context;
    char* argvs[4];
    int i = 0;
    inputCommand = command;
    cmpVal = strchr(inputCommand, '\n');
    if (cmpVal != NULL) *cmpVal = '\0';

    while ((token = strtok_r(inputCommand, " ", &context)) != NULL) {
      inputCommand = NULL;
      argvs[i++] = token;
      if (i > 3) break;
    }

    if (i == 1) {
      if (0 == strcasecmp(argvs[0], "new")) userCommand = NEW_enum;
      else if (0 == strcasecmp(argvs[0], "end")) userCommand = END_enum;
    } 
    
    else if (i == 2) {
      if (0 == strcasecmp(argvs[0], "load")) userCommand = LOAD_enum;
    } 
    
    else if (i == 3) {
      if (0 == strcasecmp(argvs[0], "locate")) userCommand = LOCATE_enum;
    } 
    
    else {
      wl_printf("ERROR: Invalid command\n");
      continue;
    }

    switch (userCommand) {
      /**
       * @brief Command entered "load", will open a new text file, and will 
       * insert every new word into the data strucutre (BST).
       */
      case LOAD_enum: {
        loadFile = fopen(argvs[1], "r");
        if (loadFile == NULL) {
          wl_printf("ERROR: Invalid command\n");
          continue;
        }
        if (root != NULL) {
          delete root;
          root = NULL;
        }
        char line[100];
        int index = 1;
        while (fgets(line, sizeof(line), loadFile) != NULL) {
          inputCommand = line;
          while (NULL != (token = strtok_r(inputCommand, delimiter.c_str(), \
            &context))) {
            inputCommand = NULL;
            std::string temp(token);
            root = insert(root, temp.c_str(), index);
            index++;
          }
        }
        fclose(loadFile);
      }
        break;

      /**
       * @brief Command to locate a entry in the binary search tree. If the 
       * desired word does not exist an error will be thrown. Otherwise the 
       * function checks to see if the word is valid, and will then use the data 
       * strucutres "lookup" function.
       */
      case LOCATE_enum: {
        char* err = NULL;
        int placement = strtol(argvs[2], &err, 10);
        if (*err != '\0') {
          wl_printf("ERROR: Invalid command\n");
          continue;
        }

        if (placement < 0) {
          wl_printf("ERROR: Invalid command\n");
          continue;
        }

        if (false == validCheck(argvs[1])) {
          wl_printf("ERROR: Invalid command\n");
          continue;
        }

        std::string temp(argvs[1]);
        node* cmpVal = lookup(root, temp.c_str());
        
        if (cmpVal == NULL) {
          wl_printf("No matching entry\n");
        } 
        
        else if (cmpVal -> index -> getSize() < (unsigned int)placement) {
          wl_printf("No matching entry\n");
        } 
        
        else {
          wl_printf("%d\n", (*(cmpVal -> index))[placement - 1]);
        }
      }
        break;

      /**
       * @brief Command to clear the data structure. Will delete the root node
       * and set it to NULL, before the break command.
       */
      case NEW_enum:
        if (root != NULL) {
          delete root;
          root = NULL;
        }
        break;

      /**
       * @brief Command to terminate the program. Will delete and set the root 
       * node to NUll before termination.
       */
      case END_enum:
        if (root != NULL) {
          delete root;
          root = NULL;
        }
        return 0;
        break;

      /**
       * @brief Default case of inputted command. If the user does not enter a 
       * valid command, this error is printed out.
       */
      default:
        wl_printf("ERROR: Invalid command\n");
        continue;
    }
  }
  return 0;
}

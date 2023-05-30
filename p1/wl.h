/*******************************************************
* Name:       Austin Lee
* Student ID: 9073532831
* Wisc ID:    alee88@wisc.edu
* File:       wl.h
********************************************************
*
* This file implements the node class for the data structure as well as
* the necessary functions such as lookup and insert. Which will hanlde loading 
* and fetching data from the binary search tree.
*
*/ //***************************************************

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <string>

/**
 * @brief Definition of a helper function to print to stdout using system call.
 */
#define wl_printf(format, ...) \
  sprintf(statement, format, ##__VA_ARGS__); \
  if (write(STDOUT_FILENO, statement, strlen(statement)) == -1) \
    perror("Error writing to STDOUT\n");

/**
 * @brief Enumeration commands, used to determine the user entered command and 
 * control a switch in the main function.
 */
typedef enum _command {
  LOAD_enum,    // Load command to open a new txt file, and insert into BST.
  LOCATE_enum,  // Locate command to find a specific word in the BST.
  NEW_enum,     // New command, which will reset the data structure.
  END_enum,     // End command to terminate the program on deamand.
  ERROR_enum    // Error enumerator,   
} command_enum;

std::string delimiter;
char statement[1000] = {0};

/**
 * @brief Function used to generate a set of delimiters used to tokenize the 
 * words from the inputted text file.
 */
void genDelimiters() {
  for (int i = 1; i < 256; i++) {
    if (i >= 48 && i <= 57) {
        continue;   // Cannot end word with [0-9].
    }
    
    if (i >= 65 && i <= 90) {
        continue;   // Cannot end word with [A-Z].
    }
    
    if (i >= 97 && i <= 122) {
        continue;   // Cannot end word with [a-z].
    }
    
    if (i == 39) {
        continue;   // Cannot end word with ['] (Apostrophe).
    }
    delimiter += (char)i;
  }
}

/**
 * @brief Function used to check the validity of the input command arguments, 
 * checks for correct characters.
 * 
 * @param word Word the function will check the validity of, in order to search.
 * @return true This command is considered to be valid, continue.
 * @return false This command is considered to be invalid. Throw an error.
 */
bool validCheck(const char* word) {
  for (; *word != '\0'; word++) {
    int i = *word;
    if (i >= 48 && i <= 57) {
        continue;                   // [0-9] are all valid characters.
    }      
    else if (i >= 65 && i <= 90) {
        continue;                   // [A-Z] are all valid characters.
    }  
    else if (i >= 97 && i <= 122) {
        continue;                   // [a-z] are all valid characters.
    }  
    else if (i == 39) {
        continue;                   // ['] Apostrophe is a valid character.
    }            
    else if (i == 0) {
        continue;
    }
    else {
        return false;
    }
  }
  return true;
}

/**
 * @brief Class used to implement vector like functionality for the storage of 
 * data related to words from the text document.
 */
class vectorObj {
 public:
    int* indexArray;    // Variable used to store indices in an array.
    size_t capacity;    // Variable used to store allocated memory.
    size_t size;        // Variable used to store word associated data in tree.

    /**
     * @brief Constructor for the vector object, which will set all pointers and 
     * variables to NULL or 0 by default.
     */
    vectorObj():indexArray(NULL), capacity(0), size(0) {}

    /**
     * @brief Function used to return the size of the data strucutre.
     * @return size_t Size of the data strucutre (binary search tree).
     */
    size_t getSize() {
      return size;
    }

    /**
     * @brief Function used to push a node to the back of the data structure. 
     * 
     * @param index Index of the data on the node to be pushed on to the data 
     * strucutre.
     * @return Nothing is returned by this function.
     */
    void push(int index) {

      if (size + 1 > capacity) {

        if (capacity == 0) {
          capacity = 2;
          indexArray = (int*)malloc(capacity * sizeof(int));
          indexArray[size++] = index;
        } 
        
        else {
          capacity *= 2;
          indexArray = (int*)realloc(indexArray, capacity * sizeof(int));
          indexArray[size++] = index;
        }

      } 
      
      else {
        indexArray[size++] = index;
      }

    }

    /**
     * @brief Overloaded operator that provides array type access to 
     * calling data.
     * 
     * @param index Index of the desired data, which is to be accessed.
     * @return int Returns the data stored at the specified index.
     */
    int operator[] (int index) {
      return indexArray[index];
    }

    /**
     * @brief Destructor used to free up memory allocation.
     */
    ~vectorObj() {
      if (indexArray != NULL) free(indexArray);
    }
};

/**
 * @brief The class used to represent a node in the binary search tree.
 */
class node {
 public:
    char* word;         // Pointer for the word that is stored in this node.
    node* left;         // Pointer to the left subtree of the current node.
    node* right;        // Pointer to the left subtree of the current node.
    vectorObj* index;   // 

    /**
     * @brief Constructor for the node object, sets all member pointers to NULL
     * by default.
     */
    node():word(NULL), left(NULL), right(NULL), index(NULL) {}

    /**
     * @brief Node constructor, which will take in one argument, the word from 
     * the text document that is to be added.
     * 
     * @param word Word from the text document, that is to be inserted into 
     * the node.
     */
    node(char* word): word(word), left(NULL), right(NULL), index(new \
    vectorObj()) {}

    /**
     * @brief Destructor used to free up memory allocation.
     */
    ~node() {
      if (word != NULL) {
        free(word);         // Free up the memory allocation.
      }

      if (left != NULL) {
        delete left;        // Delete the root node's left subtree.
      }

      if (right != NULL) {
        delete right;       // Delete the root node's right subtree.
      }

      if (index != NULL) {
        delete index;       // Delete the index information stored in the node.
      }

    }

} *root;

/**
 * @brief Function used to insert a new word into the binary search tree. The 
 * word will be added as a new node in the tree.
 * 
 * @param root Current root node of the binary search tree. 
 * @param word Word from the text document which is to be added to the binary 
 * search tree in the form of a node.
 * @param index Index of the word based on its placement in the original text 
 * file.
 * @details  Inserts a new node into the BST. If the root is NULL, the node to 
 * be inserted is the new root. Else the new node is inserted by comparing the 
 * word to be inserted against the word in the current node.
 * @return node* The new root node of the binary search tree.
 */
node* insert(node* root, const char* word, int index) {

  if (root == NULL) {
    root = new node(strdup(word));
    root -> index -> push(index);
    return root;
  }

  int cmpVal = strcasecmp(word, root->word);

  if (cmpVal < 0) {
    node* returnNode = insert(root -> left, word, index);
    if (root -> left == NULL) root -> left = returnNode;
  } 
  
  else if (cmpVal > 0) {
    node* returnNode = insert(root -> right, word, index);
    if (root -> right == NULL) root -> right = returnNode;
  } 
  
  else {
    root -> index -> push(index);
  }
  return root;
}

/**
 * @brief Function used to lookup a word that is in the binary search tree.
 * 
 * @param root The current root node of the binary search tree. The algorithm 
 * will begin by searching for the word here.
 * @param word The word the user wishes to lookup from the text document.
 * @details Compares the desired word, with the word that is stored in the 
 * current node. If it is the current node is returned, if not begin by 
 * searching other nodes. If the node does not exist, NULL is returned.
 * @return node* The node that conains information on the word to be searched. 
 * If the word does not exist in the binary search tree NULL is returned.
 */
node* lookup(node* root, const char* word) {
  if (root == NULL) {
    return NULL;
  } 

  node* returnNode = NULL;
  int cmpVal = strcasecmp(word, root -> word);

  if (cmpVal < 0) {
    returnNode = lookup(root -> left, word);
  }
  else if (cmpVal > 0) {
    returnNode = lookup(root -> right, word);
  }

  else {
    returnNode = root;
  }
    
  return returnNode;
}
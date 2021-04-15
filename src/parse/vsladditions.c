#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define READ_END 0
#define WRITE_END 1

// These are exposed by the compiled vale .obj/.o, they're
// the start of a Vale native API.
typedef struct ValeStr {
  uint64_t length;
  char chars[0];
} ValeStr;

ValeStr* ValeStrNew(int64_t length);
ValeStr* ValeStrFrom(char* source);

#include "ImmStrArray.h"
#include "ExecuteProgramArgs.h"
#include "ExecuteProgramResult.h"
#include "ExecuteProgramResultNew.h"


// typedef struct TestStruct TestStruct;
// extern TestStruct* newTestStruct(int x, ValeStr* s);
// extern void dropTestStruct(TestStruct* testStruct);
// extern int TestStruct_get_x(TestStruct* testStruct);
// extern int TestStruct_get_s(TestStruct* testStruct);

// int doThing() {
//   ValeStr* s = vale_newstr(5);
//   strcpy(vale_getstrchars(s), "hello");
//   TestStruct* ts = newTestStruct(5, s);
//   int x = TestStruct_get_x(ts);
//   dropTestStruct(ts);
//   return x;
// }


typedef struct BufferListNode {
  struct BufferListNode* previous;
  char buffer[1024];
} BufferListNode;

ExecuteProgramResultRef executeProgram(ValeStr* commandVStr) {
  char* command = commandVStr->chars;
  // printf("%s:%d\n", __FILE__, __LINE__);

  // printf("Running program: %s\n", command);

  FILE* fp = popen(command, "r");
  ValeReleaseMessage(commandVStr);
  if (!fp) {
    fprintf(stderr, "popen failed on command: %s\n", command);
    exit(1);
  }

  BufferListNode* head = malloc(sizeof(BufferListNode));
  head->previous = NULL;
  char* charPtrInBuffer = head->buffer;
  BufferListNode* first = head;

  int totalChars = 0;
  int c; // note: int, not char, required to handle EOF
  while ((c = fgetc(fp)) != EOF) {
    if (charPtrInBuffer == &head->buffer[1024]) {
      BufferListNode* newHead = malloc(sizeof(BufferListNode));
      newHead->previous = head;
      head = newHead;
      charPtrInBuffer = head->buffer;
    }
    *charPtrInBuffer = (char)c;
    charPtrInBuffer++;
    totalChars++;
  }

  if (ferror(fp)) {
    fprintf(stderr, "fgetc failed to read from command: %s\n", command);
    exit(1);
  }
  assert(feof(fp));


  ValeStr* vstr = ValeStrNew(totalChars);

  int i = totalChars;
  while (i > 0) {
    if (charPtrInBuffer == &head->buffer[0]) {
      BufferListNode* newHead = head->previous;
      free(head);
      head = newHead;
      charPtrInBuffer = &head->buffer[1024];
    }
    
    charPtrInBuffer--;
    i--;
    vstr->chars[i] = *charPtrInBuffer;
  }

  assert(head == first);
  assert(charPtrInBuffer == &first->buffer[0]);
  free(head);

  int exitCode = pclose(fp);

  return ExecuteProgramResultNew(exitCode, vstr, ValeStrFrom(""));
}


ExecuteProgramResultRef executeProgramV2(ExecuteProgramArgs* programArgs) {
  //printf("%s:%d\n", __FILE__, __LINE__);  printf("%s:%d\n", __FILE__, __LINE__);
  ValeStr* commandVStr = programArgs->program;
  char* command = commandVStr->chars;

  // printf("Running program: %s\n", command);

  int childStdinPipeFDs[2] = { 0, 0 };
  pipe(childStdinPipeFDs);

  int childStdoutPipeFDs[2] = { 0, 0 };
  pipe(childStdoutPipeFDs);
  // printf("%s:%d\n", __FILE__, __LINE__);

  pid_t pid = fork();
  if (pid < 0) {
    perror("Couldn't fork for child process");
    exit(1);
  }

  // printf("%s:%d\n", __FILE__, __LINE__);

  if (pid == 0) {
  // printf("%s:%d\n", __FILE__, __LINE__);
    char** args = malloc(sizeof(char*) * programArgs->args->length);
    for (int i = 0; i < programArgs->args->length; i++) {
      args[i] = programArgs->args->elements[i]->chars;
    }
    args[programArgs->args->length] = (char*)NULL;
  // printf("%s:%d\n", __FILE__, __LINE__);

    if (dup2(childStdoutPipeFDs[WRITE_END], STDOUT_FILENO) < 0) {
      perror("Couldn't dup2 in child process");
      exit(1);
    }
  // printf("%s:%d\n", __FILE__, __LINE__);
    close(childStdoutPipeFDs[READ_END]);
    close(childStdoutPipeFDs[WRITE_END]);

    dup2(childStdinPipeFDs[READ_END], STDIN_FILENO);
    close(childStdinPipeFDs[READ_END]);
    close(childStdinPipeFDs[WRITE_END]);

  // printf("%s:%d\n", __FILE__, __LINE__);
    execvp(programArgs->program->chars, args);
    perror("Failed to execvp in child process");
    fprintf(stderr, "while executing command '%s'\n", command);
    exit(1);
  }

  // printf("writing len: %d\n", programArgs->stdin->length);
  // printf("writing: %s\n", programArgs->stdin->chars);
  if (write(childStdinPipeFDs[WRITE_END], programArgs->stdin->chars, programArgs->stdin->length) < 0) {
    perror("Couldn't write stdin to child process!");
    exit(1);
  }

  if (close(childStdinPipeFDs[WRITE_END]) < 0) {
    perror("Couldn't close pipe to child!");
    exit(1);
  }

  // printf("%s:%d\n", __FILE__, __LINE__);
  ValeReleaseMessage(programArgs);

  // printf("%s:%d\n", __FILE__, __LINE__);
  int exitCode;
  if (waitpid(pid, &exitCode, 0) < 0) {
    perror("Couldn't waitpid");
    exit(1);
  }

  // printf("%s:%d\n", __FILE__, __LINE__);
  // char results[1024] = "";
  // read(childStdoutPipeFDs[READ_END], results, 1024);
  // close(childStdoutPipeFDs[READ_END]);
  // printf("%s\n", results);
  

  BufferListNode* head = malloc(sizeof(BufferListNode));
  head->previous = NULL;
  char* charPtrInBuffer = head->buffer;
  BufferListNode* first = head;

  // printf("%s:%d\n", __FILE__, __LINE__);
  int totalChars = 0;

  fcntl(childStdoutPipeFDs[READ_END], F_SETFL, O_NONBLOCK);
  while (1) {
    char c = 0;
    int result = read(childStdoutPipeFDs[READ_END], &c, 1);
    if (result < 0) {
        if (errno == EAGAIN) {
            break;
        }
        perror("Couldnt read");
        exit(1);
    }
    if (result == 0) {
        break;
    }

  // printf("%s:%d\n", __FILE__, __LINE__);
    if (charPtrInBuffer == &head->buffer[1024]) {
      BufferListNode* newHead = malloc(sizeof(BufferListNode));
      newHead->previous = head;
      head = newHead;
      charPtrInBuffer = head->buffer;
    }
    *charPtrInBuffer = (char)c;
    charPtrInBuffer++;
    totalChars++;
  }


  // printf("%s:%d\n", __FILE__, __LINE__);
  close(childStdoutPipeFDs[READ_END]);


  ValeStr* vstr = ValeStrNew(totalChars);

  // printf("%s:%d\n", __FILE__, __LINE__);
  int i = totalChars;
  while (i > 0) {
    if (charPtrInBuffer == &head->buffer[0]) {
      BufferListNode* newHead = head->previous;
  // printf("%s:%d\n", __FILE__, __LINE__);
      free(head);
      head = newHead;
      charPtrInBuffer = &head->buffer[1024];
    }
    
    charPtrInBuffer--;
    i--;
    vstr->chars[i] = *charPtrInBuffer;
  }

  // printf("%s:%d\n", __FILE__, __LINE__);
  assert(head == first);
  assert(charPtrInBuffer == &first->buffer[0]);
  free(head);

  return ExecuteProgramResultNew(exitCode, vstr, ValeStrFrom(""));
}

void threadSleep(int seconds) {
  sleep(seconds);
}


void makeDirectory(ValeStr* path) {
  struct stat st = {0};
  // printf("making directory path size: %lld\n", path->length);
  // printf("making directory path ptr: %p\n", path);
  // printf("making directory path chars ptr: %p\n", path->chars);
  // printf("making directory path end char int %d\n", path->chars[path->length]);
  // printf("making directory: %s\n", path->chars);
  if (stat(path->chars, &st) == -1) {
      mkdir(path->chars, 0700);
  } else {
    // fprintf(stderr, "Directory already exists: %s\n", path->chars);
  }
  ValeReleaseMessage(path);
}

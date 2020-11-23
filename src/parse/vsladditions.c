#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>


// These are exposed by the compiled vale .obj/.o, they're
// the start of a Vale native API.
typedef struct ValeStr ValeStr;
ValeStr* vale_newstr(int64_t length);
char* vale_getstrchars(ValeStr* str);
int64_t vale_getstrnumbytes(ValeStr* str);


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

ValeStr* executeProgram(ValeStr* commandVStr) {
  char* command = vale_getstrchars(commandVStr);

  FILE* fp = popen(command, "r");
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


  ValeStr* vstr = vale_newstr(totalChars);
  char* vstrChars = vale_getstrchars(vstr);

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
    vstrChars[i] = *charPtrInBuffer;
  }

  assert(head == first);
  assert(charPtrInBuffer == &first->buffer[0]);
  free(head);

  fclose(fp);

  return vstr;
}

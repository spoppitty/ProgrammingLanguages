//> Chunks of Bytecode memory-c
#include <stdlib.h>
#include <string.h>

//> Garbage Collection memory-include-compiler
#include "compiler.h"
//< Garbage Collection memory-include-compiler
#include "memory.h"
//> Strings memory-include-vm
#include "vm.h"
//< Strings memory-include-vm
//> Garbage Collection debug-log-includes

#ifdef DEBUG_LOG_GC
#include <stdio.h>
#include "debug.h"
#endif
//< Garbage Collection debug-log-includes
//> Garbage Collection heap-grow-factor

#define GC_HEAP_GROW_FACTOR 2
//< Garbage Collection heap-grow-factor

#define HEAP_SIZE (64 * 1024 * 1024)
#define ALIGNMENT sizeof(void*)

typedef struct Block {
  size_t size;
  bool free;
  struct Block* next;
  struct Block* previous;
} Block;

static void* heapMemory = NULL;
static Block* firstBlock = NULL;

static size_t alignSize(size_t size) {
  return (size + ALIGNMENT - 1) &
         ~(ALIGNMENT - 1);
}

void initMemory() {
  heapMemory = malloc(HEAP_SIZE);

  if (heapMemory == NULL) {
    exit(1);
  }

  firstBlock = (Block*)heapMemory;
  firstBlock->size = HEAP_SIZE - sizeof(Block);
  firstBlock->free = true;
  firstBlock->next = NULL;
  firstBlock->previous = NULL;
}

static Block* findFreeBlock(size_t size) {
  Block* block = firstBlock;

  while (block != NULL) {
    if (block->free && block->size >= size) {
      return block;
    }

    block = block->next;
  }

  return NULL;
}

static void splitBlock(Block* block, size_t size) {
  if (block->size < size + sizeof(Block) + ALIGNMENT) {
    return;
  }

  Block* newBlock = (Block*)((char*)block +
      sizeof(Block) + size);

  newBlock->size =
      block->size - size - sizeof(Block);
  newBlock->free = true;
  newBlock->next = block->next;
  newBlock->previous = block;

  if (newBlock->next != NULL) {
    newBlock->next->previous = newBlock;
  }

  block->next = newBlock;
  block->size = size;
}

static void mergeWithNext(Block* block) {
  Block* next = block->next;

  if (next == NULL || !next->free) {
    return;
  }

  block->size += sizeof(Block) + next->size;
  block->next = next->next;

  if (block->next != NULL) {
    block->next->previous = block;
  }
}

static void releaseBlock(Block* block) {
  block->free = true;

  if (block->next != NULL && block->next->free) {
    mergeWithNext(block);
  }

  if (block->previous != NULL &&
      block->previous->free) {
    mergeWithNext(block->previous);
  }
}

void* reallocate(void* pointer, size_t oldSize, size_t newSize) {
  if (newSize == 0) {
    if (pointer != NULL) {
      Block* block = (Block*)pointer - 1;
      releaseBlock(block);
    }

    vm.bytesAllocated -= oldSize;
    return NULL;
  }

  size_t requestedSize = alignSize(newSize);

  // Allocate a new block.
  if (pointer == NULL) {
    Block* block = findFreeBlock(requestedSize);

    if (block == NULL) {
      collectGarbage();
      block = findFreeBlock(requestedSize);
    }

    if (block == NULL) {
      exit(1);
    }

    splitBlock(block, requestedSize);
    block->free = false;

    vm.bytesAllocated += newSize;

    return (void*)(block + 1);
  }

  Block* block = (Block*)pointer - 1;

  // The current block is already large enough.
  if (block->size >= requestedSize) {
    splitBlock(block, requestedSize);
    vm.bytesAllocated += newSize - oldSize;
    return pointer;
  }

  // Try expanding into the following free block.
  if (block->next != NULL &&
      block->next->free &&
      block->size + sizeof(Block) +
          block->next->size >= requestedSize) {
    mergeWithNext(block);
    splitBlock(block, requestedSize);

    vm.bytesAllocated += newSize - oldSize;
    return pointer;
  }

  // Allocate elsewhere and copy the old contents.
  Block* newBlock = findFreeBlock(requestedSize);

  if (newBlock == NULL) {
    collectGarbage();
    newBlock = findFreeBlock(requestedSize);
  }

  if (newBlock == NULL) {
    exit(1);
  }

  splitBlock(newBlock, requestedSize);
  newBlock->free = false;

  void* newPointer = (void*)(newBlock + 1);
  memcpy(newPointer, pointer, oldSize);

  releaseBlock(block);

  vm.bytesAllocated += newSize - oldSize;

  return newPointer;
}
//> Garbage Collection mark-object
void markObject(Obj* object) {
  if (object == NULL) return;
//> check-is-marked
  if (object->isMarked) return;

//< check-is-marked
//> log-mark-object
#ifdef DEBUG_LOG_GC
  printf("%p mark ", (void*)object);
  printValue(OBJ_VAL(object));
  printf("\n");
#endif

//< log-mark-object
  object->isMarked = true;
//> add-to-gray-stack

  if (vm.grayCapacity < vm.grayCount + 1) {
    int oldCapacity = vm.grayCapacity;
    int newCapacity = GROW_CAPACITY(oldCapacity);

    vm.grayStack = (Obj**)reallocate(vm.grayStack, sizeof(Obj*) * oldCapacity, sizeof(Obj*) * newCapacity);
    vm.grayCapacity = newCapacity;
  }

  vm.grayStack[vm.grayCount++] = object;
//< add-to-gray-stack
}
//< Garbage Collection mark-object
//> Garbage Collection mark-value
void markValue(Value value) {
  if (IS_OBJ(value)) markObject(AS_OBJ(value));
}
//< Garbage Collection mark-value
//> Garbage Collection mark-array
static void markArray(ValueArray* array) {
  for (int i = 0; i < array->count; i++) {
    markValue(array->values[i]);
  }
}
//< Garbage Collection mark-array
//> Garbage Collection blacken-object
static void blackenObject(Obj* object) {
//> log-blacken-object
#ifdef DEBUG_LOG_GC
  printf("%p blacken ", (void*)object);
  printValue(OBJ_VAL(object));
  printf("\n");
#endif

//< log-blacken-object
  switch (object->type) {
//> Methods and Initializers blacken-bound-method
    case OBJ_BOUND_METHOD: {
      ObjBoundMethod* bound = (ObjBoundMethod*)object;
      markValue(bound->receiver);
      markObject((Obj*)bound->method);
      break;
    }
//< Methods and Initializers blacken-bound-method
//> Classes and Instances blacken-class
    case OBJ_CLASS: {
      ObjClass* klass = (ObjClass*)object;
      markObject((Obj*)klass->name);
//> Methods and Initializers mark-methods
      markTable(&klass->methods);
//< Methods and Initializers mark-methods
      break;
    }
//< Classes and Instances blacken-class
//> blacken-closure
    case OBJ_CLOSURE: {
      ObjClosure* closure = (ObjClosure*)object;
      markObject((Obj*)closure->function);
      for (int i = 0; i < closure->upvalueCount; i++) {
        markObject((Obj*)closure->upvalues[i]);
      }
      break;
    }
//< blacken-closure
//> blacken-function
    case OBJ_FUNCTION: {
      ObjFunction* function = (ObjFunction*)object;
      markObject((Obj*)function->name);
      markArray(&function->chunk.constants);
      break;
    }
//< blacken-function
//> Classes and Instances blacken-instance
    case OBJ_INSTANCE: {
      ObjInstance* instance = (ObjInstance*)object;
      markObject((Obj*)instance->klass);
      markTable(&instance->fields);
      break;
    }
//< Classes and Instances blacken-instance
//> blacken-upvalue
    case OBJ_UPVALUE:
      markValue(((ObjUpvalue*)object)->closed);
      break;
//< blacken-upvalue
    case OBJ_NATIVE:
    case OBJ_STRING:
      break;
  }
}
//< Garbage Collection blacken-object
//> Strings free-object
static void freeObject(Obj* object) {
//> Garbage Collection log-free-object
#ifdef DEBUG_LOG_GC
  printf("%p free type %d\n", (void*)object, object->type);
#endif

//< Garbage Collection log-free-object
  switch (object->type) {
//> Methods and Initializers free-bound-method
    case OBJ_BOUND_METHOD:
      FREE(ObjBoundMethod, object);
      break;
//< Methods and Initializers free-bound-method
//> Classes and Instances free-class
    case OBJ_CLASS: {
//> Methods and Initializers free-methods
      ObjClass* klass = (ObjClass*)object;
      freeTable(&klass->methods);
//< Methods and Initializers free-methods
      FREE(ObjClass, object);
      break;
    } // [braces]
//< Classes and Instances free-class
//> Closures free-closure
    case OBJ_CLOSURE: {
//> free-upvalues
      ObjClosure* closure = (ObjClosure*)object;
      FREE_ARRAY(ObjUpvalue*, closure->upvalues,
                 closure->upvalueCount);
//< free-upvalues
      FREE(ObjClosure, object);
      break;
    }
//< Closures free-closure
//> Calls and Functions free-function
    case OBJ_FUNCTION: {
      ObjFunction* function = (ObjFunction*)object;
      freeChunk(&function->chunk);
      FREE(ObjFunction, object);
      break;
    }
//< Calls and Functions free-function
//> Classes and Instances free-instance
    case OBJ_INSTANCE: {
      ObjInstance* instance = (ObjInstance*)object;
      freeTable(&instance->fields);
      FREE(ObjInstance, object);
      break;
    }
//< Classes and Instances free-instance
//> Calls and Functions free-native
    case OBJ_NATIVE:
      FREE(ObjNative, object);
      break;
//< Calls and Functions free-native
    case OBJ_STRING: {
      ObjString* string = (ObjString*)object;
      FREE_ARRAY(char, string->chars, string->length + 1);
      FREE(ObjString, object);
      break;
    }
//> Closures free-upvalue
    case OBJ_UPVALUE:
      FREE(ObjUpvalue, object);
      break;
//< Closures free-upvalue
  }
}
//< Strings free-object
//> Garbage Collection mark-roots
static void markRoots() {
  for (int slot = 0; slot < vm.stackCount; slot++) {
    markValue(vm.stack[slot]);
  }
//> mark-closures

  for (int i = 0; i < vm.frameCount; i++) {
    markObject((Obj*)vm.frames[i].closure);
  }
//< mark-closures
//> mark-open-upvalues

  for (ObjUpvalue* upvalue = vm.openUpvalues;
       upvalue != NULL;
       upvalue = upvalue->next) {
    markObject((Obj*)upvalue);
  }
//< mark-open-upvalues
//> mark-globals

  markTable(&vm.globals);
//< mark-globals
//> call-mark-compiler-roots
  markCompilerRoots();
//< call-mark-compiler-roots
//> Methods and Initializers mark-init-string
  markObject((Obj*)vm.initString);
//< Methods and Initializers mark-init-string
}
//< Garbage Collection mark-roots
//> Garbage Collection trace-references
static void traceReferences() {
  while (vm.grayCount > 0) {
    Obj* object = vm.grayStack[--vm.grayCount];
    blackenObject(object);
  }
}
//< Garbage Collection trace-references
//> Garbage Collection sweep
static void sweep() {
  Obj* previous = NULL;
  Obj* object = vm.objects;
  while (object != NULL) {
    if (object->isMarked) {
//> unmark
      object->isMarked = false;
//< unmark
      previous = object;
      object = object->next;
    } else {
      Obj* unreached = object;
      object = object->next;
      if (previous != NULL) {
        previous->next = object;
      } else {
        vm.objects = object;
      }

      freeObject(unreached);
    }
  }
}
//< Garbage Collection sweep
//> Garbage Collection collect-garbage
void collectGarbage() {
//> log-before-collect
#ifdef DEBUG_LOG_GC
  printf("-- gc begin\n");
//> log-before-size
  size_t before = vm.bytesAllocated;
//< log-before-size
#endif
//< log-before-collect
//> call-mark-roots

  markRoots();
//< call-mark-roots
//> call-trace-references
  traceReferences();
//< call-trace-references
//> sweep-strings
  tableRemoveWhite(&vm.strings);
//< sweep-strings
//> call-sweep
  sweep();
//< call-sweep
//> update-next-gc

  vm.nextGC = vm.bytesAllocated * GC_HEAP_GROW_FACTOR;
//< update-next-gc
//> log-after-collect

#ifdef DEBUG_LOG_GC
  printf("-- gc end\n");
//> log-collected-amount
  printf("   collected %zu bytes (from %zu to %zu) next at %zu\n",
         before - vm.bytesAllocated, before, vm.bytesAllocated,
         vm.nextGC);
//< log-collected-amount
#endif
//< log-after-collect
}
//< Garbage Collection collect-garbage
//> Strings free-objects
void freeObjects() {
  Obj* object = vm.objects;
  while (object != NULL) {
    Obj* next = object->next;
    freeObject(object);
    object = next;
  }
//> Garbage Collection free-gray-stack

  reallocate(vm.grayStack, sizeof(Obj*) * vm.grayCapacity, 0);

  vm.grayStack = NULL;
  vm.grayCapacity = 0;
//< Garbage Collection free-gray-stack
}
//< Strings free-objects

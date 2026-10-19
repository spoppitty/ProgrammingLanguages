//> Hash Tables table-h
#ifndef clox_table_h
#define clox_table_h

#include "common.h"
#include "value.h"
//> entry

typedef struct {
  Value key; // was ObjString* key
  Value value;
} Entry;

//< entry

typedef struct {
  int count;
  int capacity;
  Entry* entries;
} Table;

//> init-table-h
void initTable(Table* table);
//> free-table-h
void freeTable(Table* table);
//< free-table-h
//> table-get-h
bool tableGet(Table* table, Value key, Value* value);
bool tableSet(Table* table, Value key, Value value);
bool tableDelete(Table* table, Value key);
//< table-delete-h
//> table-add-all-h
void tableAddAll(Table* from, Table* to);
//< table-add-all-h
//> table-find-string-h
ObjString* tableFindString(Table* table, const char* chars,
                           int length, uint32_t hash);
//< table-find-string-h
//> Garbage Collection table-remove-white-h

void tableRemoveWhite(Table* table);
//< Garbage Collection table-remove-white-h
//> Garbage Collection mark-table-h
void markTable(Table* table);
//< Garbage Collection mark-table-h

//< init-table-h
#endif

#include <stdio.h>
#include <time.h>

#include "table.h"
#include "value.h"
#include "vm.h"

static volatile double checksum = 0;

static double secondsSince(clock_t start) {
  return (double)(clock() - start) / CLOCKS_PER_SEC;
}

static double benchmarkInsertAndLookup(int count) {
  Table table;
  initTable(&table);

  clock_t start = clock();

  for (int i = 0; i < count; i++) {
    tableSet(&table, NUMBER_VAL(i), NUMBER_VAL(i * 2));
  }

  for (int i = 0; i < count; i++) {
    Value value;

    if (tableGet(&table, NUMBER_VAL(i), &value)) {
      checksum += AS_NUMBER(value);
    }
  }

  double elapsed = secondsSince(start);

  freeTable(&table);
  return elapsed;
}

static double benchmarkUpdates(int keyCount, int updateCount) {
  Table table;
  initTable(&table);

  for (int i = 0; i < keyCount; i++) {
    tableSet(&table, NUMBER_VAL(i), NUMBER_VAL(i));
  }

  clock_t start = clock();

  for (int i = 0; i < updateCount; i++) {
    int key = i % keyCount;
    tableSet(&table, NUMBER_VAL(key), NUMBER_VAL(i));
  }

  double elapsed = secondsSince(start);

  freeTable(&table);
  return elapsed;
}

static double benchmarkDeletions(int count) {
  Table table;
  initTable(&table);

  for (int i = 0; i < count; i++) {
    tableSet(&table, NUMBER_VAL(i), NUMBER_VAL(i));
  }

  clock_t start = clock();

  for (int i = 0; i < count; i += 2) {
    tableDelete(&table, NUMBER_VAL(i));
  }

  for (int i = 0; i < count; i += 2) {
    Value value;

    if (tableGet(&table, NUMBER_VAL(i), &value)) {
      checksum += AS_NUMBER(value);
    }
  }

  double elapsed = secondsSince(start);

  freeTable(&table);
  return elapsed;
}

int main() {
  initVM();

  printf("Hash Table Benchmarks\n\n");

  double smallTime = benchmarkInsertAndLookup(1000);
  printf("Small insert + lookup (1,000 keys): %.6f seconds\n",
         smallTime);

  double largeTime = benchmarkInsertAndLookup(100000);
  printf("Large insert + lookup (100,000 keys): %.6f seconds\n",
         largeTime);

  double updateTime = benchmarkUpdates(1000, 1000000);
  printf("Repeated updates (1,000,000 updates): %.6f seconds\n",
         updateTime);

  double deleteTime = benchmarkDeletions(100000);
  printf("Deletion-heavy test (100,000 keys): %.6f seconds\n",
         deleteTime);

  printf("\nChecksum: %.0f\n", checksum);

  freeVM();
  return 0;
}
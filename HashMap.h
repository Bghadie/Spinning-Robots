#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define HASH_MAX 32


struct DataItem{
  Robot r;
  int key;
};

long int raise(int, int);
struct DataItem* hashArray[HASH_MAX];
struct DataItem* dummyItem;
struct DataItem* item;
int n; //size of the data structure

//multiplicative hash function curtosy of pat morin
int hashCode(int r){
  srand(time(NULL));
  long int randNum = 2;
  long int d=32, w = raise(2, 32), x = r;
  while(1){
    randNum = rand() % (w - 1) + 1;
    if(randNum % 2 == 1){
      break;
    }  
  }
  return (randNum*x) >> (32 - d);
}


//raise a base b to the power of exponent e
long int raise(int b, int e){
  long int result = 1;
  long int expt = e;
  long int base = b;
  for(;;){
    if(expt & 1)
      result *= base;
    expt >>= 1;
    if(!expt || e < 0)
      break;
    base *= base;
  }
  return result;

}


//two robots are said to be 
int greaterThan(Robot *r1, Robot *r2){
  return -1;
}

/*
fifo.h

Created 2019 by Henrik

Copyright (C) 2019 Henrik Bjorkman www.eit.se/hb.
All rights reserved etc etc...
*/



#ifndef FIFO_H
#define FIFO_H

#include "string.h"


struct Fifo
{
  uint8_t head;
  uint8_t tail;
  char buffer[256];
};




static inline void fifoInit(volatile struct Fifo *fifoPtr)
{
	memset((void*)fifoPtr, 0, sizeof(*fifoPtr));
}

static inline void fifoPut(volatile struct Fifo *fifoPtr, char ch)
{
  fifoPtr->buffer[fifoPtr->head] = ch;
  fifoPtr->head++;
}

static inline int fifoIsFull(volatile struct Fifo *fifoPtr)
{
  return (((uint8_t)(fifoPtr->head+1)) == fifoPtr->tail);
}

static inline int fifoIsEmpty(volatile struct Fifo *fifoPtr)
{
  return (fifoPtr->head == fifoPtr->tail);
}

static inline char fifoTake(volatile struct Fifo *fifoPtr)
{
  const char tmp = fifoPtr->buffer[fifoPtr->tail];
  ++fifoPtr->tail;
  return tmp;
}

#endif


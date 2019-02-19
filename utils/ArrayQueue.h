#ifndef ARRAYQUEUE_H__
#define ARRAYQUEUE_H__

#include "Arduino.h"

namespace utils
{

template <uint32_t N, typename T, T defaultValue>
class ArrayQueue
{
public:
  bool isEmpty()
  {
    return size == 0;
  }

  bool isFull()
  {
    return size == N;
  }

  T get()
  {
    if (size == 0)
    {
      return defaultValue;
    }

    return store[front];
  }

  void enqueue(T value)
  {
    if (size == N)
    {
      store[rear] = value;
      return;
    }

    if (size == 0)
    {
      front = 0;
      rear = 0;
    }
    else if (rear == N - 1)
    {
      rear = 0;
    }
    else
    {
      rear++;
    }

    size++;
    store[rear] = value;
  }

  void deque()
  {
    if (size == 0)
    {
      return;
    }

    size--;
    if (front == N - 1)
    {
      front = 0;
    }
    else
    {
      front++;
    }
  }

  uint32_t getSize()
  {
    return size;
  }

  void printArrayQueue(Print &printer)
  {
    printer.print(": queue with s=");
    printer.print(size, DEC);
    printer.print(", f=");
    printer.print(front, DEC);
    printer.print(", r=");
    printer.print(rear, DEC);
    printer.print(": ");

    for (int i = 0; i < N; i++)
    {
      if (front == i)
      {
        printer.print("f->");
      }
      if (rear == i)
      {
        printer.print("r->");
      }

      printer.print("[");
      printer.print(i);
      printer.print("]=");
      printer.print(store[i]);
      printer.print(" ");
    }

    printer.println();
    printer.flush();
  }

private:
  uint32_t size = 0;
  uint32_t front = 0;
  uint32_t rear = 0;
  T store[N];
};

} // namespace utils

#endif
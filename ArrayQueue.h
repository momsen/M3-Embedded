#ifndef ARRAYQUEUE_H__
#define ARRAYQUEUE_H__

template <byte N, typename T, T defaultValue>
class ArrayQueue
{
  public:
    bool isEmpty()
    {
        return size <= 0;
    }

    bool isFull()
    {
        return size == N;
    }

    T get()
    {
        if (isEmpty())
        {
            return defaultValue;
        }

        return store[front];
    }

#if DEBUG == 1
    void printArray(const char *access)
    {
        Serial.print(access);
        Serial.print(": queue with s=");
        Serial.print(size, DEC);
        Serial.print(", f=");
        Serial.print(front, DEC);
        Serial.print(", r=");
        Serial.print(rear, DEC);
        Serial.print(": ");

        for (int i = 0; i < N; i++)
        {
            if (front == i)
            {
                Serial.print("f->");
            }
            if (rear == i)
            {
                Serial.print("r->");
            }

            Serial.print("[");
            Serial.print(i);
            Serial.print("]=");
            Serial.print(store[i]);
            Serial.print(" ");
        }

        Serial.println();
        Serial.flush();
    }
#endif

    void enqueue(T value)
    {
        if (isFull())
        {
            store[rear] = value;
            return;
        }

        if (size == 255)
        {
            front = 0;
            rear = 0;
            size = 0;
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
        if (isEmpty())
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

    byte getSize()
    {
        return size;
    }

  private:
    byte size = 255;
    byte front = 255;
    byte rear = 255;
    T store[N];
};

#endif
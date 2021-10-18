#pragma once
#include <fstream>
#include <iostream>
#include <queue>

#define DEBUG_LOG 1

int CheckBuff2Empty(unsigned long int* buffer);

class Queue
{
public:
	Queue(int maxLen) { m_maxLen = maxLen; }

	bool push(unsigned long int value)
	{
		if (m_Q.size() == m_maxLen) return false;
		else
		{
			m_Q.push(value);
			return true;
		}
	}

	void pop() { m_Q.pop(); }

	std::size_t size() { return m_Q.size(); }

    // The size of the buffer pointed to by pBuffer MUST be equal to the Q size
	void readQ(unsigned long int* pBuffer)
	{
        size_t qSize = m_Q.size();

        for (int i = 0; i < qSize; ++i)
        {
            // read the next element and store it in the buffer, pop from Q
            // and then push it back
            pBuffer[i] = m_Q.front();
            m_Q.pop();
            m_Q.push(pBuffer[i]);
        }
	}

	bool isFull()
	{
		if (m_Q.size() == m_maxLen) return true;
		else return false;
	}

    int getMaxLen() { return m_maxLen; }
private:
	std::queue<unsigned long int> m_Q;
	int m_maxLen;
};
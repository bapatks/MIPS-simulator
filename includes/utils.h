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

	bool readQ(unsigned long int* pBuffer)
	{
		size_t bufferSize = sizeof(*pBuffer) / sizeof(unsigned long int);

		if (bufferSize == m_maxLen)
		{
			for (int i = 0; i < m_maxLen; ++i)
			{
				// read the next element and store it in the buffer, pop from Q
				// and then push it back
				pBuffer[i] = m_Q.front();
				m_Q.pop();
				m_Q.push(pBuffer[i]);
			}

			return true;
		}

		return false;
	}

	bool isFull()
	{
		if (m_Q.size() == m_maxLen) return true;
		else return false;
	}
private:
	std::queue<unsigned long int> m_Q;
	int m_maxLen;
};
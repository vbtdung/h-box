/*
Copyright (c) 2010-2012 Aalto University

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#ifndef THREADSAFEQUEUE_HH
#define THREADSAFEQUEUE_HH

#include <queue>
#include <mutex>

using namespace std;

/**
 * @class blocking_queue
 * @brief A communication queue between threads. This data structure assures thread-safe by using semaphore.
 * @author Vu Ba Tien Dung
 *
 */
template<typename T> class blocking_queue {
private:
	queue<T> data;
	mutable mutex m;
	
public:
	// RO3 pattern
	blocking_queue() {}
	
	blocking_queue& operator=(const blocking_queue& other) {
		lock_guard<mutex> lock(other.m);
		data = other.data;
	}
	
	blocking_queue(const blocking_queue& other)	= delete; /* {
		lock_guard<mutex> lock(other.m);
		data = other.data;
	} */
	
	// push
	void push(const T& newvalue) {
		lock_guard<mutex> lock(m);
		data.push(newvalue);
	}
	
	// pop
	bool pop(T& value) {
		lock_guard<mutex> lock(m);
		if (data.empty()) return false;

		value = data.front();
		data.pop();
		return true;
	}	
	
	// size
	int size() {
		return data.size();
	}
};

#endif

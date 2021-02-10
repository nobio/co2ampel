#ifndef Fifo_h
#define Fifo_h
#include <Arduino.h>
#ifndef FIFO_SIZE_TYPE
#ifndef FIFO_LARGE
#define FIFO_SIZE_TYPE uint8_t
#else
#define FIFO_SIZE_TYPE uint16_t
#endif
#endif
/*
||
||
*/
template<typename T, int rawSize>
class Fifo {
public:
	const FIFO_SIZE_TYPE size;				//speculative feature, in case it's needed

	Fifo();

	T dequeue();				//get next element
	bool enqueue( T element );	//add an element
	T peek() const;				//get the next element without releasing it from the FIFO
	void flush();				//[1.1] reset to default state 

	//how many elements are currently in the FIFO?
	FIFO_SIZE_TYPE count() { return numberOfElements; }

private:
#ifndef Fifo_NONVOLATILE
	volatile FIFO_SIZE_TYPE numberOfElements;
	volatile FIFO_SIZE_TYPE nextIn;
	volatile FIFO_SIZE_TYPE nextOut;
	volatile T raw[rawSize];
#else
	FIFO_SIZE_TYPE numberOfElements;
	FIFO_SIZE_TYPE nextIn;
	FIFO_SIZE_TYPE nextOut;
	T raw[rawSize];
#endif
};

template<typename T, int rawSize>
Fifo<T,rawSize>::Fifo() : size(rawSize) {
	flush();
}
template<typename T, int rawSize>
bool Fifo<T,rawSize>::enqueue( T element ) {
    Serial.print("enqueue"); Serial.println(element);
	if ( count() >= rawSize ) { dequeue(); }
	numberOfElements++;
	nextIn %= size;
	raw[nextIn] = element;
	nextIn++; //advance to next index
	return true;
}
template<typename T, int rawSize>
T Fifo<T,rawSize>::dequeue() {
	numberOfElements--;
	nextOut %= size;
	return raw[ nextOut++];
}
template<typename T, int rawSize>
T Fifo<T,rawSize>::peek() const {
	return raw[ nextOut % size];
}
template<typename T, int rawSize>
void Fifo<T,rawSize>::flush() {
	nextIn = nextOut = numberOfElements = 0;
}
#endif
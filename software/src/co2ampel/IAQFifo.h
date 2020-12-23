
template <int rawSize>
class IAQFifo
{
public:
    IAQFifo();                               // constructor
    void flush();                            //[1.1] reset to default state
    float push(float element);               //add an element and remove the oldes if necessary; returns the removed element
    int count() { return numberOfElements; } //how many elements are currently in the FIFO?
    float average();                         //calculates the average of float buffer
    void toString();

private:
    int numberOfElements;
    const int size; //speculative feature, in case it's needed
    int nextIn;
    int nextOut;
    float raw[rawSize];
    float dequeue();             //get next element
    bool enqueue(float element); //add an element
    float peek() const;          //get the next element without releasing it from the FIFO
};

template <int rawSize>
IAQFifo<rawSize>::IAQFifo() : size(rawSize)
{
    flush();
}
template <int rawSize>
bool IAQFifo<rawSize>::enqueue(float element)
{
    if (count() >= rawSize)
    {
        return false;
    }
    numberOfElements++;
    nextIn %= size;
    raw[nextIn] = element;
    nextIn++; //advance to next index
    return true;
}
template <int rawSize>
float IAQFifo<rawSize>::dequeue()
{
    numberOfElements--;
    nextOut %= size;
    return raw[nextOut++];
}
template <int rawSize>
float IAQFifo<rawSize>::push(float element)
{
    float elem;
    if (count() >= rawSize)
    {
        elem = this->dequeue();
    }
    this->enqueue(element);
    return elem;
}
template <int rawSize>
float IAQFifo<rawSize>::peek() const
{
    return raw[nextOut % size];
}
template <int rawSize>
void IAQFifo<rawSize>::flush()
{
    nextIn = nextOut = numberOfElements = 0;
}
template <int rawSize>
float IAQFifo<rawSize>::average()
{
    if (count() == 0)
    {
        return -1.0;
    }

    float sum = 0;
    for (int n = 0; n < count(); n++)
    {
        sum += raw[n];
    }
    return sum / count();
}
template <int rawSize>
void IAQFifo<rawSize>::toString()
{
    Serial.print("> ");
    for (int n = 0; n < count(); n++)
    {
        Serial.print(raw[n]);
        Serial.print(",");
    }
    Serial.println();
}

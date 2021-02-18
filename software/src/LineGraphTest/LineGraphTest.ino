#include "Point.h"
#include "SimpleFIFO.h"


void setup() {
  Serial.begin(115200);
}

void loop() {
  bool b;
  SimpleFIFO<float, 5> fifo;
  //Point* p = new Point(2, 4);
  for(int n=0; n<10; n++) {
    b = fifo.enqueue(n);
    Serial.print(n); Serial.print(" => "); Serial.print(b);
    Serial.print("; erstes Element: "); Serial.print(fifo.elementAt(0));
    Serial.print("; letztes Element: "); Serial.println(fifo.elementAt(fifo.count()-1));
  }
  delay(5000);
}

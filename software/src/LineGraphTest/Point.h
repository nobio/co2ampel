/** 
 * Immutable Point object; 
 * Data could only be set by constructor
 */
// header
class Point {
 public:
    Point(float x, float y);
//    ~Point();
    float x() const;
    float y() const;
 private:
    const float x_;
    const float y_;
 };

 // implementation
 Point::Point(float x, float y) : x_(x), y_(y) {}
 //Point::~Point() {} 
 float Point::x() const { return x_; }
 float Point::y() const { return y_; }

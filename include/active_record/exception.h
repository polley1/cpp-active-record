#ifndef _ACTIVE_RECORD_EXCEPTION_H_
#define _ACTIVE_RECORD_EXCEPTION_H_

#include <string>
#include <sstream>

using namespace std;

namespace ActiveRecord {
class ActiveRecordException;
}

ostream & operator<<( ostream &cout, const ActiveRecord::ActiveRecordException &e );

namespace ActiveRecord {

class ActiveRecordException {
 public:
  ActiveRecordException( const string &message,
                         const string &file = "",
                         long         line  = -1 )
    : message_( message ),
      file_( file ),
      line_( line ) {}
  string message()   const { return message_; }
  string file()      const { return file_; }
  long   line()      const { return line_; }
  string to_string() const {
    stringstream ss;
    if( file_.size() > 0 ) {
      ss << file_;
      if( line_ > 0 )
        ss << ":" << line_;
      ss << ": ";
    }
    ss << message_;
    return ss.str();
  }

 private:
  string message_;
  string file_;
  long   line_;
};

} // namespace ActiveRecord

#endif // ndef _ACTIVE_RECORD_EXCEPTION_H_

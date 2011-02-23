#ifndef _ACTIVE_RECORD_ROW_H_
#define _ACTIVE_RECORD_ROW_H_

#include <active_record/type.h>
#include <active_record/attribute.h>
#include <sqlite3.h>

namespace ActiveRecord {

class Row {
 public:
  Row() {};
  Row( sqlite3_stmt *ppStmt );
  bool   has_data()                 { return attributes_.size() > 0; };
  Type   get_type( const string &name );
  bool   is_type( const string &name, Type type );
  string get_text( const string &name );
  int    get_integer( const string &name );
  double get_floating_point( const string &name );
  const string         to_string();
  const AttributeHash& attributes() { return attributes_; };

 private:
  AttributeHash attributes_;
};

typedef vector< Row > RowSet;

} // namespace ActiveRecord

#endif // ndef _ACTIVE_RECORD_ROW_H_

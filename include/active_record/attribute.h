#ifndef _ACTIVE_RECORD_ATTRIBUTE_H_
#define _ACTIVE_RECORD_ATTRIBUTE_H_

#include <map>
#include <list>
#include <boost/assign.hpp>
#include <boost/variant.hpp>
#include <boost/assign/list_of.hpp>
#include <active_record/type.h>
#include <active_record/date.h>

using namespace std;
using namespace boost::assign;
using namespace boost;

typedef boost::variant< int, string, double, Date > AttributeType;

// N.B. boost::variant.which() returns a 0-based index into the AttributeType list
class Attribute : public AttributeType {
 public:
  Attribute() : AttributeType() {}
  Attribute( int i ) : AttributeType( i ) {}
  Attribute( const string &s ) : AttributeType( s ) {}
  Attribute( const char *s ) : AttributeType( string( s ) ) {}
  Attribute( double d ) : AttributeType( d ) {}
  Attribute( Date date ) : AttributeType( date ) {}

  Type type() const;
  bool operator==( const Attribute& other ) const;
};

typedef pair< string, Attribute >             AttributePair;
typedef map< string, Attribute >              AttributeHash;
typedef list< AttributePair >                 AttributePairList;
typedef list< Attribute >                     AttributeList;
typedef pair< string, AttributeList >         QueryParametersPair;
typedef assign_detail::generic_list< AttributePair > GenericAttributePairList;

} // namespace ActiveRecord

// Instantiate boost::assign::list_of for our option type
// so we can define 'options' below
// Warning: this *may* be GCC STL-specific
namespace boost
{
namespace assign
{

template<>
inline ActiveRecord::GenericAttributePairList
list_of( const ActiveRecord::AttributePair &t ) {
  return ActiveRecord::GenericAttributePairList()( t );
}

} // namespace assign
} // namespace boost

namespace ActiveRecord {

/*
 ( attributes ( "foo" 13 ) ( "bar" "hello" ) ( "baz" 15.5 ) )
*/
inline GenericAttributePairList attributes( const char * name,
                                            const Attribute &value ) {
  return boost::assign::list_of( AttributePair( name, value ) );
}

} // namespace ActiveRecord

#endif // ndef _ACTIVE_RECORD_ATTRIBUTE_H_

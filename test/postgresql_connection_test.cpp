#include "test_helper.h"
#include <active_record/connection/postgresql.h>

extern const char * pg_user;

using namespace ActiveRecord;

/* N.B. These tests assume that a 'template1' database exists
 *      and that the supplied user can access that database
 *      and can create new databases */

class PostgresqlTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    test_db_ = "active_record_postgresql_test";
    access_db_ = "template1";
    pg_user_ = pg_user;
  }

 protected:
   string test_db_;
   string access_db_;
   string pg_user_;
   PostgresqlConnection connection_;
};

TEST_F(PostgresqlTest, ConnectToExistentDatabase) {
  ASSERT_NO_THROW(
    connection_.connect(
      options
        ("database", access_db_)
        ("username", pg_user_)
    )
  );
}

TEST_F(PostgresqlTest, ConnectToNonExistentDatabase) {
  ASSERT_THROW(
    connection_.connect(
      options
        ("database", "database_that_does_not_exist")
        ("username", pg_user_)
    ),
    ActiveRecord::ActiveRecordException
  );
}

TEST_F(PostgresqlTest, DropDatabase) {
  postgresql_shell_create_database(test_db_, access_db_, pg_user_);
  ASSERT_TRUE(postgresql_shell_database_exists(test_db_, pg_user_));

  // Connect *after* we've used template1 via command line to avoid
  // "ERROR:  source database access_db_ is being accessed by other users"
  connection_.connect(
    options
      ("database", access_db_)
      ("username", pg_user_)
  );

  ASSERT_NO_THROW(
    PostgresqlConnection::drop_database(connection_, test_db_)
  );
  ASSERT_FALSE(postgresql_shell_database_exists(test_db_, pg_user_));
}

class PostgresqlWithTemplate1ConnectionTest : public PostgresqlTest {
 protected:
  virtual void SetUp() {
    PostgresqlTest::SetUp();
    connection_.connect(
      options
        ("database", access_db_)
        ("username", pg_user_)
    );
  }
  virtual void TearDown() {
    connection_.disconnect();
    postgresql_shell_drop_database(test_db_, access_db_, pg_user_);
  }
};

TEST_F(PostgresqlWithTemplate1ConnectionTest, CreateDatabase) {
  bool created = PostgresqlConnection::create_database(
    connection_,
    options
      ("database", test_db_)
      ("owner",    pg_user_)
  );
  ASSERT_TRUE(created);
  ASSERT_TRUE(
    postgresql_shell_database_exists(test_db_, pg_user_)
  );
}

TEST_F(PostgresqlWithTemplate1ConnectionTest, DatabaseExists) {
  ASSERT_TRUE(PostgresqlConnection::database_exists(connection_, access_db_));
  ASSERT_FALSE(
    PostgresqlConnection::database_exists(
      connection_, "database_that_does_not_exist"
    )
  );
}

class PostgresqlWithConnectionTest : public PostgresqlTest {
 protected:
  virtual void SetUp() {
    PostgresqlTest::SetUp();
    postgresql_shell_create_database(test_db_, access_db_, pg_user_);
    connection_.connect(
      options
        ("database", test_db_)
        ("username", pg_user_)
    );
  }
  virtual void TearDown() {
    connection_.disconnect();
    postgresql_shell_drop_database(test_db_, access_db_, pg_user_);
  }
};

TEST_F(PostgresqlWithConnectionTest, TableExists) {
  postgresql_shell_command(test_db_, pg_user_, "CREATE TABLE foo ()");

  ASSERT_TRUE(connection_.table_exists("foo"));
  ASSERT_FALSE(connection_.table_exists("bar"));
} 

TEST_F(PostgresqlWithConnectionTest, Execute) {
  connection_.execute("CREATE TABLE bar ()");

  ASSERT_TRUE(connection_.table_exists("bar"));
}

class PostgresqlWithConnectionAndTableTest : public PostgresqlWithConnectionTest {
 protected:
  virtual void SetUp() {
    PostgresqlWithConnectionTest::SetUp();
    postgresql_shell_command(
      test_db_, pg_user_, "\
      CREATE TABLE foo (\
        pk SERIAL PRIMARY KEY, num INTEGER, word TEXT, f NUMERIC\
      )\
      "
    );
  }
};

TEST_F(PostgresqlWithConnectionAndTableTest, Insert) {
  long id = connection_.insert("INSERT INTO foo (num) VALUES (1) RETURNING pk;");

  ASSERT_EQ(id, 1);
  list<string> output = postgresql_shell_command(
    test_db_, pg_user_, "SELECT num FROM foo"
  );
  ASSERT_EQ(" num \n", output.front());
  output.pop_front();
  output.pop_front();
  ASSERT_EQ("   1\n", output.front());
}

TEST_F(PostgresqlWithConnectionAndTableTest, SelectOne) {
  connection_.execute("INSERT INTO foo (num) VALUES (42);");

  Row row = connection_.select_one("SELECT num FROM foo");
  ASSERT_EQ(42, row.get_integer("num"));
}

TEST_F(PostgresqlWithConnectionAndTableTest, SelectOneNoResult) {
  connection_.execute("INSERT INTO foo (num) VALUES (42);");

  Row row = connection_.select_one("SELECT num FROM foo WHERE pk = 3");
  ASSERT_FALSE(row.has_data());
}

TEST_F(PostgresqlWithConnectionAndTableTest, SelectOneWithParam) {
  connection_.execute("INSERT INTO foo (num) VALUES (42);");
  connection_.execute("INSERT INTO foo (num) VALUES (99);");

  Row row = connection_.select_one(
    "SELECT num FROM foo WHERE pk = $1", parameters (2)
  );
  ASSERT_EQ(99, row.get_integer("num"));
}

TEST_F(PostgresqlWithConnectionAndTableTest, SelectAll) {
  connection_.execute("INSERT INTO foo (num) VALUES (42);");
  connection_.execute("INSERT INTO foo (num) VALUES (99);");

  RowSet rows = connection_.select_all("SELECT num FROM foo");

  ASSERT_EQ(2, rows.size());
  RowSet::iterator it = rows.begin();
  ASSERT_EQ(42, it->get_integer("num"));
  ++it;
  ASSERT_EQ(99, it->get_integer("num"));
}

TEST_F(PostgresqlWithConnectionAndTableTest, SelectAllWithParameter) {
  connection_.execute("INSERT INTO foo (num) VALUES (42);");
  connection_.execute("INSERT INTO foo (num) VALUES (99);");

  RowSet rows = connection_.select_all(
    "SELECT num FROM foo WHERE pk = $1", parameters (2)
  );

  ASSERT_EQ(1, rows.size());
  RowSet::iterator it = rows.begin();
  ASSERT_EQ(99, it->get_integer("num"));
}

TEST_F(PostgresqlWithConnectionAndTableTest, SelectValue) {
  connection_.execute("INSERT INTO foo (num) VALUES (42);");

  ActiveRecord::Attribute result = connection_.select_value("SELECT num FROM foo");
  ASSERT_TRUE(result.has_data());
  ASSERT_EQ(result.type(), ActiveRecord::Type::integer);
  ASSERT_EQ(boost::get<int>(result), 42);
}

TEST_F(PostgresqlWithConnectionAndTableTest, SelectValueWithNoResult) {
  connection_.execute("INSERT INTO foo (num) VALUES (42);");

  ActiveRecord::Attribute result = connection_.select_value(
    "SELECT pk FROM foo WHERE num = 99"
  );
  ASSERT_FALSE(result.has_data());
}

TEST_F(PostgresqlWithConnectionAndTableTest, SelectValueWithIntParam) {
  connection_.execute("INSERT INTO foo (num) VALUES (42);");
  connection_.execute("INSERT INTO foo (num) VALUES (113);");

  ActiveRecord::Attribute result = connection_.select_value(
    "SELECT pk FROM foo WHERE num = $1", parameters (113)
  );
  ASSERT_TRUE(result.has_data());
  ASSERT_EQ(result.type(), ActiveRecord::Type::integer);
  ASSERT_EQ(boost::get<int>(result), 2);
}

TEST_F(PostgresqlWithConnectionAndTableTest, SelectValueWithTextParam) {
  connection_.execute("INSERT INTO foo (word) VALUES ('Hello');");
  connection_.execute("INSERT INTO foo (word) VALUES ('World');");

  ActiveRecord::Attribute result = connection_.select_value(
    "SELECT pk FROM foo WHERE word = $1", parameters ("World")
  );
  ASSERT_TRUE(result.has_data());
  ASSERT_EQ(result.type(), ActiveRecord::Type::integer);
  ASSERT_EQ(boost::get<int>(result), 2);
}

TEST_F(PostgresqlWithConnectionAndTableTest, SelectValueWithFloatParam) {
  connection_.execute("INSERT INTO foo (f) VALUES (-5.0);");
  connection_.execute("INSERT INTO foo (f) VALUES (9.99);");

  // TODO: use parameters () ()
  AttributeList params;
  params.push_back(9.9);
  params.push_back(10.0);
  ActiveRecord::Attribute result = connection_.select_value(
    "SELECT pk FROM foo WHERE f > $1 AND f < $2", params
  );
  ASSERT_TRUE(result.has_data());
  ASSERT_EQ(result.type(), ActiveRecord::Type::integer);
  ASSERT_EQ(boost::get<int>(result), 2);
}

TEST_F(PostgresqlWithConnectionAndTableTest, SelectValues) {
  connection_.execute("INSERT INTO foo (num) VALUES (42);");
  connection_.execute("INSERT INTO foo (num) VALUES (99);");

  AttributeList attributes = connection_.select_values("SELECT num FROM foo");

  ASSERT_EQ(2, attributes.size());
  AttributeList::iterator it = attributes.begin();
  assert_attribute(42, *it);
  ++it;
  assert_attribute(99, *it);
}

TEST_F(PostgresqlWithConnectionAndTableTest, SelectValuesWithParameter) {
  connection_.execute("INSERT INTO foo (num) VALUES (42);");
  connection_.execute("INSERT INTO foo (num) VALUES (99);");

  AttributeList attributes = connection_.select_values(
    "SELECT num FROM foo WHERE pk >= $1", parameters (2)
  );

  ASSERT_EQ(1, attributes.size());
  AttributeList::iterator it = attributes.begin();
  assert_attribute(99, *it);
}

TEST_F(PostgresqlWithConnectionAndTableTest, TableData) {
  Table t = connection_.table_data("foo");

  ASSERT_EQ(t.primary_key(), "pk");
  ASSERT_EQ(t.fields().size(), 3);
  assert_field(t, 0, "num", Type::integer);
  assert_field(t, 1, "word", Type::text);
  assert_field(t, 2, "f", Type::floating_point);
}

TEST_F(PostgresqlWithConnectionAndTableTest, Schema) {
  TableSet t = connection_.schema();

  ASSERT_EQ(1, t.size());
  TableSet::const_iterator it = t.begin();
  ASSERT_EQ("foo", it->first);
}

TEST_F(PostgresqlWithConnectionAndTableTest, FindsPrimaryKey) {
  ASSERT_EQ(connection_.primary_key("foo"), "pk");
}

TEST_F(PostgresqlWithConnectionAndTableTest, RemovesField) {
  connection_.remove_field("foo", "num");

  Table t = connection_.table_data("foo");

  ASSERT_EQ(t.primary_key(), "pk");
  ASSERT_EQ(t.fields().size(), 2);
  assert_field(t, 0, "word", Type::text);
  assert_field(t, 1, "f", Type::floating_point);
}

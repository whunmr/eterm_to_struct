DataEx
=========
Decode Erlang term into Declared c/c++ structure.

Example:
=========

```
___def_data(DataA)
  ___field(int) ia;
  ___field(int) ib;
___end_def_data;

___def_data(DataB)
  ___field(const char*) x;
  ___field(DataA) y;
___end_def_data;


TEST(SingleFieldData, xxx0) {
  DataA a;
  ETERM* tuplep = erl_format((char*)"{3, 4}");
  
  ___decode_eterm(a, tuplep);  

  EXPECT_EQ(3, (int)a.ia);
  EXPECT_EQ(4, (int)a.ib);
  erl_free_term(tuplep);
}

struct Eterm_to_DataB : public ::testing::TestWithParam<const char*> {
  virtual void SetUp() { tuplep_ = erl_format((char*)GetParam()); }
  virtual void TearDown() { erl_free_term(tuplep_); }
  ETERM* tuplep_;
};

TEST_P(Eterm_to_DataB, xxx4) {
  DataB d;
  
  ___decode_eterm(d, tuplep_);  

  EXPECT_STREQ("foo", d.x);
  EXPECT_EQ(3, (int)d.y._.ia);
  EXPECT_EQ(4, (int)d.y._.ib);
}

INSTANTIATE_TEST_CASE_P( TestTupleAndList, Eterm_to_DataB
                       , ::testing::Values( "[foo, {3, 4}]"
                       , "[foo, [3, 4]]"
                       , "{foo, {3, 4}}"
                       , "{foo, [3, 4]}"));

```


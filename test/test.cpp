#include <stdint.h>
#include <cstring>
#include <climits>
#include <iostream>
#include <gtest/gtest.h>
using namespace std;

#include "erl_interface.h"
#include "ei.h"

/*------------------------------------------------------------------------------*/
struct ArrayBase {};

template<typename T, size_t N>
struct __Array : ArrayBase {
    T elems[N];

    enum {capacity = N};
    typedef T element_type;
     
    operator T*() { return elems; }
};

#define __array(type, size) __Array<type, size>

/*------------------------------------------------------------------------------*/
typedef unsigned char FieldIndex;
typedef void (*DecodeFunc)(void* instance, size_t field_offset, const ETERM* msg);

static void* g_outmost_this_address = NULL;
static FieldIndex g_field_index_;

struct FieldInfo {
  FieldIndex index_;
  uint16_t offset_;
  DecodeFunc decode_func_;
  FieldInfo* next_field_info_;
};

static FieldInfo** g_pp_next_field_info_ = NULL;
static const FieldIndex kFieldIndexStart = 1;
static const FieldIndex kInvalidFieldIndex = 0;

/*----------------------------------------------------------------------------*/
struct ArgForceMetaRegister {} ___ArgForceMetaRegister;

template<typename T>
struct TypeMetaRegister {
  static T meta_register_global_obj;
};
template<typename T> T TypeMetaRegister<T>::meta_register_global_obj(&___ArgForceMetaRegister);

/*----------------------------------------------------------------------------*/
template<typename T>
struct FieldsInfo {
  FieldsInfo() { }
  FieldsInfo(void* p) {
    static T* __p_DataA = &TypeMetaRegister<T>::meta_register_global_obj;
    p = __p_DataA; //disable unused variable warning
    
    g_pp_next_field_info_ = &class_fields_info_;
    //cout << "1set g_pp_next_field_info_ at class register: " << (long)g_pp_next_field_info_ << endl;
  }
  static FieldInfo* class_fields_info_;
};
template<typename T> FieldInfo* FieldsInfo<T>::class_fields_info_ = NULL;

struct Serializable {
  typedef FieldInfo self_is_serializable;
  Serializable() {}
  Serializable(FieldInfo* class_fields_info_) : fields_info_in_instance_(class_fields_info_) {} 
  FieldInfo* fields_info_in_instance_;
};

////////////////////////////////////////////////////////////////////////////////
void ___decode_eterm(Serializable& __s, const ETERM* msg) {
  FieldInfo* f = __s.fields_info_in_instance_;
  ETERM* et = NULL;
  
  while (f != NULL) {
    if (ERL_IS_TUPLE(msg)) {
      et = erl_element(f->index_, msg);
    } else if (ERL_IS_LIST(msg)) {
      et = erl_hd(msg);
    }

    if (!et) {
      return;
    }
    
    (*f->decode_func_)(&__s, f->offset_, et);
    
    erl_free_term(et);

    if (ERL_IS_LIST(msg)) {
      msg = erl_tl(msg);
    }
    
    f = f->next_field_info_;
  }
}

/*----------------------------------------------------------------------------*/
template<typename T>
struct Decoder {
  static void decode(void* instance, size_t field_offset, const ETERM* msg) {
    Serializable& nested = *(Serializable*)( ((uint8_t*)instance) + field_offset );
    ___decode_eterm(nested, msg);
  }
};

template<typename T, size_t N>
struct Decoder<__Array<T, N> > {
  static void decode(void* instance, size_t field_offset, const ETERM* msg) {
    cout << "array" << N << endl;
  }
};

/*Erl_Integer ival;*/
template<> struct Decoder<int> {
  static void decode(void* instance, size_t field_offset, const ETERM* msg) {
    *(int*)(((uint8_t*)instance) + field_offset) = ERL_INT_VALUE(msg);
  }
};

/*Erl_Uinteger uival;*/
template<> struct Decoder<unsigned int> {
  static void decode(void* instance, size_t field_offset, const ETERM* msg) {
    *(unsigned int*)(((uint8_t*)instance) + field_offset) = ERL_INT_UVALUE(msg);
  }
};

/*Erl_Uinteger fval;*/
template<> struct Decoder<double> {
  static void decode(void* instance, size_t field_offset, const ETERM* msg) {
    *(double*)(((uint8_t*)instance) + field_offset) = ERL_FLOAT_VALUE(msg);
  }
};

template<>
struct Decoder<const char*> {
  static void decode(void* instance, size_t field_offset, const ETERM* msg) {
    *(const char**)(((uint8_t*)instance) + field_offset) = strdup(ERL_ATOM_PTR(msg));
    //TODO: free memory from strdup
  }
};

/*not support types*/
template<> struct Decoder<float> {  static void decode(void* instance, size_t field_offset, const ETERM* msg); };
template<> struct Decoder<long> {  static void decode(void* instance, size_t field_offset, const ETERM* msg); };
template<> struct Decoder<unsigned long> {  static void decode(void* instance, size_t field_offset, const ETERM* msg); };
template<> struct Decoder<long long> {  static void decode(void* instance, size_t field_offset, const ETERM* msg); };
template<> struct Decoder<unsigned long long> {  static void decode(void* instance, size_t field_offset, const ETERM* msg); };


#if 0
typedef struct _eterm {
  union {
    Erl_Integer    ival;
    Erl_Uinteger   uival; 
    //Erl_LLInteger  llval;
    //Erl_ULLInteger ullval;
    Erl_Float      fval;
    Erl_Atom       aval;
    Erl_Pid        pidval;
    Erl_Port       portval;    
    Erl_Ref        refval;   
    Erl_List       lval;
    Erl_EmptyList  nval;
    Erl_Tuple      tval;
    Erl_Binary     bval;
    Erl_Variable   vval;
    Erl_Function   funcval;
    Erl_Big        bigval;
  } uval;
} ETERM;

#endif
////////////////////////////////////////////////////////////////////////////////
template<typename T, typename Holder, int field_tag>
struct __t {
  __t(const __t&);
  void operator=(const __t&);
  
  __t() {
    if (g_field_index_ != kInvalidFieldIndex && f_.index_ == kInvalidFieldIndex) {
      f_.index_ = g_field_index_++;
      f_.offset_ = reinterpret_cast<long>(this) - reinterpret_cast<long>(g_outmost_this_address);
      f_.decode_func_ = &Decoder<T>::decode;

      cout << "__t:" << (long*)this << " size:" << sizeof(__t<T, Holder, field_tag>)
           << " index: " << (int)f_.index_
           << " offset: " << f_.offset_
           << " decode: " << (long)f_.decode_func_
           << " this field:" << (long)&f_
           << " prev_field:" << (long)g_pp_next_field_info_
           << " next pointer addr:" << (long)&f_.next_field_info_
           << endl;

      *g_pp_next_field_info_ = &f_;      
      g_pp_next_field_info_ = &f_.next_field_info_;
    }
  }

  ~__t() {
    release(_);
  }

  /*---------------------------------------------------------*/
  template<typename X> void release(X& v) {}
  
  void release(const char*& v) {
    if (v != NULL) {
      free((void*)v);
    }
  }

  //TODO: add release routines for other types here, if need.
  /*--------------------------------------------------------*/

  operator T&() {
    return _;
  }
  
  T _;
  static FieldInfo f_;
};
template<typename T, typename Holder, int field_tag> FieldInfo __t<T, Holder, field_tag>::f_ = {0};


////////////////////////////////////////////////////////////////////////////////
struct StartAddressRegister {
  StartAddressRegister() {}
  StartAddressRegister(void *p) {
    if (p != NULL) {
      g_outmost_this_address = p;
      g_field_index_ = 1;
      //cout << "StartAddressRegister: " << g_outmost_this_address << endl;
    }
  }
};

#define ___def_data(_name) \
struct _name : Serializable, StartAddressRegister, FieldsInfo<_name> {                    \
  typedef _name data_type;                                                                \
  _name(ArgForceMetaRegister* a) : StartAddressRegister(this), FieldsInfo<_name>(this) {  \
    g_field_index_ = 0;                                                                   \
    g_pp_next_field_info_ = NULL;                                                         \
  }                                                                                       \
  _name() : Serializable(class_fields_info_) { }

#define ___field_impl(line, ...) __t<__VA_ARGS__, data_type, __##line>
#define ___field(...) ___field_impl(LINE__, __VA_ARGS__)

#define ___end_def_data };

/*----------------------------------------------------------------------------*/
___def_data(DataA)
  ___field(int) ia;
  ___field(int) ib;
___end_def_data;

___def_data(DataB)
  ___field(const char*) x;
  ___field(DataA) y;
___end_def_data;

___def_data(DataC)
  ___field(int) i;
  ___field(unsigned int) ui;
  ___field(double) d;
  ___field(const char*) atom;
___end_def_data;

___def_data(DataD)
  ___field(int) i;
  //___field(__array(int, 4)) ia;
___end_def_data;

////////////////////////////////////////////////////////////////////////////////
TEST(DataD, should_able_to_decode___array_of_int) {
  DataD d;
  ETERM* tuplep = erl_format((char*)"{3, {4, 5, 6, 7} }");

  ___decode_eterm(d, tuplep);

  EXPECT_EQ(3, (int)d.i);
}

TEST(DataC, xxx0) {
  DataC c;
  unsigned int X = INT_MAX;
  X = X + 1;
    
  ETERM* tuplep = erl_format((char*)"{~i, ~i, ~f, ~a}", INT_MAX, X, 9.99, "atom_value");

  ___decode_eterm(c, tuplep);

  EXPECT_EQ(INT_MAX, c.i._);
  EXPECT_EQ(X, c.ui._);
  EXPECT_EQ(9.99, c.d._);
  EXPECT_STREQ("atom_value", c.atom);
  erl_free_term(tuplep);
}

/*----------------------------------------------------------------------------*/
TEST(DataA, xxx0) {
  DataA a;
  ETERM* tuplep = erl_format((char*)"{3, 4}");
  
  ___decode_eterm(a, tuplep);  

  EXPECT_EQ(3, (int)a.ia);
  EXPECT_EQ(4, (int)a.ib);
  erl_free_term(tuplep);
}

TEST(DataA, xxx_test_empty_list) {
  DataA a;
  ETERM* tuplep = erl_format((char*)"[]");
  
  ___decode_eterm(a, tuplep);  

  erl_free_term(tuplep);
}

/*----------------------------------------------------------------------------*/
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




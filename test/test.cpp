#include <stdint.h>
#include <cstring>
#include <iostream>
#include <string>
#include <gtest/gtest.h>
#include <mockcpp/mockcpp.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits.hpp>
#include <boost/array.hpp>
USING_MOCKCPP_NS;
using namespace std;
using ::testing::TestWithParam;
using ::testing::Values;

#include "erl_interface.h"
#include "ei.h"

struct Serializable;
void ___decode_eterm(Serializable& __s, const ETERM* msg);
  
static void* g_outmost_this_address = NULL;
typedef unsigned char FieldIndex;
static FieldIndex g_field_index_;

typedef void (*DecodeFunc)(void* instance, size_t field_offset, const ETERM* msg);

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
    cout << "1set g_pp_next_field_info_ at class register: " << (long)g_pp_next_field_info_ << endl;
  }
  static FieldInfo* class_fields_info_;
};
template<typename T> FieldInfo* FieldsInfo<T>::class_fields_info_ = NULL;

struct Serializable {
  Serializable() {}
  Serializable(FieldInfo* class_fields_info_) : fields_info_in_instance_(class_fields_info_) {} 
  FieldInfo* fields_info_in_instance_;
};

////////////////////////////////////////////////////////////////////////////////
void ___decode_eterm(Serializable& __s, const ETERM* msg) {
  FieldInfo* f = __s.fields_info_in_instance_;
  ETERM* et = NULL;
  
  while (f != NULL) {
    //cout << (long)f << "index: " <<  (long)f->index_ << " offset: " << (long)f->offset_
    //     << " decode_f: " << (long)f->decode_func_ << " next: " << (long)f->next_field_info_ << endl;
    if (ERL_IS_TUPLE(msg)) {
      et = erl_element(f->index_, msg);
    } else if (ERL_IS_LIST(msg)) {
      et = erl_hd(msg);
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
template<typename T, class Enable = void>
struct Decoder {
  static void decode(void* instance, size_t field_offset, const ETERM* msg) {
    cout << "default navie decoder" << endl;
  }
};

template<typename T>
struct Decoder<T, typename boost::enable_if_c<boost::is_base_of<Serializable, T>::value>::type> {
  static void decode(void* instance, size_t field_offset, const ETERM* msg) {
    Serializable& nested = *(Serializable*)( ((uint8_t*)instance) + field_offset );
    ___decode_eterm(nested, msg);
  }
};

template<typename T>
struct Decoder<T, typename boost::enable_if_c<boost::is_same<const char*, T>::value>::type> {
  static void decode(void* instance, size_t field_offset, const ETERM* msg) {
    *(const char**)(((uint8_t*)instance) + field_offset) = strdup(ERL_ATOM_PTR(msg));
    //TODO: free memory from strdup
  }
};

template<typename T>
struct Decoder<T, typename boost::enable_if_c<boost::is_same<int, T>::value>::type> {
  static void decode(void* instance, size_t field_offset, const ETERM* msg) {
    *(int*)(((uint8_t*)instance) + field_offset) = ERL_INT_VALUE(msg);
  }
};

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
      cout << "StartAddressRegister: " << g_outmost_this_address << endl;
    }
  }
};

#define ___def_data(_name) \
struct _name : StartAddressRegister, FieldsInfo<_name>, Serializable {                    \
  typedef _name data_type;                                                                \
  _name(ArgForceMetaRegister* a) : StartAddressRegister(this), FieldsInfo<_name>(this) {  \
    g_field_index_ = 0;                                                                   \
    g_pp_next_field_info_ = NULL;                                                         \
  }                                                                                       \
  _name() : Serializable(class_fields_info_) { }

#define ___field_impl(_type, line) __t<_type, data_type, __##line>
#define ___field(_type) ___field_impl(_type, LINE__)

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

////////////////////////////////////////////////////////////////////////////////
TEST(SingleFieldData, xxx0) {
  DataA a;
  ETERM* tuplep = erl_format((char*)"{3, 4}");
  
  ___decode_eterm(a, tuplep);  

  EXPECT_EQ(3, (int)a.ia);
  EXPECT_EQ(4, (int)a.ib);
  erl_free_term(tuplep);
}

struct Eterm_to_DataB : public TestWithParam<const char*> {
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
                       , Values( "[foo, {3, 4}]"
                       , "[foo, [3, 4]]"
                       , "{foo, {3, 4}}"
                       , "{foo, [3, 4]}"));


////////////////////////////////////////////////////////////////////////////////
/*
struct DataA;
struct DataA : StartAddressRegister, FieldsInfo<DataA>, Serializable {
  typedef DataA data_type;
  DataA(ArgForceMetaRegister* a) : StartAddressRegister(this), FieldsInfo<DataA>(this), Serializable(NULL) {
    g_field_index_ = 0;
    g_pp_next_field_info_ = NULL;
  }
  DataA() : Serializable(class_fields_info_) { }
  
  __t<int, data_type, 1> ia;
  __t<int, data_type, 2> ib;
};

struct DataB;
struct DataB : StartAddressRegister, FieldsInfo<DataB>, Serializable {
  typedef DataB data_type;
  DataB(ArgForceMetaRegister* a) : StartAddressRegister(this), FieldsInfo<DataB>(this), Serializable(NULL) {
    g_field_index_ = 0;
    g_pp_next_field_info_ = NULL;
  }
  DataB() : Serializable(class_fields_info_) { }
  
  __t<const char*, data_type, 1> x;
  __t<DataA, data_type, 1> y;  
};*/


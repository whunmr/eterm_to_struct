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

#include "erl_interface.h"
#include "ei.h"

struct XSerializable;
void ___decode_eterm(XSerializable& __s, const ETERM* msg);
  
static void* g_outmost_this_address = NULL;
typedef unsigned char FieldIndex;
static FieldIndex g_field_index_;

typedef void (*XDecodeFunc)(void* instance, size_t field_offset, const ETERM* msg);

struct XFieldInfo {
  FieldIndex index_;
  uint16_t offset_;
  XDecodeFunc decode_func_;
  XFieldInfo* next_field_info_;
};

static XFieldInfo** g_pp_next_field_info_ = NULL;
static const FieldIndex kFieldIndexStart = 1;
static const FieldIndex kInvalidFieldIndex = 0;

template<typename T>
struct FieldsInfo {
  FieldsInfo() {}
  FieldsInfo(void* p) {
    g_pp_next_field_info_ = &class_fields_info_;
    cout << "1set g_pp_next_field_info_ at class register: " << (long)g_pp_next_field_info_ << endl;
  }
  static XFieldInfo* class_fields_info_;
};
template<typename T> XFieldInfo* FieldsInfo<T>::class_fields_info_ = NULL;

struct XSerializable {
  XSerializable(XFieldInfo* class_fields_info_) : fields_info_in_instance_(class_fields_info_) {} 
  XFieldInfo* fields_info_in_instance_;
};

/*----------------------------------------------------------------------------*/
template<typename T, class Enable = void>
struct XDecoder {
  static void decode(void* instance, size_t field_offset, const ETERM* msg) {
    cout << "default navie decoder" << endl;
  }
};

template<typename T>
struct XDecoder<T, typename boost::enable_if_c<boost::is_base_of<XSerializable, T>::value>::type> {
  static void decode(void* instance, size_t field_offset, const ETERM* msg) {
    XSerializable& nested = *(XSerializable*)( ((uint8_t*)instance) + field_offset );
    ___decode_eterm(nested, msg);
  }
};

template<typename T>
struct XDecoder<T, typename boost::enable_if_c<boost::is_same<const char*, T>::value>::type> {
  static void decode(void* instance, size_t field_offset, const ETERM* msg) {
    *(const char**)(((uint8_t*)instance) + field_offset) = strdup(ERL_ATOM_PTR(msg)); //TODO: free memory from strdup
  }
};

template<typename T>
struct XDecoder<T, typename boost::enable_if_c<boost::is_same<int, T>::value>::type> {
  static void decode(void* instance, size_t field_offset, const ETERM* msg) {
    *(int*)(((uint8_t*)instance) + field_offset) = ERL_INT_VALUE(msg);
  }
};

////////////////////////////////////////////////////////////////////////////////

template<typename T, typename Holder, int field_tag>
struct __t {
  __t() {
    if (g_field_index_ != kInvalidFieldIndex && f_.index_ == kInvalidFieldIndex) {
      f_.index_ = g_field_index_++;
      f_.offset_ = reinterpret_cast<long>(this) - reinterpret_cast<long>(g_outmost_this_address);
      f_.decode_func_ = &XDecoder<T>::decode;

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

  operator T&() {
    return v;
  }
  
  T v;
  static XFieldInfo f_;
};
template<typename T, typename Holder, int field_tag> XFieldInfo __t<T, Holder, field_tag>::f_ = {0};

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

struct ArgForceMetaRegister {} ___ArgForceMetaRegister;

////////////////////////////////////////////////////////////////////////////////

struct DataA;
extern DataA ___DataA_meta_register_instance;
struct DataA : StartAddressRegister, FieldsInfo<DataA>, XSerializable {
  typedef DataA data_type;
  DataA() : XSerializable(class_fields_info_) {}
  DataA(ArgForceMetaRegister* a) : StartAddressRegister(this), FieldsInfo<DataA>(this), XSerializable(NULL) {
    cout << "=============> register Datax: " << this << endl;
    g_field_index_ = 0;
    g_pp_next_field_info_ = NULL;
    cout << "next field: " << (long)class_fields_info_ << endl;
    cout << "=============> register Datax finished." << endl;
  }
  
  __t<int, data_type, 1> ia;
  __t<int, data_type, 2> ib;
};

DataA ___DataA_meta_register_instance(&___ArgForceMetaRegister);


struct DataB;
extern DataB ___DataB_meta_register_instance;
struct DataB : StartAddressRegister, FieldsInfo<DataB>, XSerializable {
  typedef DataB data_type;
  DataB() : XSerializable(class_fields_info_) {}
  DataB(ArgForceMetaRegister* a) : StartAddressRegister(this), FieldsInfo<DataB>(this), XSerializable(NULL) {
    cout << "=============> register Datax: " << this << endl;
    g_field_index_ = 0;
    g_pp_next_field_info_ = NULL;
    cout << "next field: " << (long)class_fields_info_ << endl;
    cout << "=============> register Datax finished." << endl;
  }
  
  __t<const char*, data_type, 1> x;
  __t<DataA, data_type, 1> y;  
};

DataB ___DataB_meta_register_instance(&___ArgForceMetaRegister);

////////////////////////////////////////////////////////////////////////////////
void ___decode_eterm(XSerializable& __s, const ETERM* msg) {
  XFieldInfo* f = __s.fields_info_in_instance_;
  ETERM* et = NULL;
  
  while (f != NULL) {
    cout << (long)f << "index: " <<  (long)f->index_ << " offset: " << (long)f->offset_
         << " decode_f: " << (long)f->decode_func_ << " next: " << (long)f->next_field_info_ << endl;

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

////////////////////////////////////////////////////////////////////////////////
TEST(SingleFieldData, xxx) {
  erl_init(NULL, 0);
  
  DataB d;
  ETERM* tuplep = erl_format((char*)"{foo, {3, 4}}");
  
  ___decode_eterm(d, tuplep);
  
  cout << "value: " << d.x << " , " << d.y.v.ia << " , " << d.y.v.ib << endl;
  erl_free_term(tuplep);
}

TEST(SingleFieldData, xxx2) {
  erl_init(NULL, 0);
  
  DataB d;
  ETERM* tuplep = erl_format((char*)"{foo, [3, 4]}");
  
  ___decode_eterm(d, tuplep);
  
  cout << "value: " << d.x << " , " << d.y.v.ia << " , " << d.y.v.ib << endl;
  erl_free_term(tuplep);
}


TEST(SingleFieldData, xxx3) {
  erl_init(NULL, 0);
  
  DataB d;
  ETERM* tuplep = erl_format((char*)"[foo, [3, 4]]");
  
  ___decode_eterm(d, tuplep);
  
  cout << "value: " << d.x << " , " << d.y.v.ia << " , " << d.y.v.ib << endl;
  erl_free_term(tuplep);
}


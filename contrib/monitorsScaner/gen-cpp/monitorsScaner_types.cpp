/**
 * Autogenerated by Thrift Compiler (0.9.1)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#include "monitorsScaner_types.h"

#include <algorithm>

namespace monitorsScaner {

int _kNetworkTypeValues[] = {
  NetworkType::TESTNET,
  NetworkType::MAIN
};
const char* _kNetworkTypeNames[] = {
  "TESTNET",
  "MAIN"
};
const std::map<int, const char*> _NetworkType_VALUES_TO_NAMES(::apache::thrift::TEnumIterator(2, _kNetworkTypeValues, _kNetworkTypeNames), ::apache::thrift::TEnumIterator(-1, NULL, NULL));

int _kInfoValues[] = {
  Info::TRACKERS_INFO,
  Info::MONITORS_INFO
};
const char* _kInfoNames[] = {
  "TRACKERS_INFO",
  "MONITORS_INFO"
};
const std::map<int, const char*> _Info_VALUES_TO_NAMES(::apache::thrift::TEnumIterator(2, _kInfoValues, _kInfoNames), ::apache::thrift::TEnumIterator(-1, NULL, NULL));

const char* InfoRequest::ascii_fingerprint = "320A4E0A7D1C541E521D7F65F2108F77";
const uint8_t InfoRequest::binary_fingerprint[16] = {0x32,0x0A,0x4E,0x0A,0x7D,0x1C,0x54,0x1E,0x52,0x1D,0x7F,0x65,0xF2,0x10,0x8F,0x77};

uint32_t InfoRequest::read(::apache::thrift::protocol::TProtocol* iprot) {

  uint32_t xfer = 0;
  std::string fname;
  ::apache::thrift::protocol::TType ftype;
  int16_t fid;

  xfer += iprot->readStructBegin(fname);

  using ::apache::thrift::protocol::TProtocolException;


  while (true)
  {
    xfer += iprot->readFieldBegin(fname, ftype, fid);
    if (ftype == ::apache::thrift::protocol::T_STOP) {
      break;
    }
    switch (fid)
    {
      case 1:
        if (ftype == ::apache::thrift::protocol::T_I32) {
          int32_t ecast0;
          xfer += iprot->readI32(ecast0);
          this->networkType = (NetworkType::type)ecast0;
          this->__isset.networkType = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 2:
        if (ftype == ::apache::thrift::protocol::T_I32) {
          int32_t ecast1;
          xfer += iprot->readI32(ecast1);
          this->info = (Info::type)ecast1;
          this->__isset.info = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 3:
        if (ftype == ::apache::thrift::protocol::T_STRING) {
          xfer += iprot->readString(this->key);
          this->__isset.key = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      default:
        xfer += iprot->skip(ftype);
        break;
    }
    xfer += iprot->readFieldEnd();
  }

  xfer += iprot->readStructEnd();

  return xfer;
}

uint32_t InfoRequest::write(::apache::thrift::protocol::TProtocol* oprot) const {
  uint32_t xfer = 0;
  xfer += oprot->writeStructBegin("InfoRequest");

  xfer += oprot->writeFieldBegin("networkType", ::apache::thrift::protocol::T_I32, 1);
  xfer += oprot->writeI32((int32_t)this->networkType);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("info", ::apache::thrift::protocol::T_I32, 2);
  xfer += oprot->writeI32((int32_t)this->info);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("key", ::apache::thrift::protocol::T_STRING, 3);
  xfer += oprot->writeString(this->key);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldStop();
  xfer += oprot->writeStructEnd();
  return xfer;
}

void swap(InfoRequest &a, InfoRequest &b) {
  using ::std::swap;
  swap(a.networkType, b.networkType);
  swap(a.info, b.info);
  swap(a.key, b.key);
  swap(a.__isset, b.__isset);
}

const char* Exception::ascii_fingerprint = "EFB929595D312AC8F305D5A794CFEDA1";
const uint8_t Exception::binary_fingerprint[16] = {0xEF,0xB9,0x29,0x59,0x5D,0x31,0x2A,0xC8,0xF3,0x05,0xD5,0xA7,0x94,0xCF,0xED,0xA1};

uint32_t Exception::read(::apache::thrift::protocol::TProtocol* iprot) {

  uint32_t xfer = 0;
  std::string fname;
  ::apache::thrift::protocol::TType ftype;
  int16_t fid;

  xfer += iprot->readStructBegin(fname);

  using ::apache::thrift::protocol::TProtocolException;


  while (true)
  {
    xfer += iprot->readFieldBegin(fname, ftype, fid);
    if (ftype == ::apache::thrift::protocol::T_STOP) {
      break;
    }
    switch (fid)
    {
      case 1:
        if (ftype == ::apache::thrift::protocol::T_STRING) {
          xfer += iprot->readString(this->msg);
          this->__isset.msg = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      default:
        xfer += iprot->skip(ftype);
        break;
    }
    xfer += iprot->readFieldEnd();
  }

  xfer += iprot->readStructEnd();

  return xfer;
}

uint32_t Exception::write(::apache::thrift::protocol::TProtocol* oprot) const {
  uint32_t xfer = 0;
  xfer += oprot->writeStructBegin("Exception");

  xfer += oprot->writeFieldBegin("msg", ::apache::thrift::protocol::T_STRING, 1);
  xfer += oprot->writeString(this->msg);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldStop();
  xfer += oprot->writeStructEnd();
  return xfer;
}

void swap(Exception &a, Exception &b) {
  using ::std::swap;
  swap(a.msg, b.msg);
  swap(a.__isset, b.__isset);
}

const char* Data::ascii_fingerprint = "4FB913456A1C0826B937721827BFAF5F";
const uint8_t Data::binary_fingerprint[16] = {0x4F,0xB9,0x13,0x45,0x6A,0x1C,0x08,0x26,0xB9,0x37,0x72,0x18,0x27,0xBF,0xAF,0x5F};

uint32_t Data::read(::apache::thrift::protocol::TProtocol* iprot) {

  uint32_t xfer = 0;
  std::string fname;
  ::apache::thrift::protocol::TType ftype;
  int16_t fid;

  xfer += iprot->readStructBegin(fname);

  using ::apache::thrift::protocol::TProtocolException;


  while (true)
  {
    xfer += iprot->readFieldBegin(fname, ftype, fid);
    if (ftype == ::apache::thrift::protocol::T_STOP) {
      break;
    }
    switch (fid)
    {
      case 1:
        if (ftype == ::apache::thrift::protocol::T_I64) {
          xfer += iprot->readI64(this->rows);
          this->__isset.rows = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 2:
        if (ftype == ::apache::thrift::protocol::T_I64) {
          xfer += iprot->readI64(this->cols);
          this->__isset.cols = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 3:
        if (ftype == ::apache::thrift::protocol::T_LIST) {
          {
            this->data.clear();
            uint32_t _size2;
            ::apache::thrift::protocol::TType _etype5;
            xfer += iprot->readListBegin(_etype5, _size2);
            this->data.resize(_size2);
            uint32_t _i6;
            for (_i6 = 0; _i6 < _size2; ++_i6)
            {
              xfer += iprot->readString(this->data[_i6]);
            }
            xfer += iprot->readListEnd();
          }
          this->__isset.data = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      default:
        xfer += iprot->skip(ftype);
        break;
    }
    xfer += iprot->readFieldEnd();
  }

  xfer += iprot->readStructEnd();

  return xfer;
}

uint32_t Data::write(::apache::thrift::protocol::TProtocol* oprot) const {
  uint32_t xfer = 0;
  xfer += oprot->writeStructBegin("Data");

  xfer += oprot->writeFieldBegin("rows", ::apache::thrift::protocol::T_I64, 1);
  xfer += oprot->writeI64(this->rows);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("cols", ::apache::thrift::protocol::T_I64, 2);
  xfer += oprot->writeI64(this->cols);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("data", ::apache::thrift::protocol::T_LIST, 3);
  {
    xfer += oprot->writeListBegin(::apache::thrift::protocol::T_STRING, static_cast<uint32_t>(this->data.size()));
    std::vector<std::string> ::const_iterator _iter7;
    for (_iter7 = this->data.begin(); _iter7 != this->data.end(); ++_iter7)
    {
      xfer += oprot->writeString((*_iter7));
    }
    xfer += oprot->writeListEnd();
  }
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldStop();
  xfer += oprot->writeStructEnd();
  return xfer;
}

void swap(Data &a, Data &b) {
  using ::std::swap;
  swap(a.rows, b.rows);
  swap(a.cols, b.cols);
  swap(a.data, b.data);
  swap(a.__isset, b.__isset);
}

} // namespace
// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: Rpc.proto

#include "Rpc.pb.h"

#include <algorithm>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/wire_format_lite.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>
// @@protoc_insertion_point(includes)
#include <google/protobuf/port_def.inc>
extern PROTOBUF_INTERNAL_EXPORT_google_2fprotobuf_2fany_2eproto ::PROTOBUF_NAMESPACE_ID::internal::SCCInfo<0> scc_info_Any_google_2fprotobuf_2fany_2eproto;
namespace Mira {
namespace Rpc {
class RpcMessageDefaultTypeInternal {
 public:
  ::PROTOBUF_NAMESPACE_ID::internal::ExplicitlyConstructed<RpcMessage> _instance;
} _RpcMessage_default_instance_;
}  // namespace Rpc
}  // namespace Mira
static void InitDefaultsscc_info_RpcMessage_Rpc_2eproto() {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  {
    void* ptr = &::Mira::Rpc::_RpcMessage_default_instance_;
    new (ptr) ::Mira::Rpc::RpcMessage();
    ::PROTOBUF_NAMESPACE_ID::internal::OnShutdownDestroyMessage(ptr);
  }
}

::PROTOBUF_NAMESPACE_ID::internal::SCCInfo<1> scc_info_RpcMessage_Rpc_2eproto =
    {{ATOMIC_VAR_INIT(::PROTOBUF_NAMESPACE_ID::internal::SCCInfoBase::kUninitialized), 1, 0, InitDefaultsscc_info_RpcMessage_Rpc_2eproto}, {
      &scc_info_Any_google_2fprotobuf_2fany_2eproto.base,}};

static ::PROTOBUF_NAMESPACE_ID::Metadata file_level_metadata_Rpc_2eproto[1];
static const ::PROTOBUF_NAMESPACE_ID::EnumDescriptor* file_level_enum_descriptors_Rpc_2eproto[2];
static constexpr ::PROTOBUF_NAMESPACE_ID::ServiceDescriptor const** file_level_service_descriptors_Rpc_2eproto = nullptr;

const ::PROTOBUF_NAMESPACE_ID::uint32 TableStruct_Rpc_2eproto::offsets[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
  ~0u,  // no _has_bits_
  PROTOBUF_FIELD_OFFSET(::Mira::Rpc::RpcMessage, _internal_metadata_),
  ~0u,  // no _extensions_
  ~0u,  // no _oneof_case_
  ~0u,  // no _weak_field_map_
  PROTOBUF_FIELD_OFFSET(::Mira::Rpc::RpcMessage, magic_),
  PROTOBUF_FIELD_OFFSET(::Mira::Rpc::RpcMessage, category_),
  PROTOBUF_FIELD_OFFSET(::Mira::Rpc::RpcMessage, inner_message_),
  PROTOBUF_FIELD_OFFSET(::Mira::Rpc::RpcMessage, error_),
};
static const ::PROTOBUF_NAMESPACE_ID::internal::MigrationSchema schemas[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
  { 0, -1, sizeof(::Mira::Rpc::RpcMessage)},
};

static ::PROTOBUF_NAMESPACE_ID::Message const * const file_default_instances[] = {
  reinterpret_cast<const ::PROTOBUF_NAMESPACE_ID::Message*>(&::Mira::Rpc::_RpcMessage_default_instance_),
};

const char descriptor_table_protodef_Rpc_2eproto[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) =
  "\n\tRpc.proto\022\010Mira.Rpc\032\031google/protobuf/a"
  "ny.proto\"\315\002\n\nRpcMessage\022)\n\005magic\030\001 \001(\0162\032"
  ".Mira.Rpc.RpcMessage.Magic\0222\n\010category\030\002"
  " \001(\0162 .Mira.Rpc.RpcMessage.RpcCategory\022+"
  "\n\rinner_message\030\003 \001(\0132\024.google.protobuf."
  "Any\022\r\n\005error\030\005 \001(\005\">\n\005Magic\022\014\n\010NO_MAGIC\020"
  "\000\022\006\n\002V1\020\001\022\006\n\002V2\020\002\022\006\n\002V3\020\003\022\017\n\013MAGIC_COUNT"
  "\020\004\"d\n\013RpcCategory\022\017\n\013NO_CATEGORY\020\000\022\n\n\006SY"
  "STEM\020\001\022\007\n\003LOG\020\002\022\t\n\005DEBUG\020\003\022\010\n\004FILE\020\004\022\013\n\007"
  "COMMAND\020\005\022\r\n\tRPC_COUNT\020\006b\006proto3"
  ;
static const ::PROTOBUF_NAMESPACE_ID::internal::DescriptorTable*const descriptor_table_Rpc_2eproto_deps[1] = {
  &::descriptor_table_google_2fprotobuf_2fany_2eproto,
};
static ::PROTOBUF_NAMESPACE_ID::internal::SCCInfoBase*const descriptor_table_Rpc_2eproto_sccs[1] = {
  &scc_info_RpcMessage_Rpc_2eproto.base,
};
static ::PROTOBUF_NAMESPACE_ID::internal::once_flag descriptor_table_Rpc_2eproto_once;
const ::PROTOBUF_NAMESPACE_ID::internal::DescriptorTable descriptor_table_Rpc_2eproto = {
  false, false, descriptor_table_protodef_Rpc_2eproto, "Rpc.proto", 392,
  &descriptor_table_Rpc_2eproto_once, descriptor_table_Rpc_2eproto_sccs, descriptor_table_Rpc_2eproto_deps, 1, 1,
  schemas, file_default_instances, TableStruct_Rpc_2eproto::offsets,
  file_level_metadata_Rpc_2eproto, 1, file_level_enum_descriptors_Rpc_2eproto, file_level_service_descriptors_Rpc_2eproto,
};

// Force running AddDescriptors() at dynamic initialization time.
static bool dynamic_init_dummy_Rpc_2eproto = (static_cast<void>(::PROTOBUF_NAMESPACE_ID::internal::AddDescriptors(&descriptor_table_Rpc_2eproto)), true);
namespace Mira {
namespace Rpc {
const ::PROTOBUF_NAMESPACE_ID::EnumDescriptor* RpcMessage_Magic_descriptor() {
  ::PROTOBUF_NAMESPACE_ID::internal::AssignDescriptors(&descriptor_table_Rpc_2eproto);
  return file_level_enum_descriptors_Rpc_2eproto[0];
}
bool RpcMessage_Magic_IsValid(int value) {
  switch (value) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
      return true;
    default:
      return false;
  }
}

#if (__cplusplus < 201703) && (!defined(_MSC_VER) || _MSC_VER >= 1900)
constexpr RpcMessage_Magic RpcMessage::NO_MAGIC;
constexpr RpcMessage_Magic RpcMessage::V1;
constexpr RpcMessage_Magic RpcMessage::V2;
constexpr RpcMessage_Magic RpcMessage::V3;
constexpr RpcMessage_Magic RpcMessage::MAGIC_COUNT;
constexpr RpcMessage_Magic RpcMessage::Magic_MIN;
constexpr RpcMessage_Magic RpcMessage::Magic_MAX;
constexpr int RpcMessage::Magic_ARRAYSIZE;
#endif  // (__cplusplus < 201703) && (!defined(_MSC_VER) || _MSC_VER >= 1900)
const ::PROTOBUF_NAMESPACE_ID::EnumDescriptor* RpcMessage_RpcCategory_descriptor() {
  ::PROTOBUF_NAMESPACE_ID::internal::AssignDescriptors(&descriptor_table_Rpc_2eproto);
  return file_level_enum_descriptors_Rpc_2eproto[1];
}
bool RpcMessage_RpcCategory_IsValid(int value) {
  switch (value) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
      return true;
    default:
      return false;
  }
}

#if (__cplusplus < 201703) && (!defined(_MSC_VER) || _MSC_VER >= 1900)
constexpr RpcMessage_RpcCategory RpcMessage::NO_CATEGORY;
constexpr RpcMessage_RpcCategory RpcMessage::SYSTEM;
constexpr RpcMessage_RpcCategory RpcMessage::LOG;
constexpr RpcMessage_RpcCategory RpcMessage::DEBUG;
constexpr RpcMessage_RpcCategory RpcMessage::FILE;
constexpr RpcMessage_RpcCategory RpcMessage::COMMAND;
constexpr RpcMessage_RpcCategory RpcMessage::RPC_COUNT;
constexpr RpcMessage_RpcCategory RpcMessage::RpcCategory_MIN;
constexpr RpcMessage_RpcCategory RpcMessage::RpcCategory_MAX;
constexpr int RpcMessage::RpcCategory_ARRAYSIZE;
#endif  // (__cplusplus < 201703) && (!defined(_MSC_VER) || _MSC_VER >= 1900)

// ===================================================================

class RpcMessage::_Internal {
 public:
  static const PROTOBUF_NAMESPACE_ID::Any& inner_message(const RpcMessage* msg);
};

const PROTOBUF_NAMESPACE_ID::Any&
RpcMessage::_Internal::inner_message(const RpcMessage* msg) {
  return *msg->inner_message_;
}
void RpcMessage::clear_inner_message() {
  if (GetArena() == nullptr && inner_message_ != nullptr) {
    delete inner_message_;
  }
  inner_message_ = nullptr;
}
RpcMessage::RpcMessage(::PROTOBUF_NAMESPACE_ID::Arena* arena)
  : ::PROTOBUF_NAMESPACE_ID::Message(arena) {
  SharedCtor();
  RegisterArenaDtor(arena);
  // @@protoc_insertion_point(arena_constructor:Mira.Rpc.RpcMessage)
}
RpcMessage::RpcMessage(const RpcMessage& from)
  : ::PROTOBUF_NAMESPACE_ID::Message() {
  _internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
  if (from._internal_has_inner_message()) {
    inner_message_ = new PROTOBUF_NAMESPACE_ID::Any(*from.inner_message_);
  } else {
    inner_message_ = nullptr;
  }
  ::memcpy(&magic_, &from.magic_,
    static_cast<size_t>(reinterpret_cast<char*>(&error_) -
    reinterpret_cast<char*>(&magic_)) + sizeof(error_));
  // @@protoc_insertion_point(copy_constructor:Mira.Rpc.RpcMessage)
}

void RpcMessage::SharedCtor() {
  ::PROTOBUF_NAMESPACE_ID::internal::InitSCC(&scc_info_RpcMessage_Rpc_2eproto.base);
  ::memset(reinterpret_cast<char*>(this) + static_cast<size_t>(
      reinterpret_cast<char*>(&inner_message_) - reinterpret_cast<char*>(this)),
      0, static_cast<size_t>(reinterpret_cast<char*>(&error_) -
      reinterpret_cast<char*>(&inner_message_)) + sizeof(error_));
}

RpcMessage::~RpcMessage() {
  // @@protoc_insertion_point(destructor:Mira.Rpc.RpcMessage)
  SharedDtor();
  _internal_metadata_.Delete<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>();
}

void RpcMessage::SharedDtor() {
  GOOGLE_DCHECK(GetArena() == nullptr);
  if (this != internal_default_instance()) delete inner_message_;
}

void RpcMessage::ArenaDtor(void* object) {
  RpcMessage* _this = reinterpret_cast< RpcMessage* >(object);
  (void)_this;
}
void RpcMessage::RegisterArenaDtor(::PROTOBUF_NAMESPACE_ID::Arena*) {
}
void RpcMessage::SetCachedSize(int size) const {
  _cached_size_.Set(size);
}
const RpcMessage& RpcMessage::default_instance() {
  ::PROTOBUF_NAMESPACE_ID::internal::InitSCC(&::scc_info_RpcMessage_Rpc_2eproto.base);
  return *internal_default_instance();
}


void RpcMessage::Clear() {
// @@protoc_insertion_point(message_clear_start:Mira.Rpc.RpcMessage)
  ::PROTOBUF_NAMESPACE_ID::uint32 cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  if (GetArena() == nullptr && inner_message_ != nullptr) {
    delete inner_message_;
  }
  inner_message_ = nullptr;
  ::memset(&magic_, 0, static_cast<size_t>(
      reinterpret_cast<char*>(&error_) -
      reinterpret_cast<char*>(&magic_)) + sizeof(error_));
  _internal_metadata_.Clear<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>();
}

const char* RpcMessage::_InternalParse(const char* ptr, ::PROTOBUF_NAMESPACE_ID::internal::ParseContext* ctx) {
#define CHK_(x) if (PROTOBUF_PREDICT_FALSE(!(x))) goto failure
  while (!ctx->Done(&ptr)) {
    ::PROTOBUF_NAMESPACE_ID::uint32 tag;
    ptr = ::PROTOBUF_NAMESPACE_ID::internal::ReadTag(ptr, &tag);
    CHK_(ptr);
    switch (tag >> 3) {
      // .Mira.Rpc.RpcMessage.Magic magic = 1;
      case 1:
        if (PROTOBUF_PREDICT_TRUE(static_cast<::PROTOBUF_NAMESPACE_ID::uint8>(tag) == 8)) {
          ::PROTOBUF_NAMESPACE_ID::uint64 val = ::PROTOBUF_NAMESPACE_ID::internal::ReadVarint64(&ptr);
          CHK_(ptr);
          _internal_set_magic(static_cast<::Mira::Rpc::RpcMessage_Magic>(val));
        } else goto handle_unusual;
        continue;
      // .Mira.Rpc.RpcMessage.RpcCategory category = 2;
      case 2:
        if (PROTOBUF_PREDICT_TRUE(static_cast<::PROTOBUF_NAMESPACE_ID::uint8>(tag) == 16)) {
          ::PROTOBUF_NAMESPACE_ID::uint64 val = ::PROTOBUF_NAMESPACE_ID::internal::ReadVarint64(&ptr);
          CHK_(ptr);
          _internal_set_category(static_cast<::Mira::Rpc::RpcMessage_RpcCategory>(val));
        } else goto handle_unusual;
        continue;
      // .google.protobuf.Any inner_message = 3;
      case 3:
        if (PROTOBUF_PREDICT_TRUE(static_cast<::PROTOBUF_NAMESPACE_ID::uint8>(tag) == 26)) {
          ptr = ctx->ParseMessage(_internal_mutable_inner_message(), ptr);
          CHK_(ptr);
        } else goto handle_unusual;
        continue;
      // int32 error = 5;
      case 5:
        if (PROTOBUF_PREDICT_TRUE(static_cast<::PROTOBUF_NAMESPACE_ID::uint8>(tag) == 40)) {
          error_ = ::PROTOBUF_NAMESPACE_ID::internal::ReadVarint64(&ptr);
          CHK_(ptr);
        } else goto handle_unusual;
        continue;
      default: {
      handle_unusual:
        if ((tag & 7) == 4 || tag == 0) {
          ctx->SetLastTag(tag);
          goto success;
        }
        ptr = UnknownFieldParse(tag,
            _internal_metadata_.mutable_unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(),
            ptr, ctx);
        CHK_(ptr != nullptr);
        continue;
      }
    }  // switch
  }  // while
success:
  return ptr;
failure:
  ptr = nullptr;
  goto success;
#undef CHK_
}

::PROTOBUF_NAMESPACE_ID::uint8* RpcMessage::_InternalSerialize(
    ::PROTOBUF_NAMESPACE_ID::uint8* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const {
  // @@protoc_insertion_point(serialize_to_array_start:Mira.Rpc.RpcMessage)
  ::PROTOBUF_NAMESPACE_ID::uint32 cached_has_bits = 0;
  (void) cached_has_bits;

  // .Mira.Rpc.RpcMessage.Magic magic = 1;
  if (this->magic() != 0) {
    target = stream->EnsureSpace(target);
    target = ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::WriteEnumToArray(
      1, this->_internal_magic(), target);
  }

  // .Mira.Rpc.RpcMessage.RpcCategory category = 2;
  if (this->category() != 0) {
    target = stream->EnsureSpace(target);
    target = ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::WriteEnumToArray(
      2, this->_internal_category(), target);
  }

  // .google.protobuf.Any inner_message = 3;
  if (this->has_inner_message()) {
    target = stream->EnsureSpace(target);
    target = ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::
      InternalWriteMessage(
        3, _Internal::inner_message(this), target, stream);
  }

  // int32 error = 5;
  if (this->error() != 0) {
    target = stream->EnsureSpace(target);
    target = ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::WriteInt32ToArray(5, this->_internal_error(), target);
  }

  if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields())) {
    target = ::PROTOBUF_NAMESPACE_ID::internal::WireFormat::InternalSerializeUnknownFieldsToArray(
        _internal_metadata_.unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(::PROTOBUF_NAMESPACE_ID::UnknownFieldSet::default_instance), target, stream);
  }
  // @@protoc_insertion_point(serialize_to_array_end:Mira.Rpc.RpcMessage)
  return target;
}

size_t RpcMessage::ByteSizeLong() const {
// @@protoc_insertion_point(message_byte_size_start:Mira.Rpc.RpcMessage)
  size_t total_size = 0;

  ::PROTOBUF_NAMESPACE_ID::uint32 cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  // .google.protobuf.Any inner_message = 3;
  if (this->has_inner_message()) {
    total_size += 1 +
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::MessageSize(
        *inner_message_);
  }

  // .Mira.Rpc.RpcMessage.Magic magic = 1;
  if (this->magic() != 0) {
    total_size += 1 +
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::EnumSize(this->_internal_magic());
  }

  // .Mira.Rpc.RpcMessage.RpcCategory category = 2;
  if (this->category() != 0) {
    total_size += 1 +
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::EnumSize(this->_internal_category());
  }

  // int32 error = 5;
  if (this->error() != 0) {
    total_size += 1 +
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::Int32Size(
        this->_internal_error());
  }

  if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields())) {
    return ::PROTOBUF_NAMESPACE_ID::internal::ComputeUnknownFieldsSize(
        _internal_metadata_, total_size, &_cached_size_);
  }
  int cached_size = ::PROTOBUF_NAMESPACE_ID::internal::ToCachedSize(total_size);
  SetCachedSize(cached_size);
  return total_size;
}

void RpcMessage::MergeFrom(const ::PROTOBUF_NAMESPACE_ID::Message& from) {
// @@protoc_insertion_point(generalized_merge_from_start:Mira.Rpc.RpcMessage)
  GOOGLE_DCHECK_NE(&from, this);
  const RpcMessage* source =
      ::PROTOBUF_NAMESPACE_ID::DynamicCastToGenerated<RpcMessage>(
          &from);
  if (source == nullptr) {
  // @@protoc_insertion_point(generalized_merge_from_cast_fail:Mira.Rpc.RpcMessage)
    ::PROTOBUF_NAMESPACE_ID::internal::ReflectionOps::Merge(from, this);
  } else {
  // @@protoc_insertion_point(generalized_merge_from_cast_success:Mira.Rpc.RpcMessage)
    MergeFrom(*source);
  }
}

void RpcMessage::MergeFrom(const RpcMessage& from) {
// @@protoc_insertion_point(class_specific_merge_from_start:Mira.Rpc.RpcMessage)
  GOOGLE_DCHECK_NE(&from, this);
  _internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
  ::PROTOBUF_NAMESPACE_ID::uint32 cached_has_bits = 0;
  (void) cached_has_bits;

  if (from.has_inner_message()) {
    _internal_mutable_inner_message()->PROTOBUF_NAMESPACE_ID::Any::MergeFrom(from._internal_inner_message());
  }
  if (from.magic() != 0) {
    _internal_set_magic(from._internal_magic());
  }
  if (from.category() != 0) {
    _internal_set_category(from._internal_category());
  }
  if (from.error() != 0) {
    _internal_set_error(from._internal_error());
  }
}

void RpcMessage::CopyFrom(const ::PROTOBUF_NAMESPACE_ID::Message& from) {
// @@protoc_insertion_point(generalized_copy_from_start:Mira.Rpc.RpcMessage)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

void RpcMessage::CopyFrom(const RpcMessage& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:Mira.Rpc.RpcMessage)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool RpcMessage::IsInitialized() const {
  return true;
}

void RpcMessage::InternalSwap(RpcMessage* other) {
  using std::swap;
  _internal_metadata_.Swap<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(&other->_internal_metadata_);
  ::PROTOBUF_NAMESPACE_ID::internal::memswap<
      PROTOBUF_FIELD_OFFSET(RpcMessage, error_)
      + sizeof(RpcMessage::error_)
      - PROTOBUF_FIELD_OFFSET(RpcMessage, inner_message_)>(
          reinterpret_cast<char*>(&inner_message_),
          reinterpret_cast<char*>(&other->inner_message_));
}

::PROTOBUF_NAMESPACE_ID::Metadata RpcMessage::GetMetadata() const {
  return GetMetadataStatic();
}


// @@protoc_insertion_point(namespace_scope)
}  // namespace Rpc
}  // namespace Mira
PROTOBUF_NAMESPACE_OPEN
template<> PROTOBUF_NOINLINE ::Mira::Rpc::RpcMessage* Arena::CreateMaybeMessage< ::Mira::Rpc::RpcMessage >(Arena* arena) {
  return Arena::CreateMessageInternal< ::Mira::Rpc::RpcMessage >(arena);
}
PROTOBUF_NAMESPACE_CLOSE

// @@protoc_insertion_point(global_scope)
#include <google/protobuf/port_undef.inc>

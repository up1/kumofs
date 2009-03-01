#ifndef PROTOCOL_H__
#define PROTOCOL_H__

#include "logic/hash.h"
#include "storage/interface.h"
#include "rpc/protocol.h"
#include "rpc/message.h"

namespace kumo {

namespace protocol {

using namespace rpc::protocol;

const static rpc::role_type MANAGER = 0;
const static rpc::role_type SERVER  = 1;


enum message_type {
	// Manager -> Server
	// Server -> Manager
	// Manager -> Manager
	KeepAlive				= 0,

	// Client -> Manager
	HashSpaceRequest		= 1,

	// Manager -> Server
	ReplaceCopyStart		= 16,
	ReplaceDeleteStart		= 17,
	CreateBackup            = 18,

	// Server -> Manager
	ReplaceCopyEnd			= 32,
	ReplaceDeleteEnd		= 33,
	WHashSpaceRequest		= 34,
	RHashSpaceRequest		= 35,

	// Manager -> Client
	HashSpacePush			= 48,

	// Server -> Server
	ReplicateSet			= 64,
	ReplicateDelete			= 65,
	ReplaceOffer			= 66,

	// Manager -> Manager
	ReplaceElection			= 80,

	// Manager -> Manager
	// Manager -> Server
	HashSpaceSync			= 81,

	// Client -> Server
	Get						= 96,
	Set						= 97,
	Delete					= 98,

	GetStatus				= 112,
	SetConfig				= 113,
};

enum status_type {
	STAT_PID			= 0,
	STAT_UPTIME			= 1,
	STAT_TIME			= 2,
	STAT_VERSION		= 3,
	STAT_CMD_GET		= 4,
	STAT_CMD_SET		= 5,
	STAT_CMD_DELETE		= 6,
	STAT_DB_ITEMS		= 7,
};

namespace type {
	using msgpack::define;
	using msgpack::type::tuple;
	using msgpack::type::raw_ref;
	using msgpack::type_error;
	typedef HashSpace::Seed HSSeed;


	struct DBKey {
		DBKey() {}

		DBKey(const char* key, size_t keylen, uint64_t hash) :
			m_keylen(keylen), m_key(key), m_hash(hash) {}

		DBKey(const char* raw_key, size_t raw_keylen)
		{
			msgpack_unpack(raw_ref(raw_key, raw_keylen));
		}

		const char* data() const		{ return m_key; }
		size_t size() const				{ return m_keylen; }
		uint64_t hash() const			{ return m_hash; }

		// these functions are available only when deserialized
		const char* raw_data() const	{ return m_key - Storage::KEY_META_SIZE; }
		size_t raw_size() const			{ return m_keylen + Storage::KEY_META_SIZE; }

		template <typename Packer>
		void msgpack_pack(Packer& pk) const
		{
			char metabuf[Storage::KEY_META_SIZE];
			Storage::hash_to(m_hash, metabuf);
			pk.pack_raw(m_keylen + Storage::KEY_META_SIZE);
			pk.pack_raw_body(metabuf, Storage::KEY_META_SIZE);
			pk.pack_raw_body(m_key, m_keylen);
		}

		void msgpack_unpack(raw_ref o)
		{
			if(o.size < Storage::KEY_META_SIZE) {
				throw type_error();
			}
			m_keylen = o.size - Storage::KEY_META_SIZE;
			m_hash = Storage::hash_of(o.ptr);
			m_key = o.ptr + Storage::KEY_META_SIZE;
		}

	private:
		size_t m_keylen;
		const char* m_key;
		uint64_t m_hash;
	};

	struct DBValue {
		DBValue() : m_clocktime(0) {}

		DBValue(const char* val, size_t vallen, uint64_t meta) :
			m_vallen(vallen), m_val(val), m_clocktime(0), m_meta(meta) {}

		DBValue(const char* raw_val, size_t raw_vallen) :
			m_clocktime(0)
		{
			msgpack_unpack(raw_ref(raw_val, raw_vallen));
		}

		const char* data() const		{ return m_val; }
		size_t size() const				{ return m_vallen; }
		ClockTime clocktime() const		{ return m_clocktime; }
		uint64_t meta() const			{ return m_meta; }

		// these functions are available only when deserialized
		const char* raw_data() const	{ return m_val - Storage::VALUE_META_SIZE; }
		size_t raw_size() const			{ return m_vallen + Storage::VALUE_META_SIZE; }
		void raw_set_clocktime(ClockTime clocktime)
		{
			m_clocktime = clocktime;
			Storage::clocktime_to(clocktime, const_cast<char*>(raw_data()));
		}

		template <typename Packer>
		void msgpack_pack(Packer& pk) const
		{
			char metabuf[Storage::VALUE_META_SIZE];
			Storage::clocktime_to(m_clocktime, metabuf);
			Storage::meta_to(m_meta, metabuf);
			pk.pack_raw(m_vallen + Storage::VALUE_META_SIZE);
			pk.pack_raw_body(metabuf, Storage::VALUE_META_SIZE);
			pk.pack_raw_body(m_val, m_vallen);
		}

		void msgpack_unpack(raw_ref o)
		{
			if(o.size < Storage::VALUE_META_SIZE) {
				throw type_error();
			}
			m_clocktime = Storage::clocktime_of(o.ptr);
			m_meta      = Storage::meta_of(o.ptr);
			m_vallen    = o.size - Storage::VALUE_META_SIZE;
			m_val       = o.ptr  + Storage::VALUE_META_SIZE;
		}

	private:
		size_t m_vallen;
		const char* m_val;
		ClockTime m_clocktime;
		uint64_t m_meta;
	};


	struct KeepAlive : define< tuple<uint32_t> > {
		KeepAlive() {}
		KeepAlive(uint32_t clock) :
			define_type(msgpack_type( clock )) {}
		uint32_t clock() const { return get<0>(); }
		// ok: UNDEFINED
	};

	struct HashSpaceRequest : define< tuple<> > {
		HashSpaceRequest() {}
		// success: HashSpacePush
	};

	struct WHashSpaceRequest : define< tuple<> > {
		WHashSpaceRequest() {}
		// success: hash_space:tuple<array<raw_ref>,uint64_t>
	};

	struct RHashSpaceRequest : define< tuple<> > {
		RHashSpaceRequest() {}
		// success: hash_space:tuple<array<raw_ref>,uint64_t>
	};

	struct ReplaceCopyStart : define< tuple<HSSeed, uint32_t> > {
		ReplaceCopyStart() {}
		ReplaceCopyStart(HSSeed& hsseed, uint32_t clock) :
			define_type(msgpack_type( hsseed, clock )) {}
		HSSeed& hsseed()				{ return get<0>(); }
		uint32_t clock() const			{ return get<1>(); }
		// accepted: true
	};

	struct ReplaceDeleteStart : define< tuple<HSSeed, uint32_t> > {
		ReplaceDeleteStart() {}
		ReplaceDeleteStart(HSSeed& hsseed, uint32_t clock) :
			define_type(msgpack_type( hsseed, clock )) {}
		HSSeed& hsseed()				{ return get<0>(); }
		uint32_t clock() const			{ return get<1>(); }
		// accepted: true
	};

	struct CreateBackup : define< tuple<std::string> > {
		CreateBackup() {}
		CreateBackup(const std::string& postfix) :
			define_type(msgpack_type( postfix )) {}
		const std::string& suffix() const { return get<0>(); }
		// success: true
	};

	struct ReplaceCopyEnd : define< tuple<uint64_t, uint32_t> > {
		ReplaceCopyEnd() {}
		ReplaceCopyEnd(uint64_t clocktime, uint32_t clock) :
			define_type(msgpack_type( clocktime, clock )) {}
		uint64_t clocktime() const		{ return get<0>(); }
		uint32_t clock() const			{ return get<1>(); }
		// acknowledge: true
	};

	struct ReplaceDeleteEnd : define< tuple<uint64_t, uint32_t> > {
		ReplaceDeleteEnd() {}
		ReplaceDeleteEnd(uint64_t clocktime, uint32_t clock) :
			define_type(msgpack_type( clocktime, clock )) {}
		uint64_t clocktime() const		{ return get<0>(); }
		uint32_t clock() const			{ return get<1>(); }
		// acknowledge: true
	};

	struct HashSpacePush : define< tuple<HSSeed, HSSeed> > {
		HashSpacePush() {}
		HashSpacePush(HSSeed& wseed, HSSeed& rseed) :
			define_type(msgpack_type( wseed, rseed )) {}
		HSSeed& wseed()					{ return get<0>(); }
		HSSeed& rseed()					{ return get<1>(); }
		// acknowledge: true
	};


	struct ReplicateSet : define< tuple<uint32_t, uint32_t, raw_ref, raw_ref> > {
		ReplicateSet() {}
		ReplicateSet(
				const char* raw_key, size_t raw_keylen,
				const char* raw_val, size_t raw_vallen,
				uint32_t clock, bool rhs) :
			define_type(msgpack_type(
						(rhs ? 0x01 : 0),
						clock,
						raw_ref(raw_key, raw_keylen),
						raw_ref(raw_val, raw_vallen)
						)) {}
		DBKey dbkey() const
			{ return DBKey(get<2>().ptr, get<2>().size); }
		DBValue dbval() const
			{ return DBValue(get<3>().ptr, get<3>().size); }
		uint32_t clock() const			{ return get<1>(); }
		bool is_rhs() const				{ return get<0>() & 0x01; }
		// success: true
		// ignored: false
	};

	struct ReplicateDelete : define< tuple<uint32_t, uint32_t, uint64_t, raw_ref> > {
		ReplicateDelete() {}
		ReplicateDelete(
				const char* raw_key, size_t raw_keylen,
				uint64_t clocktime,
				uint32_t clock, bool rhs) :
			define_type(msgpack_type(
						(rhs ? 0x01 : 0),
						clock,
						clocktime,
						raw_ref(raw_key, raw_keylen)
						)) {}
		DBKey dbkey() const
			{ return DBKey(get<3>().ptr, get<3>().size); }
		uint64_t clocktime() const		{ return get<2>(); }
		uint32_t clock() const			{ return get<1>(); }
		bool is_rhs() const				{ return get<0>() & 0x01; }
		// success: true
		// ignored: false
	};

	struct ReplaceOffer : define< tuple<uint16_t> > {
		ReplaceOffer() { }
		ReplaceOffer(uint16_t port) :
			define_type(msgpack_type( port )) { }
		uint16_t port() const { return get<0>(); }
		// no response
	};

	struct ReplaceElection : define< tuple<HSSeed, uint32_t> > {
		ReplaceElection() {}
		ReplaceElection(HSSeed& hsseed, uint32_t clock) :
			define_type(msgpack_type( hsseed, clock )) {}
		HSSeed& hsseed()				{ return get<0>(); }
		uint32_t clock() const			{ return get<1>(); }
		// sender   of ReplaceElection is responsible for replacing: true
		// receiver of ReplaceElection is responsible for replacing: nil
	};

	struct HashSpaceSync : define< tuple<HSSeed, HSSeed, uint32_t> > {
		HashSpaceSync() {}
		HashSpaceSync(HSSeed& wseed, HSSeed& rseed, uint32_t clock) :
			define_type(msgpack_type( wseed, rseed, clock )) {}
		HSSeed& wseed()					{ return get<0>(); }
		HSSeed& rseed()					{ return get<1>(); }
		uint32_t clock() const			{ return get<2>(); }
		// success: true
		// obsolete: nil
	};

	struct Get : define< tuple<DBKey> > {
		Get() {}
		Get(DBKey k) : define_type(msgpack_type( k )) {}
		const DBKey& dbkey() const		{ return get<0>(); }
		// success: value:DBValue
		// not found: nil
	};

	struct Set : define< tuple<uint32_t, DBKey, DBValue> > {
		Set() {}
		Set(DBKey k, DBValue v, bool async) :
			define_type(msgpack_type( (async ? 0x1 : 0), k, v )) {}
		const DBKey& dbkey() const		{ return get<1>(); }
		const DBValue& dbval() const	{ return get<2>(); }
		bool is_async() const			{ return get<0>() & 0x01; }
		// success: tuple< clocktime:uint64 >
		// failed:  nil
	};

	struct Delete : define< tuple<uint32_t, DBKey> > {
		Delete() {}
		Delete(DBKey k, bool async) :
			define_type(msgpack_type( (async ? 0x1 : 0), k )) {}
		const DBKey& dbkey() const		{ return get<1>(); }
		bool is_async() const			{ return get<0>() & 0x01; }
		// success: true
		// not foud: false
		// failed: nil
	};

	struct GetStatus : define< tuple<uint32_t> > {
		GetStatus() {}
		uint32_t command() const	{ return get<0>(); }
	};

}  // namespace type

}  // namespace protocol

}  // namespace kumo

#endif /* protocol.h */


/*

Copyright (c) 2016, Alden Torres
Copyright (c) 2016-2017, 2019, Andrei Kurushin
Copyright (c) 2017-2020, Arvid Norberg
Copyright (c) 2021, Mike Tzou
All rights reserved.

You may use, distribute and modify this code under the terms of the BSD license,
see LICENSE file.
*/

#ifndef TORRENT_HASHER512_HPP_INCLUDED
#define TORRENT_HASHER512_HPP_INCLUDED

#include "libtorrent/config.hpp"
#include "libtorrent/sha1_hash.hpp"
#include "libtorrent/span.hpp"

#include <cstdint>

#include "libtorrent/aux_/disable_warnings_push.hpp"
#ifdef TORRENT_USE_LIBGCRYPT
#include <gcrypt.h>

#elif TORRENT_USE_COMMONCRYPTO
#include <CommonCrypto/CommonDigest.h>

#elif TORRENT_USE_CNG
#include "libtorrent/aux_/win_cng.hpp"

#elif TORRENT_USE_CRYPTOAPI_SHA_512
#include "libtorrent/aux_/win_crypto_provider.hpp"

#elif defined TORRENT_USE_LIBCRYPTO

extern "C" {
#include <openssl/evp.h>
}

#else
#include "libtorrent/aux_/sha512.hpp"
#endif

#include "libtorrent/aux_/disable_warnings_pop.hpp"

namespace libtorrent {
namespace aux {

	using sha512_hash = digest32<512>;

	// internal
	struct TORRENT_EXTRA_EXPORT hasher512
	{
	// this is a SHA-512 hash class.
	//
	// You use it by first instantiating it, then call ``update()`` to feed it
	// with data. i.e. you don't have to keep the entire buffer of which you want to
	// create the hash in memory. You can feed the hasher parts of it at a time. When
	// You have fed the hasher with all the data, you call ``final()`` and it
	// will return the sha1-hash of the data.
	//
	// The constructor that takes a ``char const*`` and an integer will construct the
	// sha1 context and feed it the data passed in.
	//
	// If you want to reuse the hasher object once you have created a hash, you have to
	// call ``reset()`` to reinitialize it.
	//
	// The built-in software version of the sha512-algorithm is from LibTomCrypt
		hasher512();

		// this is the same as default constructing followed by a call to
		// ``update(data)``.
		explicit hasher512(span<char const> data);
		hasher512(hasher512 const&);
		hasher512& operator=(hasher512 const&) &;
		hasher512(hasher512&&);
		hasher512& operator=(hasher512&&) &;

		// append the following bytes to what is being hashed
		hasher512& update(span<char const> data);

		// store the SHA-512 digest of the buffers previously passed to
		// update() and the hasher constructor.
		sha512_hash final();

		// restore the hasher state to be as if the hasher has just been
		// default constructed.
		void reset();

		// hidden
		~hasher512();

	private:

#ifdef TORRENT_USE_LIBGCRYPT
		gcry_md_hd_t m_context;
#elif TORRENT_USE_COMMONCRYPTO
		CC_SHA512_CTX m_context;
#elif TORRENT_USE_CNG
		aux::cng_hash<aux::cng_sha512_algorithm> m_context;
#elif TORRENT_USE_CRYPTOAPI_SHA_512
		aux::crypt_hash<CALG_SHA_512, PROV_RSA_AES> m_context;
#elif defined TORRENT_USE_LIBCRYPTO
		EVP_MD_CTX *m_context = nullptr;
#else
		sha512_ctx m_context;
#endif
	};

}
}

#endif // TORRENT_HASHER512_HPP_INCLUDED

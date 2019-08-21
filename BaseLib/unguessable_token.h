#pragma once

// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>
#include <string.h>
#include <iosfwd>
#include <tuple>

#include "base_export.h"
#include "hash/hash.h"
#include "logging.h"
#include "token.h"

namespace base
{

	struct UnguessableTokenHash;

	// UnguessableToken is, like Token, a randomly chosen 128-bit value. Unlike
	// Token however, a new UnguessableToken must always be generated at runtime
	// from a cryptographically strong random source (or copied or serialized and
	// deserialized from another such UnguessableToken). It can be used as part of a
	// larger aggregate type, or as an ID in and of itself.
	//
	// UnguessableToken can be used to implement "Capability-Based Security".
	// In other words, UnguessableToken can be used when the resource associated
	// with the ID needs to be protected against manipulation by other untrusted
	// agents in the system, and there is no other convenient way to verify the
	// authority of the agent to do so (because the resource is part of a table
	// shared across processes, for instance). In such a scheme, knowledge of the
	// token value in and of itself is sufficient proof of authority to carry out
	// an operation against the associated resource.
	//
	// Use Create() for creating new UnguessableTokens.
	//
	// NOTE: It is illegal to send empty UnguessableTokens across processes, and
	// sending/receiving empty tokens should be treated as a security issue.
	// If there is a valid scenario for sending "no token" across processes,
	// base::Optional should be used instead of an empty token.
	class BASE_EXPORT UnguessableToken
	{
	public:
		// Create a unique UnguessableToken.
		static UnguessableToken Create();

		// Returns a reference to a global null UnguessableToken. This should only be
		// used for functions that need to return a reference to an UnguessableToken,
		// and should not be used as a general-purpose substitute for invoking the
		// default constructor.
		static const UnguessableToken& Null();

		// Return a UnguessableToken built from the high/low bytes provided.
		// It should only be used in deserialization scenarios.
		//
		// NOTE: If the deserialized token is empty, it means that it was never
		// initialized via Create(). This is a security issue, and should be handled.
		static UnguessableToken Deserialize(uint64_t high, uint64_t low);

		// Creates an empty UnguessableToken.
		// Assign to it with Create() before using it.
		constexpr UnguessableToken() = default;

		// NOTE: Serializing an empty UnguessableToken is an illegal operation.
		uint64_t GetHighForSerialization() const
		{
			DCHECK(!is_empty());
			return token_.high();
		}

		// NOTE: Serializing an empty UnguessableToken is an illegal operation.
		uint64_t GetLowForSerialization() const
		{
			DCHECK(!is_empty());
			return token_.low();
		}

		bool is_empty() const { return token_.is_zero(); }

		// Hex representation of the unguessable token.
		std::string ToString() const { return token_.ToString(); }

		explicit operator bool() const { return !is_empty(); }

		bool operator<(const UnguessableToken& other) const
		{
			return token_ < other.token_;
		}

		bool operator==(const UnguessableToken& other) const
		{
			return token_ == other.token_;
		}

		bool operator!=(const UnguessableToken& other) const
		{
			return !(*this == other);
		}

	private:
		friend struct UnguessableTokenHash;
		explicit UnguessableToken(const Token& token);

		base::Token token_;
	};

	BASE_EXPORT std::ostream& operator<<(std::ostream& out,
		const UnguessableToken& token);

	// For use in std::unordered_map.
	struct UnguessableTokenHash
	{
		size_t operator()(const base::UnguessableToken& token) const
		{
			DCHECK(token);
			return TokenHash()(token.token_);
		}
	};

}  // namespace base
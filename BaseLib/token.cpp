// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "token.h"

#include <cinttypes>

#include "pickle.h"
#include "rand_util.h"
#include "strings/stringprintf.h"

namespace base {

	// static
	Token Token::CreateRandom() {
		Token token;

		// Use base::RandBytes instead of crypto::RandBytes, because crypto calls the
		// base version directly, and to prevent the dependency from base/ to crypto/.
		RandBytes(&token, sizeof(token));
		return token;
	}

	std::string Token::ToString() const {
		return StringPrintf("%016" PRIX64 "%016" PRIX64, high_, low_);
	}

	void WriteTokenToPickle(Pickle* pickle, const Token& token) {
		pickle->WriteUInt64(token.high());
		pickle->WriteUInt64(token.low());
	}

	std::optional<Token> ReadTokenFromPickle(PickleIterator* pickle_iterator) {
		uint64_t high;
		if (!pickle_iterator->ReadUInt64(&high))
			return std::nullopt;

		uint64_t low;
		if (!pickle_iterator->ReadUInt64(&low))
			return std::nullopt;

		return Token(high, low);
	}

}  // namespace base


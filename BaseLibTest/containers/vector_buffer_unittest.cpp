// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "pch.h"
#include "containers/vector_buffer.h"

#include "test/copy_only_int.h"
#include "test/move_only_int.h"

namespace base::internal {

	TEST(VectorBuffer, DeletePOD) {
		constexpr int size = 10;
		VectorBuffer<int> buffer(size);
		for (int i = 0; i < size; i++)
			buffer[i] = i + 1;

		buffer.DestructRange(buffer.begin(), buffer.end());

		// Delete should do nothing.
		for (int i = 0; i < size; i++)
			EXPECT_EQ(i + 1, buffer[i]);
	}

	TEST(VectorBuffer, DeleteMoveOnly) {
		constexpr int size = 10;
		VectorBuffer<MoveOnlyInt> buffer(size);
		for (int i = 0; i < size; i++)
			buffer[i] = MoveOnlyInt(i + 1);

		buffer.DestructRange(buffer.begin(), buffer.end());

		// Delete should have reset all of the values to 0.
		for (int i = 0; i < size; i++)
			EXPECT_EQ(0, buffer[i].data());
	}

	TEST(VectorBuffer, PODMove) {
		constexpr int size = 10;
		VectorBuffer<int> dest(size);

		VectorBuffer<int> original(size);
		for (int i = 0; i < size; i++)
			original[i] = i + 1;

		original.MoveRange(original.begin(), original.end(), dest.begin());
		for (int i = 0; i < size; i++)
			EXPECT_EQ(i + 1, dest[i]);
	}

	TEST(VectorBuffer, MovableMove) {
		constexpr int size = 10;
		VectorBuffer<MoveOnlyInt> dest(size);

		VectorBuffer<MoveOnlyInt> original(size);
		for (int i = 0; i < size; i++)
			original[i] = MoveOnlyInt(i + 1);

		original.MoveRange(original.begin(), original.end(), dest.begin());

		// Moving from a MoveOnlyInt resets to 0.
		for (int i = 0; i < size; i++) {
			EXPECT_EQ(0, original[i].data());
			EXPECT_EQ(i + 1, dest[i].data());
		}
	}

	TEST(VectorBuffer, CopyToMove) {
		constexpr int size = 10;
		VectorBuffer<CopyOnlyInt> dest(size);

		VectorBuffer<CopyOnlyInt> original(size);
		for (int i = 0; i < size; i++)
			new (&original[i]) CopyOnlyInt(i + 1);

		original.MoveRange(original.begin(), original.end(), dest.begin());

		// The original should have been destructed, which should reset the value to
		// 0. Technically this dereferences the destructed object.
		for (int i = 0; i < size; i++) {
			EXPECT_EQ(0, original[i].data());
			EXPECT_EQ(i + 1, dest[i].data());
		}
	}
} // namespace base

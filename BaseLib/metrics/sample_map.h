// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// SampleMap implements HistogramSamples interface. It is used by the
// SparseHistogram class to store samples.

#pragma once

#include <cstdint>

#include <map>
#include <memory>

#include "macros.h"
#include "metrics/histogram_base.h"
#include "metrics/histogram_samples.h"

namespace base {

	// The logic here is similar to that of PersistentSampleMap but with different
	// data structures. Changes here likely need to be duplicated there.
	class BASE_EXPORT SampleMap : public HistogramSamples {
	public:
		SampleMap();
		explicit SampleMap(uint64_t id);
		~SampleMap() override;

		// HistogramSamples:
		void Accumulate(HistogramBase::Sample value, HistogramBase::Count count) override;
		[[nodiscard]] HistogramBase::Count GetCount(HistogramBase::Sample value) const override;
		[[nodiscard]] HistogramBase::Count TotalCount() const override;
		[[nodiscard]] std::unique_ptr<SampleCountIterator> Iterator() const override;

	protected:
		// Performs arithemetic. |op| is ADD or SUBTRACT.
		bool AddSubtractImpl(SampleCountIterator* iter, Operator op) override;

	private:
		std::map<HistogramBase::Sample, HistogramBase::Count> sample_counts_;

		DISALLOW_COPY_AND_ASSIGN(SampleMap);
	};

}  // namespace base

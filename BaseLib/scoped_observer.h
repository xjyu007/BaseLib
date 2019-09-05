// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <algorithm>
#include <vector>

#include "logging.h"
#include "macros.h"
#include "stl_util.h"

// ScopedObserver is used to keep track of the set of sources an object has
// attached itself to as an observer. When ScopedObserver is destroyed it
// removes the object as an observer from all sources it has been added to.
template <class Source, class Observer>
class ScopedObserver {
public:
	explicit ScopedObserver(Observer* observer) : observer_(observer) {}

	~ScopedObserver() {
		RemoveAll();
	}

	// Adds the object passed to the constructor as an observer on |source|.
	void Add(Source* source) {
		sources_.push_back(source);
		source->AddObserver(observer_);
	}

	// Remove the object passed to the constructor as an observer from |source|.
	void Remove(Source* source) {
		auto it = std::find(sources_.begin(), sources_.end(), source);
		DCHECK(it != sources_.end());
		sources_.erase(it);
		source->RemoveObserver(observer_);
	}

	void RemoveAll() {
		for (size_t i = 0; i < sources_.size(); ++i)
			sources_[i]->RemoveObserver(observer_);
		sources_.clear();
	}

	bool IsObserving(Source* source) const {
		return base::Contains(sources_, source);
	}

	[[nodiscard]] bool IsObservingSources() const { return !sources_.empty(); }

	[[nodiscard]] size_t GetSourcesCount() const { return sources_.size(); }

private:
	Observer* observer_;

	std::vector<Source*> sources_;

	DISALLOW_COPY_AND_ASSIGN(ScopedObserver);
};

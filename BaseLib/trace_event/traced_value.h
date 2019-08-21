#pragma once

// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include <memory>
#include <string>
#include <vector>

#include "macros.h"
#include "pickle.h"
#include "trace_event/trace_event_impl.h"

namespace base {

	class Value;

	namespace trace_event {

		class BASE_EXPORT TracedValue : public ConvertableToTraceFormat {
		public:
			TracedValue();
			explicit TracedValue(size_t capacity, bool force_json = false);
			~TracedValue() override;

			void EndDictionary();
			void EndArray();

			// These methods assume that |name| is a long lived "quoted" string.
			void SetInteger(const char* name, int value);
			void SetDouble(const char* name, double value);
			void SetBoolean(const char* name, bool value);
			void SetString(const char* name, std::string_view value);
			void SetValue(const char* name, TracedValue* value);
			void BeginDictionary(const char* name);
			void BeginArray(const char* name);

			// These, instead, can be safely passed a temporary string.
			void SetIntegerWithCopiedName(std::string_view name, int value);
			void SetDoubleWithCopiedName(std::string_view name, double value);
			void SetBooleanWithCopiedName(std::string_view name, bool value);
			void SetStringWithCopiedName(std::string_view name, std::string_view value);
			void SetValueWithCopiedName(std::string_view name, TracedValue* value);
			void BeginDictionaryWithCopiedName(std::string_view name);
			void BeginArrayWithCopiedName(std::string_view name);

			void AppendInteger(int);
			void AppendDouble(double);
			void AppendBoolean(bool);
			void AppendString(std::string_view);
			void BeginArray();
			void BeginDictionary();

			// ConvertableToTraceFormat implementation.
			void AppendAsTraceFormat(std::string* out) const override;
			bool AppendToProto(ProtoAppender* appender) override;

			void EstimateTraceMemoryOverhead(TraceEventMemoryOverhead* overhead) override;

			// A custom serialization class can be supplied by implementing the
			// Writer interface and supplying a factory class to SetWriterFactoryCallback.
			// Primarily used by Perfetto to write TracedValues directly into its proto
			// format, which lets us do a direct memcpy() in AppendToProto() rather than
			// a JSON serialization step in AppendAsTraceFormat.
			class BASE_EXPORT Writer {
			public:
				virtual ~Writer() = default;

				virtual void BeginArray() = 0;
				virtual void BeginDictionary() = 0;
				virtual void EndDictionary() = 0;
				virtual void EndArray() = 0;

				// These methods assume that |name| is a long lived "quoted" string.
				virtual void SetInteger(const char* name, int value) = 0;
				virtual void SetDouble(const char* name, double value) = 0;
				virtual void SetBoolean(const char* name, bool value) = 0;
				virtual void SetString(const char* name, std::string_view value) = 0;
				virtual void SetValue(const char* name, Writer* value) = 0;
				virtual void BeginDictionary(const char* name) = 0;
				virtual void BeginArray(const char* name) = 0;

				// These, instead, can be safely passed a temporary string.
				virtual void SetIntegerWithCopiedName(std::string_view name, int value) = 0;
				virtual void SetDoubleWithCopiedName(std::string_view name, double value) = 0;
				virtual void SetBooleanWithCopiedName(std::string_view name, bool value) = 0;
				virtual void SetStringWithCopiedName(std::string_view name, std::string_view value) = 0;
				virtual void SetValueWithCopiedName(std::string_view name, Writer* value) = 0;
				virtual void BeginDictionaryWithCopiedName(std::string_view name) = 0;
				virtual void BeginArrayWithCopiedName(std::string_view name) = 0;

				virtual void AppendInteger(int) = 0;
				virtual void AppendDouble(double) = 0;
				virtual void AppendBoolean(bool) = 0;
				virtual void AppendString(std::string_view) = 0;

				virtual void AppendAsTraceFormat(std::string* out) const = 0;

				virtual bool AppendToProto(ProtoAppender* appender);

				virtual void EstimateTraceMemoryOverhead(TraceEventMemoryOverhead* overhead) = 0;

				virtual std::unique_ptr<base::Value> ToBaseValue() const = 0;

				virtual bool IsPickleWriter() const = 0;
				virtual bool IsProtoWriter() const = 0;
			};

			typedef std::unique_ptr<Writer>(*WriterFactoryCallback)(size_t capacity);
			static void SetWriterFactoryCallback(WriterFactoryCallback callback);

			// Public for tests only.
			std::unique_ptr<base::Value> ToBaseValue() const;

		private:
			std::unique_ptr<Writer> writer_;

#ifndef NDEBUG
			// In debug builds checks the pairings of {Start,End}{Dictionary,Array}
			std::vector<bool> nesting_stack_;
#endif

			DISALLOW_COPY_AND_ASSIGN(TracedValue);
		};

	}  // namespace trace_event
}  // namespace base

// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "i18n/time_formatting.h"

#include <stddef.h>

#include <memory>

#include "i18n/unicodestring.h"
#include "logging.h"
#include "strings/utf_string_conversions.h"
#include "time/time.h"
#include "unicode/utypes.h"
#include "unicode/datefmt.h"
#include "unicode/dtitvfmt.h"
#include "unicode/dtptngen.h"
#include "unicode/fmtable.h"
#include "unicode/measfmt.h"
#include "unicode/smpdtfmt.h"

namespace base {
	namespace {

		std::wstring TimeFormat(const icu::DateFormat* formatter,
								const Time& time) {
			DCHECK(formatter);
			icu::UnicodeString date_string;

			formatter->format(static_cast<UDate>(time.ToDoubleT() * 1000), date_string);
			return i18n::UnicodeStringToString16(date_string);
		}

		std::wstring TimeFormatWithoutAmPm(const icu::DateFormat* formatter,
			const Time& time) {
			DCHECK(formatter);
			icu::UnicodeString time_string;

			icu::FieldPosition ampm_field(icu::DateFormat::kAmPmField);
			formatter->format(
				static_cast<UDate>(time.ToDoubleT() * 1000), time_string, ampm_field);
			int ampm_length = ampm_field.getEndIndex() - ampm_field.getBeginIndex();
			if (ampm_length) {
				int begin = ampm_field.getBeginIndex();
				// Doesn't include any spacing before the field.
				if (begin)
					begin--;
				time_string.removeBetween(begin, ampm_field.getEndIndex());
			}
			return i18n::UnicodeStringToString16(time_string);
		}

		icu::SimpleDateFormat CreateSimpleDateFormatter(const char* pattern) {
			// Generate a locale-dependent format pattern. The generator will take
			// care of locale-dependent formatting issues like which separator to
			// use (some locales use '.' instead of ':'), and where to put the am/pm
			// marker.
			UErrorCode status = U_ZERO_ERROR;
			std::unique_ptr<icu::DateTimePatternGenerator> generator(
				icu::DateTimePatternGenerator::createInstance(status));
			DCHECK(U_SUCCESS(status));
			const auto generated_pattern =
				generator->getBestPattern(icu::UnicodeString(pattern), status);
			DCHECK(U_SUCCESS(status));

			// Then, format the time using the generated pattern.
			icu::SimpleDateFormat formatter(generated_pattern, status);
			DCHECK(U_SUCCESS(status));

			return formatter;
		}

		UMeasureFormatWidth DurationWidthToMeasureWidth(DurationFormatWidth width) {
			switch (width) {
			case DURATION_WIDTH_WIDE: return UMEASFMT_WIDTH_WIDE;
			case DURATION_WIDTH_SHORT: return UMEASFMT_WIDTH_SHORT;
			case DURATION_WIDTH_NARROW: return UMEASFMT_WIDTH_NARROW;
			case DURATION_WIDTH_NUMERIC: return UMEASFMT_WIDTH_NUMERIC;
			}
			NOTREACHED();
			return UMEASFMT_WIDTH_COUNT;
		}

		const char* DateFormatToString(DateFormat format) {
			switch (format) {
			case DATE_FORMAT_YEAR_MONTH:
				return UDAT_YEAR_MONTH;
			case DATE_FORMAT_MONTH_WEEKDAY_DAY:
				return UDAT_MONTH_WEEKDAY_DAY;
			}
			NOTREACHED();
			return UDAT_YEAR_MONTH_DAY;
		}

	}  // namespace

	std::wstring TimeFormatTimeOfDay(const Time& time) {
		// We can omit the locale parameter because the default should match
		// Chrome's application locale.
		std::unique_ptr<icu::DateFormat> formatter(
			icu::DateFormat::createTimeInstance(icu::DateFormat::kShort));
		return TimeFormat(formatter.get(), time);
	}

	std::wstring TimeFormatTimeOfDayWithMilliseconds(const Time& time) {
		icu::SimpleDateFormat formatter = CreateSimpleDateFormatter("HmsSSS");
		return TimeFormatWithoutAmPm(&formatter, time);
	}

	std::wstring TimeFormatTimeOfDayWithHourClockType(const Time& time,
													  HourClockType type,
													  AmPmClockType ampm) {
		// Just redirect to the normal function if the default type matches the
		// given type.
		const auto default_type = GetHourClockType();
		if (default_type == type && (type == k24HourClock || ampm == kKeepAmPm)) {
			return TimeFormatTimeOfDay(time);
		}

		const char* base_pattern = (type == k12HourClock ? "ahm" : "Hm");
		auto formatter = CreateSimpleDateFormatter(base_pattern);

		if (ampm == kKeepAmPm) {
			return TimeFormat(&formatter, time);
		}
		return TimeFormatWithoutAmPm(&formatter, time);
	}

	std::wstring TimeFormatShortDate(const Time& time) {
		const std::unique_ptr<icu::DateFormat> formatter(
			icu::DateFormat::createDateInstance(icu::DateFormat::kMedium));
		return TimeFormat(formatter.get(), time);
	}

	std::wstring TimeFormatShortDateNumeric(const Time& time) {
		const std::unique_ptr<icu::DateFormat> formatter(
			icu::DateFormat::createDateInstance(icu::DateFormat::kShort));
		return TimeFormat(formatter.get(), time);
	}

	std::wstring TimeFormatShortDateAndTime(const Time& time) {
		const std::unique_ptr<icu::DateFormat> formatter(
			icu::DateFormat::createDateTimeInstance(icu::DateFormat::kShort));
		return TimeFormat(formatter.get(), time);
	}

	std::wstring TimeFormatShortDateAndTimeWithTimeZone(const Time& time) {
		const std::unique_ptr<icu::DateFormat> formatter(
			icu::DateFormat::createDateTimeInstance(icu::DateFormat::kShort,
				icu::DateFormat::kLong));
		return TimeFormat(formatter.get(), time);
	}

	std::wstring TimeFormatMonthAndYear(const Time& time) {
		auto formatter =
			CreateSimpleDateFormatter(DateFormatToString(DATE_FORMAT_YEAR_MONTH));
		return TimeFormat(&formatter, time);
	}

	std::wstring TimeFormatFriendlyDateAndTime(const Time& time) {
		const std::unique_ptr<icu::DateFormat> formatter(
			icu::DateFormat::createDateTimeInstance(icu::DateFormat::kFull));
		return TimeFormat(formatter.get(), time);
	}

	std::wstring TimeFormatFriendlyDate(const Time& time) {
		const std::unique_ptr<icu::DateFormat> formatter(
			icu::DateFormat::createDateInstance(icu::DateFormat::kFull));
		return TimeFormat(formatter.get(), time);
	}

	std::wstring TimeFormatWithPattern(const Time& time, const char* pattern) {
		auto formatter = CreateSimpleDateFormatter(pattern);
		return TimeFormat(&formatter, time);
	}

	bool TimeDurationFormat(const TimeDelta time,
							const DurationFormatWidth width,
							std::wstring* out) {
		DCHECK(out);
		UErrorCode status = U_ZERO_ERROR;
		const int total_minutes = static_cast<int>(time.InSecondsF() / 60 + 0.5);
		const int hours = total_minutes / 60;
		const int minutes = total_minutes % 60;
		UMeasureFormatWidth u_width = DurationWidthToMeasureWidth(width);

		// TODO(derat): Delete the |status| checks and LOG(ERROR) calls throughout
		// this function once the cause of http://crbug.com/677043 is tracked down.
		const icu::Measure measures[] = {
			icu::Measure(hours, icu::MeasureUnit::createHour(status), status),
			icu::Measure(minutes, icu::MeasureUnit::createMinute(status), status) };
		if (U_FAILURE(status)) {
			LOG(ERROR) << "Creating MeasureUnit or Measure for " << hours << "h"
				<< minutes << "m failed: " << u_errorName(status);
			return false;
		}

		icu::MeasureFormat measure_format(icu::Locale::getDefault(), u_width, status);
		if (U_FAILURE(status)) {
			LOG(ERROR) << "Creating MeasureFormat for "
				<< icu::Locale::getDefault().getName()
				<< " failed: " << u_errorName(status);
			return false;
		}

		icu::UnicodeString formatted;
		icu::FieldPosition ignore(icu::FieldPosition::DONT_CARE);
		measure_format.formatMeasures(measures, 2, formatted, ignore, status);
		if (U_FAILURE(status)) {
			LOG(ERROR) << "formatMeasures failed: " << u_errorName(status);
			return false;
		}

		*out = i18n::UnicodeStringToString16(formatted);
		return true;
	}

	bool TimeDurationFormatWithSeconds(const TimeDelta time,
		const DurationFormatWidth width,
		std::wstring* out) {
		DCHECK(out);
		UErrorCode status = U_ZERO_ERROR;
		const int64_t total_seconds = static_cast<int64_t>(time.InSecondsF() + 0.5);
		const int64_t hours = total_seconds / 3600;
		const int64_t minutes = (total_seconds - hours * 3600) / 60;
		const int64_t seconds = total_seconds % 60;
		UMeasureFormatWidth u_width = DurationWidthToMeasureWidth(width);

		const icu::Measure measures[] = {
			icu::Measure(hours, icu::MeasureUnit::createHour(status), status),
			icu::Measure(minutes, icu::MeasureUnit::createMinute(status), status),
			icu::Measure(seconds, icu::MeasureUnit::createSecond(status), status) };
		const icu::MeasureFormat measure_format(icu::Locale::getDefault(), u_width, status);
		icu::UnicodeString formatted;
		icu::FieldPosition ignore(icu::FieldPosition::DONT_CARE);
		measure_format.formatMeasures(measures, 3, formatted, ignore, status);
		*out = i18n::UnicodeStringToString16(formatted);
		return U_SUCCESS(status) == TRUE;
	}

	std::wstring DateIntervalFormat(const Time& begin_time,
									const Time& end_time,
									DateFormat format) {
		UErrorCode status = U_ZERO_ERROR;

		std::unique_ptr<icu::DateIntervalFormat> formatter(
			icu::DateIntervalFormat::createInstance(DateFormatToString(format),
				status));

		icu::FieldPosition pos = 0;
		const UDate start_date = static_cast<UDate>(begin_time.ToDoubleT() * 1000);
		const UDate end_date = static_cast<UDate>(end_time.ToDoubleT() * 1000);
		icu::DateInterval interval(start_date, end_date);
		icu::UnicodeString formatted;
		formatter->format(&interval, formatted, pos, status);
		return i18n::UnicodeStringToString16(formatted);
	}

	HourClockType GetHourClockType() {
		// TODO(satorux,jshin): Rework this with ures_getByKeyWithFallback()
		// once it becomes public. The short time format can be found at
		// "calendar/gregorian/DateTimePatterns/3" in the resources.
		std::unique_ptr<icu::SimpleDateFormat> formatter(
			static_cast<icu::SimpleDateFormat*>(
				icu::DateFormat::createTimeInstance(icu::DateFormat::kShort)));
		// Retrieve the short time format.
		icu::UnicodeString pattern_unicode;
		formatter->toPattern(pattern_unicode);

		// Determine what hour clock type the current locale uses, by checking
		// "a" (am/pm marker) in the short time format. This is reliable as "a"
		// is used by all of 12-hour clock formats, but not any of 24-hour clock
		// formats, as shown below.
		//
		// % grep -A4 DateTimePatterns third_party/icu/source/data/locales/*.txt |
		//   grep -B1 -- -- |grep -v -- '--' |
		//   perl -nle 'print $1 if /^\S+\s+"(.*)"/' |sort -u
		//
		// H.mm
		// H:mm
		// HH.mm
		// HH:mm
		// a h:mm
		// ah:mm
		// ahh:mm
		// h-mm a
		// h:mm a
		// hh:mm a
		//
		// See http://userguide.icu-project.org/formatparse/datetime for details
		// about the date/time format syntax.
		return pattern_unicode.indexOf('a') == -1 ? k24HourClock : k12HourClock;
	}

}  // namespace base

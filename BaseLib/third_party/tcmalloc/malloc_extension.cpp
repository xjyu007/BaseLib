#include "malloc_extension.h"
#include <atomic>
#include <string>

// Note: this routine is meant to be called before threads are spawned.
void MallocExtension::Initialize(){
	static bool initialize_called = false;
	if (initialize_called) return;
	initialize_called = true;
}

// SysAllocator implementation
SysAllocator::~SysAllocator() = default;

// Default implementation -- does nothing
MallocExtension::~MallocExtension() = default;

bool MallocExtension::GetNumericProperty(const char* property, size_t* value) {
	return false;
}

bool MallocExtension::SetNumericProperty(const char* property, size_t value) {
	return false;
}

void MallocExtension::GetHeapSample(MallocExtensionWriter* writer) {
	int sample_period = 0;
	void** entries = ReadStackTraces(&sample_period);
	if (entries == NULL) {
		const char* const kErrorMsg =
			"This malloc implementation does not support sampling.\n"
			"As of 2005/01/26, only tcmalloc supports sampling, and\n"
			"you are probably running a binary that does not use\n"
			"tcmalloc.\n";
		writer->append(kErrorMsg, strlen(kErrorMsg));
		return;
	}

	char label[32];
	sprintf(label, "heap_v2/%d", sample_period);
	PrintHeader(writer, label, entries);
	for (void** entry = entries; Count(entry) != 0; entry += 3 + Depth(entry)) {
		PrintStackEntry(writer, entry);
	}
	delete[] entries;

	DumpAddressMap(writer);
}

void MallocExtension::ReleaseFreeMemory() {
	ReleaseToSystem(static_cast<size_t>(-1));   // SIZE_T_MAX
}

// The current malloc extension object.

std::atomic<MallocExtension*> MallocExtension::current_instance_;

// static
MallocExtension* MallocExtension::InitModule() {
	MallocExtension* instance = new MallocExtension;
	current_instance_.store(instance, std::memory_order_release);
	return instance;
}
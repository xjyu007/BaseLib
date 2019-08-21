#include "malloc_hook.h"

int MallocHook::GetCallerStackTrace(void** result, int max_depth, int skip_count){
	return 0;
}

DeleteHook MallocHook::SetDeleteHook(DeleteHook hook) {
	return nullptr;
}

NewHook MallocHook::SetNewHook(NewHook hook) {
	return nullptr;
}



// Copyright Epic Games, Inc. All Rights Reserved.

#include "UbaMemory.h"
#include "UbaPlatform.h"

#if PLATFORM_LINUX
#include <linux/mman.h>
#if defined(MAP_HUGE_2MB)
#define UBA_SUPPORTS_HUGE_PAGES 1
#endif
#endif

#if !defined(UBA_SUPPORTS_HUGE_PAGES)
#define UBA_SUPPORTS_HUGE_PAGES 0
#endif

namespace uba
{
	constexpr u64 MemoryBlock_ReserveAlign = 1 * 1024 * 1024;


	MemoryBlock::MemoryBlock(u64 reserveSize_, void* baseAddress_)
	{
		Init(reserveSize_, baseAddress_, false);
	}

	MemoryBlock::MemoryBlock(u8* baseAddress_)
	{
		memory = baseAddress_;
	}

	MemoryBlock::~MemoryBlock()
	{
		Deinit();
	}

	bool MemoryBlock::Init(u64 reserveSize_, void* baseAddress_, bool useHugePages)
	{
		#if PLATFORM_WINDOWS
		reserveSize = AlignUp(reserveSize_, MemoryBlock_ReserveAlign);
		memory = (u8*)VirtualAlloc(baseAddress_, reserveSize, MEM_RESERVE, PAGE_READWRITE); // Max size of obj file?
		if (!memory)
			FatalError(1347, TC("Failed to reserve virtual memory (%u)"), GetLastError());
		#else

		int flags = MAP_PRIVATE | MAP_ANONYMOUS;
		int reserveAlign = MemoryBlock_ReserveAlign;
		#if UBA_SUPPORTS_HUGE_PAGES
		if (useHugePages)
		{
			flags |= MAP_HUGETLB | MAP_HUGE_2MB;
			reserveAlign = 2 * 1024 * 1024;
		}
		#endif

		reserveSize = AlignUp(reserveSize_, reserveAlign);
		memory = (u8*)mmap(baseAddress_, reserveSize, PROT_READ | PROT_WRITE, flags, -1, 0);

		if (memory == MAP_FAILED)
		{
			if (useHugePages)
				return false;
			FatalError(1347, "mmap failed to reserve %llu bytes (asking for %llu): %s", reserveSize, reserveSize_, strerror(errno));
		}
		#endif

		if (baseAddress_ && baseAddress_ != memory)
			FatalError(9881, TC("Failed to reserve virtual memory at address (%u)"), GetLastError());
		return true;
	}

	void MemoryBlock::Deinit()
	{
		if (!memory)
			return;

		#if PLATFORM_WINDOWS
		VirtualFree(memory, 0, MEM_RELEASE);
		#else
		if (munmap(memory, reserveSize) == -1)
			FatalError(9885, "munmap failed to free %llu bytes: %s", reserveSize, strerror(errno));
		#endif
		memory = nullptr;
	}

	void* MemoryBlock::Allocate(u64 bytes, u64 alignment, const tchar* hint)
	{
		if (!memory)
			return aligned_alloc(alignment, bytes);

		SCOPED_WRITE_LOCK(lock, l);
		return AllocateNoLock(bytes, alignment, hint);
	}

	void* MemoryBlock::AllocateNoLock(u64 bytes, u64 alignment, const tchar* hint)
	{
		u64 startPos = AlignUp(writtenSize, alignment);
		u64 newPos = startPos + bytes;
		
		if (newPos > reserveSize)
			FatalError(9882, TC("Ran out of reserved space . Reserved %llu, Needed %llu (%s)"), reserveSize, newPos, hint);

		#if PLATFORM_WINDOWS
		if (newPos > mappedSize)
		{
			u64 toCommit = AlignUp(newPos - mappedSize, MemoryBlock_ReserveAlign);
			if (mappedSize + toCommit > reserveSize)
				toCommit = reserveSize - mappedSize;
			if (!VirtualAlloc(memory + mappedSize, toCommit, MEM_COMMIT, PAGE_READWRITE))
				FatalError(9883, TC("Failed to commit virtual memory for memory block. Total size %llu (%u) (%s)"), mappedSize + toCommit, GetLastError(), hint);
			mappedSize += toCommit;
		}
		#endif

		void* ret = memory + startPos;
		writtenSize = newPos;
		return ret;
	}

	void MemoryBlock::ReserveNoLock(u64 bytes, const tchar* hint)
	{
		#if PLATFORM_WINDOWS
		u64 newPos = Min(writtenSize + bytes, reserveSize);
		if (newPos <= mappedSize)
			return;
		u64 toCommit = AlignUp(newPos - mappedSize, MemoryBlock_ReserveAlign);
		if (mappedSize + toCommit > reserveSize)
			toCommit = reserveSize - mappedSize;
		if (!VirtualAlloc(memory + mappedSize, toCommit, MEM_COMMIT, PAGE_READWRITE))
			FatalError(9883, TC("Failed to commit virtual memory for memory block. Total size %llu (%u) (%s)"), mappedSize + toCommit, GetLastError(), hint);
		mappedSize += toCommit;
		#endif
	}

	void MemoryBlock::Free(void* p)
	{
		if (!memory)
			aligned_free(p);
	}

	tchar* MemoryBlock::Strdup(const tchar* str)
	{
		u64 len = TStrlen(str);
		u64 memSize = (len + 1) * sizeof(tchar);
		void* mem = Allocate(memSize, sizeof(tchar), TC("Strdup"));
		const void* src = str;
		memcpy(mem, src, memSize);
		return (tchar*)mem;
	}

	void MemoryBlock::Swap(MemoryBlock& other)
	{
		u8* m = memory;
		u64 rs = reserveSize;
		u64 ws = writtenSize;
		u64 ms = mappedSize;

		memory = other.memory;
		reserveSize = other.reserveSize;
		writtenSize = other.writtenSize;
		mappedSize = other.mappedSize;

		other.memory = m;
		other.reserveSize = rs;
		other.writtenSize = ws;
		other.mappedSize = ms;
	}

	bool SupportsHugePages()
	{
		return UBA_SUPPORTS_HUGE_PAGES != 0;
	}
	u64 GetHugePageCount()
	{
#if UBA_SUPPORTS_HUGE_PAGES
		FILE* f = fopen("/proc/sys/vm/nr_hugepages", "r");
		if (!f)
			return 0;
		u32 pageCount = 0;
		fscanf(f, "%u", &pageCount);
		fclose(f);
		return pageCount;
#else
		return 0;
#endif
	}
}

// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassArchetypeTypes.h"
#include "StructUtils/InstancedStruct.h"
#include "MassEntityView.generated.h"


struct FMassEntityManager;
struct FMassArchetypeData;
struct FMassArchetypeHandle;

/** 
 * The type representing a single entity in a single archetype. It's of a very transient nature so we guarantee it's 
 * validity only within the scope it has been created in. Don't store it. 
 */
USTRUCT()
struct MASSENTITY_API FMassEntityView
{
	GENERATED_BODY()

	FMassEntityView() = default;

	/** 
	 *  Resolves Entity against ArchetypeHandle. Note that this approach requires the caller to ensure that Entity
	 *  indeed belongs to ArchetypeHandle. If not the call will fail a check. As a remedy calling the 
	 *  FMassEntityManager-flavored constructor is recommended since it will first find the appropriate archetype for
	 *  Entity. 
	 */
	FMassEntityView(const FMassArchetypeHandle& ArchetypeHandle, FMassEntityHandle Entity);

	/** 
	 *  Finds the archetype Entity belongs to and then resolves against it. The caller is responsible for ensuring
	 *  that the given Entity is in fact a valid ID tied to any of the archetypes 
	 */
	FMassEntityView(const FMassEntityManager& EntityManager, FMassEntityHandle Entity);

	/** 
	 * If the given handle represents a valid entity the function will create a FMassEntityView just like a constructor 
	 * would. If the entity is not valid the produced view will be "unset".
	 */
	static FMassEntityView TryMakeView(const FMassEntityManager& EntityManager, FMassEntityHandle Entity);

	FMassEntityHandle GetEntity() const	{ return Entity; }

	/** will fail a check if the viewed entity doesn't have the given fragment */	
	template<typename T>
	T& GetFragmentData() const
	{
		static_assert(!std::is_base_of_v<FMassTag, T>,
			"Given struct doesn't represent a valid fragment type but a tag. Use HasTag instead.");
		static_assert(std::is_base_of_v<FMassTag, T> || std::is_base_of_v<FMassFragment, T>,
			"Given struct doesn't represent a valid fragment type. Make sure to inherit from FMassFragment or one of its child-types.");

		return *((T*)GetFragmentPtrChecked(*T::StaticStruct()));
	}
		
	/** if the viewed entity doesn't have the given fragment the function will return null */
	template<typename T>
	T* GetFragmentDataPtr() const
	{
		static_assert(!std::is_base_of_v<FMassTag, T>,
			"Given struct doesn't represent a valid fragment type but a tag. Use HasTag instead.");
		static_assert(std::is_base_of_v<FMassTag, T> || std::is_base_of_v<FMassFragment, T>,
			"Given struct doesn't represent a valid fragment type. Make sure to inherit from FMassFragment or one of its child-types.");

		return (T*)GetFragmentPtr(*T::StaticStruct());
	}

	FStructView GetFragmentDataStruct(const UScriptStruct* FragmentType) const
	{
		check(FragmentType);
		return FStructView(FragmentType, static_cast<uint8*>(GetFragmentPtr(*FragmentType)));
	}

	/** if the viewed entity doesn't have the given const shared fragment the function will return null */
	template<typename T>
	const T* GetConstSharedFragmentDataPtr() const
	{
		static_assert(std::is_base_of_v<FMassConstSharedFragment, T>,
			"Given struct doesn't represent a valid shared fragment type. Make sure to inherit from FMassConstSharedFragment or one of its child-types.");

		return (const T*)GetConstSharedFragmentPtr(*T::StaticStruct());
	}

	/** will fail a check if the viewed entity doesn't have the given const shared fragment */
	template<typename T>
	const T& GetConstSharedFragmentData() const
	{
		static_assert(std::is_base_of_v<FMassConstSharedFragment, T>,
			"Given struct doesn't represent a valid const shared fragment type. Make sure to inherit from FMassConstSharedFragment or one of its child-types.");

		return *((const T*)GetConstSharedFragmentPtrChecked(*T::StaticStruct()));
	}

	FConstStructView GetConstSharedFragmentDataStruct(const UScriptStruct* FragmentType) const
	{
		check(FragmentType && FragmentType->IsChildOf(FMassConstSharedFragment::StaticStruct()));
		return FConstStructView(FragmentType, static_cast<const uint8*>(GetConstSharedFragmentPtr(*FragmentType)));
	}

	/** will fail a check if the viewed entity doesn't have the given shared fragment */
	template<typename T UE_REQUIRES(std::is_base_of_v<FMassSharedFragment, T>)>
	T& GetSharedFragmentData() const
	{
		return *((T*)GetSharedFragmentPtrChecked(*T::StaticStruct()));
	}

	/** if the viewed entity doesn't have the given shared fragment the function will return null */
	template<typename T UE_REQUIRES(std::is_base_of_v<FMassSharedFragment, T>)>
	T* GetSharedFragmentDataPtr() const
	{
		return (T*)GetSharedFragmentPtr(*T::StaticStruct());
	}

	template<typename T UE_REQUIRES(std::is_base_of_v<FMassConstSharedFragment, T>)>
	UE_DEPRECATED(5.5, "Using GetSharedFragmentDataPtr with const shared fragments is deprecated. Use GetConstSharedFragmentDataPtr instead")
	T* GetSharedFragmentDataPtr() const
	{
		return const_cast<T*>(GetConstSharedFragmentDataPtr<T>());
	}

	template<typename T UE_REQUIRES(std::is_base_of_v<FMassConstSharedFragment, T>)>
	UE_DEPRECATED(5.5, "Using GetSharedFragmentDataPtr with const shared fragments is deprecated. Use GetConstSharedFragmentData instead")
	T& GetSharedFragmentData() const
	{
		/*static T DummyInstance;
		return DummyInstance;*/
		return const_cast<T&>(GetConstSharedFragmentData<T>());
	}

	FStructView GetSharedFragmentDataStruct(const UScriptStruct* FragmentType) const
	{
		check(FragmentType && FragmentType->IsChildOf(FMassSharedFragment::StaticStruct()));
		return FStructView(FragmentType, static_cast<uint8*>(GetSharedFragmentPtr(*FragmentType)));
	}

	template<typename T>
	bool HasTag() const
	{
		static_assert(std::is_base_of_v<FMassTag, T>, "Given struct doesn't represent a valid tag type. Make sure to inherit from FMassTag or one of its child-types.");
		return HasTag(*T::StaticStruct());
	}

	bool HasTag(const UScriptStruct& TagType) const;

	bool IsSet() const { return Archetype != nullptr && EntityDataHandle.IsValid(); }
	bool IsValid() const { return IsSet(); }
	bool operator==(const FMassEntityView& Other) const { return Archetype == Other.Archetype && EntityDataHandle == Other.EntityDataHandle; }

protected:
	void* GetFragmentPtr(const UScriptStruct& FragmentType) const;
	void* GetFragmentPtrChecked(const UScriptStruct& FragmentType) const;
	const void* GetConstSharedFragmentPtr(const UScriptStruct& FragmentType) const;
	const void* GetConstSharedFragmentPtrChecked(const UScriptStruct& FragmentType) const;
	void* GetSharedFragmentPtr(const UScriptStruct& FragmentType) const;
	void* GetSharedFragmentPtrChecked(const UScriptStruct& FragmentType) const;

private:
	FMassEntityHandle Entity;
	FMassRawEntityInChunkData EntityDataHandle;
	FMassArchetypeData* Archetype = nullptr;
};

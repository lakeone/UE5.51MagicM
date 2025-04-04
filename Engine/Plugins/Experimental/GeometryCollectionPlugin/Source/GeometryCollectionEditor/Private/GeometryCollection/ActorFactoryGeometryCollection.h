// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "ActorFactories/ActorFactory.h"

#include "ActorFactoryGeometryCollection.generated.h"

class AActor;
struct FAssetData;

UCLASS(MinimalAPI, config=Editor)
class UActorFactoryGeometryCollection : public UActorFactory
{
	GENERATED_UCLASS_BODY()

	//~ Begin UActorFactory Interface
	virtual void PostSpawnActor( UObject* Asset, AActor* NewActor ) override;
	virtual bool CanCreateActorFrom( const FAssetData& AssetData, FText& OutErrorMsg ) override;
	//~ End UActorFactory Interface
};




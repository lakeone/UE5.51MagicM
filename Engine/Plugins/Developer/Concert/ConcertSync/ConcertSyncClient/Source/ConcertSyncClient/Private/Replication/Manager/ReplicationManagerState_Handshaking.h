// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ConcertSyncSessionFlags.h"
#include "ReplicationManagerState.h"
#include "Replication/Processing/ClientReplicationDataCollector.h"

class IConcertClientSession;

namespace UE::ConcertSyncClient::Replication
{
	/** Initial state: waiting for a call to JoinReplicationSession to join replication session. */
	class FReplicationManagerState_Handshaking : public FReplicationManagerState
	{
	public:
		
		FReplicationManagerState_Handshaking(
			FJoinReplicatedSessionArgs RequestArgs,
			TPromise<FJoinReplicatedSessionResult> JoinSessionPromise,
			TSharedRef<IConcertClientSession> LiveSession,
			IConcertClientReplicationBridge& ReplicationBridge UE_LIFETIMEBOUND,
			FReplicationManager& Owner UE_LIFETIMEBOUND,
			EConcertSyncSessionFlags SessionFlags
			);
		virtual ~FReplicationManagerState_Handshaking() override;
		
		//~ Begin IConcertClientReplicationManager Interface
		virtual TFuture<FJoinReplicatedSessionResult> JoinReplicationSession(FJoinReplicatedSessionArgs Args) override;
		virtual void LeaveReplicationSession() override;
		virtual bool CanJoin() override { return false; }
		virtual bool IsConnectedToReplicationSession() override { return false; }
		//~ End IConcertClientReplicationManager Interface

	private:

		FJoinReplicatedSessionArgs RequestArgs;
		
		/** Promise for finishing the handshake. */
		TPromise<FJoinReplicatedSessionResult> JoinSessionPromise;
		bool bFulfilledPromise = false;
		
		/** Passed to FReplicationManagerState_Handshaking */
		TSharedRef<IConcertClientSession> LiveSession;
		/** Passed to FReplicationManagerState_Handshaking */
		IConcertClientReplicationBridge& ReplicationBridge;
		/** Passed to FReplicationManagerState_Handshaking */
		const EConcertSyncSessionFlags SessionFlags;

		//~ Begin FReplicationManagerState Interface
		virtual void OnEnterState() override;
		//~ End FReplicationManagerState Interface

		void ReturnToDisconnectedState();
	};
}
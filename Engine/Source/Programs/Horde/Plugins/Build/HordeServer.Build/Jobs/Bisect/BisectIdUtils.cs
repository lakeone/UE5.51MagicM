// Copyright Epic Games, Inc. All Rights Reserved.

using EpicGames.Horde.Jobs.Bisect;
using HordeServer.Utilities;

namespace HordeServer.Jobs.Bisect
{
	/// <summary>
	/// Server helper methods for <see cref="BisectTaskId"/>
	/// </summary>
	static class BisectTaskIdUtils
	{
		/// <summary>
		/// Creates a new <see cref="BisectTaskId"/>
		/// </summary>
		public static BisectTaskId GenerateNewId() => new BisectTaskId(BinaryIdUtils.CreateNew());
	}
}

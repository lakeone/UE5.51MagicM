// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class AudioModulationEditor : ModuleRules
{
	// Set this to false & disable MetaSound plugin dependency
	// by setting MetaSound's field '"Enabled": false' in the
	// AudioModulation.uplugin if running Modulation without
	// MetaSound support.
	public static bool bIncludeMetaSoundSupport = true;

	public AudioModulationEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"EditorFramework",
				"GameProjectGeneration",
				"UnrealEd",
				"PropertyEditor",
				"SequenceRecorder",
				"Slate",
				"SlateCore",
				"InputCore",				
				"AudioEditor",
				"AudioExtensions",
				"AudioInsights",
				"AudioInsightsEditor",
				"AudioModulation",
				"CurveEditor",
				"EditorWidgets",
				"ToolWidgets",
				"TraceAnalysis",
				"WaveTable",
				"WaveTableEditor"
			}
		);

		if (bIncludeMetaSoundSupport)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"MetasoundGraphCore",
					"MetasoundFrontend",
					"MetasoundEditor"
				}
			);
		}
	}
}

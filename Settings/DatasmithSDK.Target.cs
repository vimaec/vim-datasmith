// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;
using System.IO;

public class DatasmithSDKTarget : TargetRules
{
	public DatasmithSDKTarget(TargetInfo Target)
		: base(Target)
	{
		Type = TargetType.Program;
		SolutionDirectory = "Programs/Datasmith";

		LaunchModuleName = "DatasmithSDK";
		ExeBinariesSubFolder = "DatasmithSDK";

		ExtraModuleNames.AddRange( new string[] { "DatasmithCore", "DatasmithExporter" } );

		LinkType = TargetLinkType.Monolithic;
		bShouldCompileAsDLL = true;

		bBuildDeveloperTools = false;
		bUseMallocProfiler = false;
		bBuildWithEditorOnlyData = true;
		bCompileAgainstEngine = false;
		bCompileAgainstCoreUObject = true;
		bCompileICU = false;
		bUsesSlate = false;
//		bDisableDebugInfo = true;
		bUsePDBFiles = true;
		bHasExports = true;
		bIsBuildingConsoleApplication = true;

		AddCopySDKDocumentationAndHeadersPostBuildSteps();
	}

	public void PostBuildCopy(string SrcPath, string SrcMask, string Destination)
    {
        string DestPath = Path.Combine("$(EngineDir)", "Binaries", "$(TargetPlatform)", "DatasmithSDK", Destination);

        if (Platform == UnrealTargetPlatform.Win64 || Platform == UnrealTargetPlatform.Win32)
        {
            PostBuildSteps.Add(string.Format("echo Copying {0}\\{1} to {2}", SrcPath, SrcMask, DestPath));
            PostBuildSteps.Add(string.Format("xcopy {0}\\{1} {2} /R /Y /S", SrcPath, SrcMask, DestPath));
        }
        else
        {
            PostBuildSteps.Add(string.Format("echo rsync -r -t \\\"\"{0}/\"\\\" to \\\"\"{1}\"\\\"", SrcPath, DestPath));
            PostBuildSteps.Add(string.Format("rsync -r -t \"{0}/\" \"{1}\"", SrcPath, DestPath));
        }
    }

	public void AddCopySDKDocumentationAndHeadersPostBuildSteps()
    {
        // Copy the documentation
        PostBuildCopy(
        Path.Combine("$(EngineDir)", "Source", "Programs", "Enterprise", "Datasmith", "DatasmithSDK", "Documentation"), "*.*",
        "Documentation"
        );
        
        // Package our public headers
        PostBuildCopy(
        Path.Combine("$(EngineDir)", "Source", "Runtime", "Datasmith", "DatasmithCore", "Public"), "*.h",
        "Public"
        );
        
        PostBuildCopy(
        Path.Combine("$(EngineDir)", "Source", "Runtime", "Datasmith", "DirectLink", "Public"), "*.h",
        "Public"
        );
        
        PostBuildCopy(
        Path.Combine("$(EngineDir)", "Source", "Developer", "Datasmith", "DatasmithExporter", "Public"), "*.h",
        "Public"
        );
        
        // Other headers we depend on, but that are not part of our public API:
        PostBuildCopy(
        Path.Combine("$(EngineDir)", "Source", "Runtime", "TraceLog", "Public"), "*.*",
        "Private"
        );
        
        PostBuildCopy(
        Path.Combine("$(EngineDir)", "Source", "Runtime", "Messaging", "Public"), "*.h",
        "Private"
        );
        
        PostBuildCopy(
        Path.Combine("$(EngineDir)", "Source", "Runtime", "Core", "Public"), "*.*",
        "Private"
        );
        
        PostBuildCopy(
        Path.Combine("$(EngineDir)", "Source", "Runtime", "CoreUObject", "Public"), "*.h",
        "Private"
        );
    }
}

# vim-datasmith
A VIM to DataSmith converter. 

## Build requirements
You must build the Datasmith SDK
	With Git
		For that you need to get Unreal Engine git repository. Branch 4.27
		For mac OS : Replace the files with the ones provided vim-datasmith/Settings folder.
			"Engine/Source/Programs/Enterprise/Datasmith/DatasmithSDK/DatasmithSDK.Target.cs"
			"Engine/Source/Programs/Enterprise/Datasmith/DatasmithSDK/DatasmithSDK.Build.cs"
		Run "Setup.command"
		Run GenerateProjectFiles.command
		Open the UE4 project and build scheme "DatasmithSDK"
	With Perforce
		Just use the force (a lots) and everything will goes well.

In the file 
	In Vim-Datasmith.xcconfig - Edit "DatasmithSDKFolder" for the real path of your builded SDK

Normally you arer ready to build the converter application.

Good Luck

Richard Young

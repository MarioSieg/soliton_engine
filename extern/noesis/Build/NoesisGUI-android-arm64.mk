# GNU Make solution 

PROJECTS = NoesisApp Gui.XamlPlayer Samples.Integration Samples.HelloWorld Samples.Buttons Samples.Login Samples.QuestLog Samples.Scoreboard Samples.DataBinding Samples.ApplicationTutorial Samples.Touch Samples.Commands Samples.UserControl Samples.CustomControl Samples.BlendTutorial Samples.Menu3D Samples.Localization Samples.Inventory Samples.TicTacToe Samples.Gallery Samples.Rive Samples.BackgroundBlur Samples.BrushShaders

.SUFFIXES:
.PHONY: all clean help $(PROJECTS)

all: $(PROJECTS)

clean:
	@$(MAKE) --no-print-directory -C ..\Src\Projects\NoesisApp\android_arm64 clean
	@$(MAKE) --no-print-directory -C ..\Src\Projects\XamlPlayer\android_arm64 clean
	@$(MAKE) --no-print-directory -C ..\Src\Projects\Integration\android_arm64 clean
	@$(MAKE) --no-print-directory -C ..\Src\Projects\HelloWorld\android_arm64 clean
	@$(MAKE) --no-print-directory -C ..\Src\Projects\Buttons\android_arm64 clean
	@$(MAKE) --no-print-directory -C ..\Src\Projects\Login\android_arm64 clean
	@$(MAKE) --no-print-directory -C ..\Src\Projects\QuestLog\android_arm64 clean
	@$(MAKE) --no-print-directory -C ..\Src\Projects\Scoreboard\android_arm64 clean
	@$(MAKE) --no-print-directory -C ..\Src\Projects\DataBinding\android_arm64 clean
	@$(MAKE) --no-print-directory -C ..\Src\Projects\ApplicationTutorial\android_arm64 clean
	@$(MAKE) --no-print-directory -C ..\Src\Projects\Touch\android_arm64 clean
	@$(MAKE) --no-print-directory -C ..\Src\Projects\Commands\android_arm64 clean
	@$(MAKE) --no-print-directory -C ..\Src\Projects\UserControl\android_arm64 clean
	@$(MAKE) --no-print-directory -C ..\Src\Projects\CustomControl\android_arm64 clean
	@$(MAKE) --no-print-directory -C ..\Src\Projects\BlendTutorial\android_arm64 clean
	@$(MAKE) --no-print-directory -C ..\Src\Projects\Menu3D\android_arm64 clean
	@$(MAKE) --no-print-directory -C ..\Src\Projects\Localization\android_arm64 clean
	@$(MAKE) --no-print-directory -C ..\Src\Projects\Inventory\android_arm64 clean
	@$(MAKE) --no-print-directory -C ..\Src\Projects\TicTacToe\android_arm64 clean
	@$(MAKE) --no-print-directory -C ..\Src\Projects\Gallery\android_arm64 clean
	@$(MAKE) --no-print-directory -C ..\Src\Projects\Rive\android_arm64 clean
	@$(MAKE) --no-print-directory -C ..\Src\Projects\BackgroundBlur\android_arm64 clean
	@$(MAKE) --no-print-directory -C ..\Src\Projects\BrushShaders\android_arm64 clean

help:
	@echo Usage: make [CONFIG=name] [V=1] [target]
	@echo CONFIGURATIONS:
	@echo - Debug
	@echo - Profile
	@echo - Release
	@echo TARGETS:
	@echo - all [default]
	@echo - clean
	@echo - NoesisApp
	@echo - Gui.XamlPlayer
	@echo - Samples.Integration
	@echo - Samples.HelloWorld
	@echo - Samples.Buttons
	@echo - Samples.Login
	@echo - Samples.QuestLog
	@echo - Samples.Scoreboard
	@echo - Samples.DataBinding
	@echo - Samples.ApplicationTutorial
	@echo - Samples.Touch
	@echo - Samples.Commands
	@echo - Samples.UserControl
	@echo - Samples.CustomControl
	@echo - Samples.BlendTutorial
	@echo - Samples.Menu3D
	@echo - Samples.Localization
	@echo - Samples.Inventory
	@echo - Samples.TicTacToe
	@echo - Samples.Gallery
	@echo - Samples.Rive
	@echo - Samples.BackgroundBlur
	@echo - Samples.BrushShaders

NoesisApp: 
	@$(MAKE) --no-print-directory -C ..\Src\Projects\NoesisApp\android_arm64

Gui.XamlPlayer: NoesisApp
	@$(MAKE) --no-print-directory -C ..\Src\Projects\XamlPlayer\android_arm64

Samples.Integration: NoesisApp
	@$(MAKE) --no-print-directory -C ..\Src\Projects\Integration\android_arm64

Samples.HelloWorld: NoesisApp
	@$(MAKE) --no-print-directory -C ..\Src\Projects\HelloWorld\android_arm64

Samples.Buttons: NoesisApp
	@$(MAKE) --no-print-directory -C ..\Src\Projects\Buttons\android_arm64

Samples.Login: NoesisApp
	@$(MAKE) --no-print-directory -C ..\Src\Projects\Login\android_arm64

Samples.QuestLog: NoesisApp
	@$(MAKE) --no-print-directory -C ..\Src\Projects\QuestLog\android_arm64

Samples.Scoreboard: NoesisApp
	@$(MAKE) --no-print-directory -C ..\Src\Projects\Scoreboard\android_arm64

Samples.DataBinding: NoesisApp
	@$(MAKE) --no-print-directory -C ..\Src\Projects\DataBinding\android_arm64

Samples.ApplicationTutorial: NoesisApp
	@$(MAKE) --no-print-directory -C ..\Src\Projects\ApplicationTutorial\android_arm64

Samples.Touch: NoesisApp
	@$(MAKE) --no-print-directory -C ..\Src\Projects\Touch\android_arm64

Samples.Commands: NoesisApp
	@$(MAKE) --no-print-directory -C ..\Src\Projects\Commands\android_arm64

Samples.UserControl: NoesisApp
	@$(MAKE) --no-print-directory -C ..\Src\Projects\UserControl\android_arm64

Samples.CustomControl: NoesisApp
	@$(MAKE) --no-print-directory -C ..\Src\Projects\CustomControl\android_arm64

Samples.BlendTutorial: NoesisApp
	@$(MAKE) --no-print-directory -C ..\Src\Projects\BlendTutorial\android_arm64

Samples.Menu3D: NoesisApp
	@$(MAKE) --no-print-directory -C ..\Src\Projects\Menu3D\android_arm64

Samples.Localization: NoesisApp
	@$(MAKE) --no-print-directory -C ..\Src\Projects\Localization\android_arm64

Samples.Inventory: NoesisApp
	@$(MAKE) --no-print-directory -C ..\Src\Projects\Inventory\android_arm64

Samples.TicTacToe: NoesisApp
	@$(MAKE) --no-print-directory -C ..\Src\Projects\TicTacToe\android_arm64

Samples.Gallery: NoesisApp
	@$(MAKE) --no-print-directory -C ..\Src\Projects\Gallery\android_arm64

Samples.Rive: NoesisApp
	@$(MAKE) --no-print-directory -C ..\Src\Projects\Rive\android_arm64

Samples.BackgroundBlur: NoesisApp
	@$(MAKE) --no-print-directory -C ..\Src\Projects\BackgroundBlur\android_arm64

Samples.BrushShaders: NoesisApp
	@$(MAKE) --no-print-directory -C ..\Src\Projects\BrushShaders\android_arm64


# GNU Make solution 

PROJECTS = NoesisApp Gui.XamlPlayer Samples.Integration Samples.HelloWorld Samples.Buttons Samples.Login Samples.QuestLog Samples.Scoreboard Samples.DataBinding Samples.ApplicationTutorial Samples.Touch Samples.Commands Samples.UserControl Samples.CustomControl Samples.BlendTutorial Samples.Menu3D Samples.Localization Samples.Inventory Samples.TicTacToe Samples.Gallery Samples.Rive Samples.BackgroundBlur Samples.BrushShaders

.SUFFIXES:
.PHONY: all clean help $(PROJECTS)

all: $(PROJECTS)

clean:
	@$(MAKE) --no-print-directory -C ..\Src\Projects\NoesisApp\android_x86 clean
	@$(MAKE) --no-print-directory -C ..\Src\Projects\XamlPlayer\android_x86 clean
	@$(MAKE) --no-print-directory -C ..\Src\Projects\Integration\android_x86 clean
	@$(MAKE) --no-print-directory -C ..\Src\Projects\HelloWorld\android_x86 clean
	@$(MAKE) --no-print-directory -C ..\Src\Projects\Buttons\android_x86 clean
	@$(MAKE) --no-print-directory -C ..\Src\Projects\Login\android_x86 clean
	@$(MAKE) --no-print-directory -C ..\Src\Projects\QuestLog\android_x86 clean
	@$(MAKE) --no-print-directory -C ..\Src\Projects\Scoreboard\android_x86 clean
	@$(MAKE) --no-print-directory -C ..\Src\Projects\DataBinding\android_x86 clean
	@$(MAKE) --no-print-directory -C ..\Src\Projects\ApplicationTutorial\android_x86 clean
	@$(MAKE) --no-print-directory -C ..\Src\Projects\Touch\android_x86 clean
	@$(MAKE) --no-print-directory -C ..\Src\Projects\Commands\android_x86 clean
	@$(MAKE) --no-print-directory -C ..\Src\Projects\UserControl\android_x86 clean
	@$(MAKE) --no-print-directory -C ..\Src\Projects\CustomControl\android_x86 clean
	@$(MAKE) --no-print-directory -C ..\Src\Projects\BlendTutorial\android_x86 clean
	@$(MAKE) --no-print-directory -C ..\Src\Projects\Menu3D\android_x86 clean
	@$(MAKE) --no-print-directory -C ..\Src\Projects\Localization\android_x86 clean
	@$(MAKE) --no-print-directory -C ..\Src\Projects\Inventory\android_x86 clean
	@$(MAKE) --no-print-directory -C ..\Src\Projects\TicTacToe\android_x86 clean
	@$(MAKE) --no-print-directory -C ..\Src\Projects\Gallery\android_x86 clean
	@$(MAKE) --no-print-directory -C ..\Src\Projects\Rive\android_x86 clean
	@$(MAKE) --no-print-directory -C ..\Src\Projects\BackgroundBlur\android_x86 clean
	@$(MAKE) --no-print-directory -C ..\Src\Projects\BrushShaders\android_x86 clean

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
	@$(MAKE) --no-print-directory -C ..\Src\Projects\NoesisApp\android_x86

Gui.XamlPlayer: NoesisApp
	@$(MAKE) --no-print-directory -C ..\Src\Projects\XamlPlayer\android_x86

Samples.Integration: NoesisApp
	@$(MAKE) --no-print-directory -C ..\Src\Projects\Integration\android_x86

Samples.HelloWorld: NoesisApp
	@$(MAKE) --no-print-directory -C ..\Src\Projects\HelloWorld\android_x86

Samples.Buttons: NoesisApp
	@$(MAKE) --no-print-directory -C ..\Src\Projects\Buttons\android_x86

Samples.Login: NoesisApp
	@$(MAKE) --no-print-directory -C ..\Src\Projects\Login\android_x86

Samples.QuestLog: NoesisApp
	@$(MAKE) --no-print-directory -C ..\Src\Projects\QuestLog\android_x86

Samples.Scoreboard: NoesisApp
	@$(MAKE) --no-print-directory -C ..\Src\Projects\Scoreboard\android_x86

Samples.DataBinding: NoesisApp
	@$(MAKE) --no-print-directory -C ..\Src\Projects\DataBinding\android_x86

Samples.ApplicationTutorial: NoesisApp
	@$(MAKE) --no-print-directory -C ..\Src\Projects\ApplicationTutorial\android_x86

Samples.Touch: NoesisApp
	@$(MAKE) --no-print-directory -C ..\Src\Projects\Touch\android_x86

Samples.Commands: NoesisApp
	@$(MAKE) --no-print-directory -C ..\Src\Projects\Commands\android_x86

Samples.UserControl: NoesisApp
	@$(MAKE) --no-print-directory -C ..\Src\Projects\UserControl\android_x86

Samples.CustomControl: NoesisApp
	@$(MAKE) --no-print-directory -C ..\Src\Projects\CustomControl\android_x86

Samples.BlendTutorial: NoesisApp
	@$(MAKE) --no-print-directory -C ..\Src\Projects\BlendTutorial\android_x86

Samples.Menu3D: NoesisApp
	@$(MAKE) --no-print-directory -C ..\Src\Projects\Menu3D\android_x86

Samples.Localization: NoesisApp
	@$(MAKE) --no-print-directory -C ..\Src\Projects\Localization\android_x86

Samples.Inventory: NoesisApp
	@$(MAKE) --no-print-directory -C ..\Src\Projects\Inventory\android_x86

Samples.TicTacToe: NoesisApp
	@$(MAKE) --no-print-directory -C ..\Src\Projects\TicTacToe\android_x86

Samples.Gallery: NoesisApp
	@$(MAKE) --no-print-directory -C ..\Src\Projects\Gallery\android_x86

Samples.Rive: NoesisApp
	@$(MAKE) --no-print-directory -C ..\Src\Projects\Rive\android_x86

Samples.BackgroundBlur: NoesisApp
	@$(MAKE) --no-print-directory -C ..\Src\Projects\BackgroundBlur\android_x86

Samples.BrushShaders: NoesisApp
	@$(MAKE) --no-print-directory -C ..\Src\Projects\BrushShaders\android_x86


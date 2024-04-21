# GNU Make solution 

PROJECTS = NoesisApp Gui.XamlPlayer Samples.Integration Samples.HelloWorld Samples.Buttons Samples.Login Samples.QuestLog Samples.Scoreboard Samples.DataBinding Samples.ApplicationTutorial Samples.Commands Samples.UserControl Samples.CustomControl Samples.BlendTutorial Samples.Menu3D Samples.Localization Samples.Inventory Samples.TicTacToe Samples.Gallery Samples.CustomRender Samples.CustomAnimation Samples.BackgroundBlur Samples.BrushShaders

.SUFFIXES:
.PHONY: all clean help $(PROJECTS)

all: $(PROJECTS)

clean:
	@$(MAKE) --no-print-directory -C ../Src/Projects/NoesisApp/linux_arm clean
	@$(MAKE) --no-print-directory -C ../Src/Projects/XamlPlayer/linux_arm clean
	@$(MAKE) --no-print-directory -C ../Src/Projects/Integration/linux_arm clean
	@$(MAKE) --no-print-directory -C ../Src/Projects/HelloWorld/linux_arm clean
	@$(MAKE) --no-print-directory -C ../Src/Projects/Buttons/linux_arm clean
	@$(MAKE) --no-print-directory -C ../Src/Projects/Login/linux_arm clean
	@$(MAKE) --no-print-directory -C ../Src/Projects/QuestLog/linux_arm clean
	@$(MAKE) --no-print-directory -C ../Src/Projects/Scoreboard/linux_arm clean
	@$(MAKE) --no-print-directory -C ../Src/Projects/DataBinding/linux_arm clean
	@$(MAKE) --no-print-directory -C ../Src/Projects/ApplicationTutorial/linux_arm clean
	@$(MAKE) --no-print-directory -C ../Src/Projects/Commands/linux_arm clean
	@$(MAKE) --no-print-directory -C ../Src/Projects/UserControl/linux_arm clean
	@$(MAKE) --no-print-directory -C ../Src/Projects/CustomControl/linux_arm clean
	@$(MAKE) --no-print-directory -C ../Src/Projects/BlendTutorial/linux_arm clean
	@$(MAKE) --no-print-directory -C ../Src/Projects/Menu3D/linux_arm clean
	@$(MAKE) --no-print-directory -C ../Src/Projects/Localization/linux_arm clean
	@$(MAKE) --no-print-directory -C ../Src/Projects/Inventory/linux_arm clean
	@$(MAKE) --no-print-directory -C ../Src/Projects/TicTacToe/linux_arm clean
	@$(MAKE) --no-print-directory -C ../Src/Projects/Gallery/linux_arm clean
	@$(MAKE) --no-print-directory -C ../Src/Projects/CustomRender/linux_arm clean
	@$(MAKE) --no-print-directory -C ../Src/Projects/CustomAnimation/linux_arm clean
	@$(MAKE) --no-print-directory -C ../Src/Projects/BackgroundBlur/linux_arm clean
	@$(MAKE) --no-print-directory -C ../Src/Projects/BrushShaders/linux_arm clean

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
	@echo - Samples.Commands
	@echo - Samples.UserControl
	@echo - Samples.CustomControl
	@echo - Samples.BlendTutorial
	@echo - Samples.Menu3D
	@echo - Samples.Localization
	@echo - Samples.Inventory
	@echo - Samples.TicTacToe
	@echo - Samples.Gallery
	@echo - Samples.CustomRender
	@echo - Samples.CustomAnimation
	@echo - Samples.BackgroundBlur
	@echo - Samples.BrushShaders

NoesisApp: 
	@$(MAKE) --no-print-directory -C ../Src/Projects/NoesisApp/linux_arm

Gui.XamlPlayer: NoesisApp
	@$(MAKE) --no-print-directory -C ../Src/Projects/XamlPlayer/linux_arm

Samples.Integration: NoesisApp
	@$(MAKE) --no-print-directory -C ../Src/Projects/Integration/linux_arm

Samples.HelloWorld: NoesisApp
	@$(MAKE) --no-print-directory -C ../Src/Projects/HelloWorld/linux_arm

Samples.Buttons: NoesisApp
	@$(MAKE) --no-print-directory -C ../Src/Projects/Buttons/linux_arm

Samples.Login: NoesisApp
	@$(MAKE) --no-print-directory -C ../Src/Projects/Login/linux_arm

Samples.QuestLog: NoesisApp
	@$(MAKE) --no-print-directory -C ../Src/Projects/QuestLog/linux_arm

Samples.Scoreboard: NoesisApp
	@$(MAKE) --no-print-directory -C ../Src/Projects/Scoreboard/linux_arm

Samples.DataBinding: NoesisApp
	@$(MAKE) --no-print-directory -C ../Src/Projects/DataBinding/linux_arm

Samples.ApplicationTutorial: NoesisApp
	@$(MAKE) --no-print-directory -C ../Src/Projects/ApplicationTutorial/linux_arm

Samples.Commands: NoesisApp
	@$(MAKE) --no-print-directory -C ../Src/Projects/Commands/linux_arm

Samples.UserControl: NoesisApp
	@$(MAKE) --no-print-directory -C ../Src/Projects/UserControl/linux_arm

Samples.CustomControl: NoesisApp
	@$(MAKE) --no-print-directory -C ../Src/Projects/CustomControl/linux_arm

Samples.BlendTutorial: NoesisApp
	@$(MAKE) --no-print-directory -C ../Src/Projects/BlendTutorial/linux_arm

Samples.Menu3D: NoesisApp
	@$(MAKE) --no-print-directory -C ../Src/Projects/Menu3D/linux_arm

Samples.Localization: NoesisApp
	@$(MAKE) --no-print-directory -C ../Src/Projects/Localization/linux_arm

Samples.Inventory: NoesisApp
	@$(MAKE) --no-print-directory -C ../Src/Projects/Inventory/linux_arm

Samples.TicTacToe: NoesisApp
	@$(MAKE) --no-print-directory -C ../Src/Projects/TicTacToe/linux_arm

Samples.Gallery: NoesisApp
	@$(MAKE) --no-print-directory -C ../Src/Projects/Gallery/linux_arm

Samples.CustomRender: NoesisApp
	@$(MAKE) --no-print-directory -C ../Src/Projects/CustomRender/linux_arm

Samples.CustomAnimation: NoesisApp
	@$(MAKE) --no-print-directory -C ../Src/Projects/CustomAnimation/linux_arm

Samples.BackgroundBlur: NoesisApp
	@$(MAKE) --no-print-directory -C ../Src/Projects/BackgroundBlur/linux_arm

Samples.BrushShaders: NoesisApp
	@$(MAKE) --no-print-directory -C ../Src/Projects/BrushShaders/linux_arm


# GNU Make solution 

PROJECTS = NoesisApp Gui.XamlPlayer Samples.Integration Samples.IntegrationGLUT Samples.HelloWorld Samples.Buttons Samples.Login Samples.QuestLog Samples.Scoreboard Samples.DataBinding Samples.ApplicationTutorial Samples.Commands Samples.UserControl Samples.CustomControl Samples.BlendTutorial Samples.Menu3D Samples.Localization Samples.Inventory Samples.TicTacToe Samples.Gallery Samples.CustomRender Samples.CustomAnimation Samples.BackgroundBlur Samples.BrushShaders

.SUFFIXES:
.PHONY: all clean help $(PROJECTS)

all: $(PROJECTS)

clean:
	@$(MAKE) --no-print-directory -C ../Src/Projects/NoesisApp/linux_x86_64 clean
	@$(MAKE) --no-print-directory -C ../Src/Projects/XamlPlayer/linux_x86_64 clean
	@$(MAKE) --no-print-directory -C ../Src/Projects/Integration/linux_x86_64 clean
	@$(MAKE) --no-print-directory -C ../Src/Projects/IntegrationGLUT/linux_x86_64 clean
	@$(MAKE) --no-print-directory -C ../Src/Projects/HelloWorld/linux_x86_64 clean
	@$(MAKE) --no-print-directory -C ../Src/Projects/Buttons/linux_x86_64 clean
	@$(MAKE) --no-print-directory -C ../Src/Projects/Login/linux_x86_64 clean
	@$(MAKE) --no-print-directory -C ../Src/Projects/QuestLog/linux_x86_64 clean
	@$(MAKE) --no-print-directory -C ../Src/Projects/Scoreboard/linux_x86_64 clean
	@$(MAKE) --no-print-directory -C ../Src/Projects/DataBinding/linux_x86_64 clean
	@$(MAKE) --no-print-directory -C ../Src/Projects/ApplicationTutorial/linux_x86_64 clean
	@$(MAKE) --no-print-directory -C ../Src/Projects/Commands/linux_x86_64 clean
	@$(MAKE) --no-print-directory -C ../Src/Projects/UserControl/linux_x86_64 clean
	@$(MAKE) --no-print-directory -C ../Src/Projects/CustomControl/linux_x86_64 clean
	@$(MAKE) --no-print-directory -C ../Src/Projects/BlendTutorial/linux_x86_64 clean
	@$(MAKE) --no-print-directory -C ../Src/Projects/Menu3D/linux_x86_64 clean
	@$(MAKE) --no-print-directory -C ../Src/Projects/Localization/linux_x86_64 clean
	@$(MAKE) --no-print-directory -C ../Src/Projects/Inventory/linux_x86_64 clean
	@$(MAKE) --no-print-directory -C ../Src/Projects/TicTacToe/linux_x86_64 clean
	@$(MAKE) --no-print-directory -C ../Src/Projects/Gallery/linux_x86_64 clean
	@$(MAKE) --no-print-directory -C ../Src/Projects/CustomRender/linux_x86_64 clean
	@$(MAKE) --no-print-directory -C ../Src/Projects/CustomAnimation/linux_x86_64 clean
	@$(MAKE) --no-print-directory -C ../Src/Projects/BackgroundBlur/linux_x86_64 clean
	@$(MAKE) --no-print-directory -C ../Src/Projects/BrushShaders/linux_x86_64 clean

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
	@echo - Samples.IntegrationGLUT
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
	@$(MAKE) --no-print-directory -C ../Src/Projects/NoesisApp/linux_x86_64

Gui.XamlPlayer: NoesisApp
	@$(MAKE) --no-print-directory -C ../Src/Projects/XamlPlayer/linux_x86_64

Samples.Integration: NoesisApp
	@$(MAKE) --no-print-directory -C ../Src/Projects/Integration/linux_x86_64

Samples.IntegrationGLUT: NoesisApp
	@$(MAKE) --no-print-directory -C ../Src/Projects/IntegrationGLUT/linux_x86_64

Samples.HelloWorld: NoesisApp
	@$(MAKE) --no-print-directory -C ../Src/Projects/HelloWorld/linux_x86_64

Samples.Buttons: NoesisApp
	@$(MAKE) --no-print-directory -C ../Src/Projects/Buttons/linux_x86_64

Samples.Login: NoesisApp
	@$(MAKE) --no-print-directory -C ../Src/Projects/Login/linux_x86_64

Samples.QuestLog: NoesisApp
	@$(MAKE) --no-print-directory -C ../Src/Projects/QuestLog/linux_x86_64

Samples.Scoreboard: NoesisApp
	@$(MAKE) --no-print-directory -C ../Src/Projects/Scoreboard/linux_x86_64

Samples.DataBinding: NoesisApp
	@$(MAKE) --no-print-directory -C ../Src/Projects/DataBinding/linux_x86_64

Samples.ApplicationTutorial: NoesisApp
	@$(MAKE) --no-print-directory -C ../Src/Projects/ApplicationTutorial/linux_x86_64

Samples.Commands: NoesisApp
	@$(MAKE) --no-print-directory -C ../Src/Projects/Commands/linux_x86_64

Samples.UserControl: NoesisApp
	@$(MAKE) --no-print-directory -C ../Src/Projects/UserControl/linux_x86_64

Samples.CustomControl: NoesisApp
	@$(MAKE) --no-print-directory -C ../Src/Projects/CustomControl/linux_x86_64

Samples.BlendTutorial: NoesisApp
	@$(MAKE) --no-print-directory -C ../Src/Projects/BlendTutorial/linux_x86_64

Samples.Menu3D: NoesisApp
	@$(MAKE) --no-print-directory -C ../Src/Projects/Menu3D/linux_x86_64

Samples.Localization: NoesisApp
	@$(MAKE) --no-print-directory -C ../Src/Projects/Localization/linux_x86_64

Samples.Inventory: NoesisApp
	@$(MAKE) --no-print-directory -C ../Src/Projects/Inventory/linux_x86_64

Samples.TicTacToe: NoesisApp
	@$(MAKE) --no-print-directory -C ../Src/Projects/TicTacToe/linux_x86_64

Samples.Gallery: NoesisApp
	@$(MAKE) --no-print-directory -C ../Src/Projects/Gallery/linux_x86_64

Samples.CustomRender: NoesisApp
	@$(MAKE) --no-print-directory -C ../Src/Projects/CustomRender/linux_x86_64

Samples.CustomAnimation: NoesisApp
	@$(MAKE) --no-print-directory -C ../Src/Projects/CustomAnimation/linux_x86_64

Samples.BackgroundBlur: NoesisApp
	@$(MAKE) --no-print-directory -C ../Src/Projects/BackgroundBlur/linux_x86_64

Samples.BrushShaders: NoesisApp
	@$(MAKE) --no-print-directory -C ../Src/Projects/BrushShaders/linux_x86_64


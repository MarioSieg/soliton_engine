# GNU Make solution 

PROJECTS = NoesisApp Samples.IntegrationGLUT Samples.HelloWorld Samples.Buttons Samples.TicTacToe Samples.QuestLog Samples.Scoreboard

.SUFFIXES:
.PHONY: all clean help $(PROJECTS)

all: $(PROJECTS)

clean:
	@$(MAKE) --no-print-directory -C ..\Src\Projects\NoesisApp\wasm clean
	@$(MAKE) --no-print-directory -C ..\Src\Projects\IntegrationGLUT\wasm clean
	@$(MAKE) --no-print-directory -C ..\Src\Projects\HelloWorld\wasm clean
	@$(MAKE) --no-print-directory -C ..\Src\Projects\Buttons\wasm clean
	@$(MAKE) --no-print-directory -C ..\Src\Projects\TicTacToe\wasm clean
	@$(MAKE) --no-print-directory -C ..\Src\Projects\QuestLog\wasm clean
	@$(MAKE) --no-print-directory -C ..\Src\Projects\Scoreboard\wasm clean

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
	@echo - Samples.IntegrationGLUT
	@echo - Samples.HelloWorld
	@echo - Samples.Buttons
	@echo - Samples.TicTacToe
	@echo - Samples.QuestLog
	@echo - Samples.Scoreboard

NoesisApp: 
	@$(MAKE) --no-print-directory -C ..\Src\Projects\NoesisApp\wasm

Samples.IntegrationGLUT: NoesisApp
	@$(MAKE) --no-print-directory -C ..\Src\Projects\IntegrationGLUT\wasm

Samples.HelloWorld: NoesisApp
	@$(MAKE) --no-print-directory -C ..\Src\Projects\HelloWorld\wasm

Samples.Buttons: NoesisApp
	@$(MAKE) --no-print-directory -C ..\Src\Projects\Buttons\wasm

Samples.TicTacToe: NoesisApp
	@$(MAKE) --no-print-directory -C ..\Src\Projects\TicTacToe\wasm

Samples.QuestLog: NoesisApp
	@$(MAKE) --no-print-directory -C ..\Src\Projects\QuestLog\wasm

Samples.Scoreboard: NoesisApp
	@$(MAKE) --no-print-directory -C ..\Src\Projects\Scoreboard\wasm


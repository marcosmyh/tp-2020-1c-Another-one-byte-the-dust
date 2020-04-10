.PHONY: default all clean Team Broker GameBoy GameCard

include Paths.Makefile

default: main

Team:
	@echo 
	@echo "##################################################"
	@echo "#                                                #"
	@echo "#               Building Team                    #"
	@echo "#                                                #"
	@echo "##################################################"
	@echo 
	@cd $(Team_PATH) && $(MAKE)
	
Broker:
	@echo 
	@echo "##################################################"
	@echo "#                                                #"
	@echo "#               Building Broker                  #"
	@echo "#                                                #"
	@echo "##################################################"
	@echo 
	@cd $(Broker_PATH) && $(MAKE)
	
GameBoy:
	@echo 
	@echo "##################################################"
	@echo "#                                                #"
	@echo "#               Building GameBoy                 #"
	@echo "#                                                #"
	@echo "##################################################"
	@echo 
	@cd $(GameBoy_PATH) && $(MAKE)
	
GameCard:
	@echo 
	@echo "##################################################"
	@echo "#                                                #"
	@echo "#               Building GameCard                #"
	@echo "#                                                #"
	@echo "##################################################"
	@echo 
	@cd $(GameCard_PATH) && $(MAKE)

main: Team Broker GameBoy GameCard

all: main

clean: 
	@cd $(Team_PATH) && $(MAKE) clean
	@cd $(Broker_PATH) && $(MAKE) clean
	@cd $(GameBoy_PATH) && $(MAKE) clean
	@cd $(GameCard_PATH) && $(MAKE) clean
    
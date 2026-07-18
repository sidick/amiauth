# AmiAuth GUI test harness (docker-amiberry). See README.md.
#
#   make up      seed config-home + start the container (http://localhost:8443)
#   make down    stop and remove the container (keeps config-home)
#   make logs    follow the container logs
#   make seed    (re)copy amiauth.uae -> default.uae and the ROM into config-home
#   make clean   down + delete config-home

COMPOSE ?= docker compose
CONFIG  := config-home

include .env
export

.PHONY: up down restart logs ps seed clean

up: seed ## seed then start the container in the background
	$(COMPOSE) up -d
	@echo "Open http://localhost:8443  (amiberry / amiberry)"

down: ## stop and remove the container (keeps config-home)
	$(COMPOSE) down

restart: down up ## reseed and restart

logs: ## follow the container logs
	$(COMPOSE) logs -f

ps: ## container status
	$(COMPOSE) ps

# Seed the amiberry HOME: the amiauth config becomes default.uae (the config
# amiberry loads on launch; it carries use_gui=no so it boots straight in), and
# the kickstart is copied in (ROMs must live under /config, which can't be a
# nested bind mount on macOS). config-home is a plain host dir, gitignored.
seed: ## (re)copy the config + ROM into config-home
	@test -n "$(AMIAUTH_ROM)" || { echo "AMIAUTH_ROM unset; cp .env.example .env"; exit 1; }
	mkdir -p $(CONFIG)/Amiberry/Configurations $(CONFIG)/Amiberry/ROMs
	cp amiauth.uae $(CONFIG)/Amiberry/Configurations/default.uae
	@test -f "$(AMIAUTH_ROM)" || { echo "ROM not found: $(AMIAUTH_ROM)"; exit 1; }
	cp "$(AMIAUTH_ROM)" $(CONFIG)/Amiberry/ROMs/A1200.47.115.rom

clean: down ## down + delete config-home
	rm -rf $(CONFIG)

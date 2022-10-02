APP:=rootme
FILE:=*.c src/*.c
FLAGS:=-I. -L. -Iinclude -Linclude -Iheaders -Lheaders

IP:=le-carote
ADDR:=mobile@$(IP)
PORT:=44
SSH:=ssh
UPLOAD_DIR:=/var/mobile/Downloads

# end of configurable variables
green=\033[0;32m
red=\033[0;31m
blue=\033[0;34m
end=\033[0m
arrow=$(red)=> $(end)
MUTE= 2>/dev/null; true

RERUN=$(MAKE) --no-print-directory

all: build sign
do: build sign upload run clean

build:
	@echo "$(arrow)$(green)Compiling ${FILE} to ${APP}$(end)"
	@aarch64-apple-darwin-clang ${FLAGS} ${FILE} -o ${APP}

sign:
	@echo "$(arrow)$(green)Signing ${APP}$(red)"
	@chmod +x ${APP}
	@trustcache create $(APP).tc $(APP)
# @ldid -Sent.xml ${APP}

upload:
	@echo "$(arrow)$(green)Uploading ${APP}$(end)"
	@scp -P $(PORT) -q ${APP} ${APP}.tc ${ADDR}:${UPLOAD_DIR}

run:
	@if ! ssh -p $(PORT) $(ADDR) "stat $(UPLOAD_DIR)/$(APP)" >/dev/null; then (echo "$(arrow)$(red)Build the app first!$(end)"; $(RERUN); $(RERUN) upload); fi

	@echo "$(arrow)$(green)Running ${APP}$(end)"
	-@ssh -p $(PORT) $(ADDR) "cd $(UPLOAD_DIR); ldid -Sent.xml $(APP); printf '\n'; ./${APP}; printf '\n'"

clean:
	@echo "$(arrow)$(green)Cleaning up!$(end)"
	@rm ${APP} $(APP).tc

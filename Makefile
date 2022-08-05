APP:=rootme
FILE:=*.c src/*.c
FLAGS:=-I. -Iinclude -Linclude -L.

IP:=le-carote
ADDR:=mobile@$(IP)
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
# @ldid -Sent.xml ${APP}

upload:
	@echo "$(arrow)$(green)Uploading ${APP}$(end)"
	@scp -q ${APP} ${ADDR}:${UPLOAD_DIR}

run:
	@if ! ssh $(ADDR) "stat $(UPLOAD_DIR)/$(APP)" >/dev/null; then (echo "$(arrow)$(red)Build the app first!$(end)"; $(RERUN); $(RERUN) upload); fi

	@echo "$(arrow)$(green)Running ${APP}$(end)"
	-@ssh $(ADDR) "cd $(UPLOAD_DIR); ldid -Sent.xml $(APP); printf '\n'; ./${APP}; printf '\n'"

clean:
	@echo "$(arrow)$(green)Cleaning up!$(end)"
	@rm ${APP}

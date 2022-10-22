#needed for ld to work
export PATH := ${DIYCOMPILE}/toolchain/iphone/bin:$(PATH)

APP:=rootme
FILE:=*.c src/*.c
ENT:=

INCLUDE:=-I. -L. -Iinclude -Linclude -Iheaders -Lheaders
ARCH=-target arm64-apple-ios14.4
CUSTOM=-isysroot "/home/hamdan/iOS/Theos/sdks/iPhoneOS14.5.sdk"
#-I. -Iinclude -Linclude -L.

IP:=le-carote
ADDR:=root@$(IP)
PORT:=44
UPLOAD_DIR:=/

# end of configurable variables
green=\033[0;32m
red=\033[0;31m
blue=\033[0;34m
end=\033[0m
arrow=$(red)=> $(end)
MUTE= 2>/dev/null; true

RERUN=$(MAKE) --no-print-directory

FLAGS=$(INCLUDE) $(ARCH) $(CUSTOM)

all: build sign
do: build sign upload

build:
	@echo "$(arrow)$(green)Compiling ${FILE} to ${APP}$(end)"
	@aarch64-apple-darwin-clang ${FLAGS} ${FILE} -o ${APP}

sign:
	@echo "$(arrow)$(green)Signing ${APP}$(red)"
	@chmod +x ${APP}
	@ldid -S$(ENT) $(APP)
	@trustcache create $(APP).tc $(APP)

upload:
	@echo "$(arrow)$(green)Uploading ${APP}$(end)"
	-@ssh -p $(PORT) $(ADDR) "rm $(UPLOAD_DIR)/$(APP)"
	@scp -P $(PORT) ${APP} ${ADDR}:${UPLOAD_DIR}
	@scp -P $(PORT) $(APP).tc $(ADDR):/tmp
	-ssh -p $(PORT) $(ADDR) "/.Fugu14Untether/jailbreakd loadTC /tmp/$(APP).tc && rm /tmp/$(APP).tc"

run:
	@if ! ssh -p $(PORT) $(ADDR) "stat $(UPLOAD_DIR)/$(APP)" >/dev/null; then (echo "$(arrow)$(red)Build the app first!$(end)"; $(RERUN); $(RERUN) upload); fi

	@echo "$(arrow)$(green)Running ${APP}$(red)"
	ssh -p $(PORT) $(ADDR) "$(UPLOAD_DIR)/${APP}"

clean:
	@echo "$(arrow)$(green)Cleaning up!$(end)"
	@rm ${APP} $(APP).tc

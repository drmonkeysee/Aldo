OS := $(shell uname)
ifneq ($(OS), Darwin)
SHELL := $(shell which bash)
endif

SDL_VERSION := 3.2.28
SDL_DMG := SDL3-$(SDL_VERSION).dmg
SDL_HTTP := https://github.com/libsdl-org/SDL/releases/download/release-$(SDL_VERSION)

IMGUI_VERSION := 1.92.5
IMGUI_TARGET := imgui
IMGUI_HTTP := https://github.com/ocornut/imgui/archive/refs/tags/v$(IMGUI_VERSION).tar.gz
IMGUI_SRC := $(IMGUI_TARGET)-$(IMGUI_VERSION)

.PHONY: clean fetch

ifeq ($(OS), Darwin)
fetch: $(SDL_DMG)
endif
fetch: $(IMGUI_TARGET)

clean:
	find . -maxdepth 1 -not -name Makefile -and -not -name '.*' -exec $(RM) -rv {} +

$(SDL_DMG):
	curl -L -O '$(SDL_HTTP)/$@'

$(IMGUI_TARGET):
	mkdir -p $@
	cd $@ && \
	curl -L -O '$(IMGUI_HTTP)' && \
	tar -xzf $(notdir $(IMGUI_HTTP)) && \
	cp -v $(IMGUI_SRC)/*.{h,cpp} . && \
	for impl in sdl3 sdlrenderer3; do \
		cp -v $(IMGUI_SRC)/backends/imgui_impl_$${impl}.h .; \
	done && \
	$(RM) -r $(IMGUI_SRC)

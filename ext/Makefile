SDL_VERSION := 2.26.0
SDL_DMG := SDL2-$(SDL_VERSION).dmg
SDL_HTTP := https://github.com/libsdl-org/SDL/releases/download/release-$(SDL_VERSION)

IMGUI_VERSION := 1.89.1
IMGUI_TARGET := imgui
IMGUI_HTTP := https://github.com/ocornut/imgui/archive/refs/tags/v$(IMGUI_VERSION).tar.gz
IMGUI_SRC := $(IMGUI_TARGET)-$(IMGUI_VERSION)

.PHONY: clean fetch

fetch: $(SDL_DMG) $(IMGUI_TARGET)

clean:
	find . -not -name Makefile -and -not -name '.*' -maxdepth 1 -exec $(RM) -rv {} +

$(SDL_DMG):
	curl -L -O '$(SDL_HTTP)/$@'

$(IMGUI_TARGET):
	mkdir -p $@
	cd $@ && \
	curl -L -O '$(IMGUI_HTTP)' && \
	tar -xzf $(notdir $(IMGUI_HTTP)) && \
	cp -v $(IMGUI_SRC)/*.{h,cpp} . && \
	for impl in sdl sdlrenderer; do \
		cp -v $(IMGUI_SRC)/backends/imgui_impl_$${impl}.h . && \
		sed 's/\(#include <SDL\)/\12\/SDL/' $(IMGUI_SRC)/backends/imgui_impl_$${impl}.cpp \
			> imgui_impl_$${impl}.cpp; \
	done && \
	$(RM) -r $(IMGUI_SRC)

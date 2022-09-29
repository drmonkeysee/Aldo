SDL_VERSION := 2.24.0
SDL_DMG := SDL2-$(SDL_VERSION).dmg
SDL_HTTP := https://github.com/libsdl-org/SDL/releases/download/release-$(SDL_VERSION)

IMGUI_VERSION := 1.88
IMGUI_HTTP := https://github.com/ocornut/imgui/archive/refs/tags/v$(IMGUI_VERSION).tar.gz
IMGUI_SRC := imgui-$(IMGUI_VERSION)

download: $(SDL_DMG) imgui

$(SDL_DMG):
	curl -L -O '$(SDL_HTTP)/$@'

imgui:
	mkdir -p $@
	cd $@; \
	curl -L -O '$(IMGUI_HTTP)'; \
	tar -xzf $(notdir $(IMGUI_HTTP)); \
	cp -v $(IMGUI_SRC)/*.{h,cpp} .; \
	cp -v $(IMGUI_SRC)/backends/imgui_impl_{sdl,sdlrenderer}.{h,cpp} .; \
	rm -r $(IMGUI_SRC)

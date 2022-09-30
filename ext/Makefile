SDL_VERSION := 2.24.0
SDL_DMG := SDL2-$(SDL_VERSION).dmg
SDL_HTTP := https://github.com/libsdl-org/SDL/releases/download/release-$(SDL_VERSION)

IMGUI_VERSION := 1.88
IMGUI_TARGET := imgui
IMGUI_HTTP := https://github.com/ocornut/imgui/archive/refs/tags/v$(IMGUI_VERSION).tar.gz
IMGUI_SRC := $(IMGUI_TARGET)-$(IMGUI_VERSION)

.PHONY: clean fetch

fetch: $(SDL_DMG) $(IMGUI_TARGET)

clean:
	$(RM) $(SDL_DMG)
	$(RM) -r SDL2.framework
	$(RM) -r $(IMGUI_TARGET)

$(SDL_DMG):
	curl -L -O '$(SDL_HTTP)/$@'
	$(info ================================)
	$(info Next steps...)
	$(info Mount $(SDL_DMG))
	$(info Copy SDL2.framework to $(CURDIR))
	$(info ================================)

$(IMGUI_TARGET):
	mkdir -p $@
	cd $@; \
	curl -L -O '$(IMGUI_HTTP)'; \
	tar -xzf $(notdir $(IMGUI_HTTP)); \
	cp -v $(IMGUI_SRC)/*.{h,cpp} .; \
	cp -v $(IMGUI_SRC)/backends/imgui_impl_{sdl,sdlrenderer}.{h,cpp} .; \
	$(RM) -r $(IMGUI_SRC)

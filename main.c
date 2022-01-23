#define _CRT_SECURE_NO_DEPRECATE

#include<stdio.h>
#include<string.h>

#include<SDL.h>
#include<SDL_ttf.h>

struct GlyphPosition {
    int x;
    int y;
};

int GlyphWidth, GlyphHeight;
SDL_Texture *GlyphAtlas;
struct GlyphPosition GlyphMapping[128];

SDL_Texture *CreateGlyphAtlas(SDL_Renderer* Renderer, TTF_Font* Font)
{
	// Create texture and set as render target
	SDL_Texture* Texture = SDL_CreateTexture(
		Renderer,
		SDL_PIXELFORMAT_RGBA32,
		SDL_TEXTUREACCESS_TARGET,
		2048,
		2048
	);
	SDL_SetRenderTarget(Renderer, Texture);

	// Loop over all characters and write onto the texture
	// 128 -> amount of ascii characters
	for (int i = 0; i < 128; i++) {
		char c = (char)i;

		SDL_Rect Rect = { i * GlyphWidth, 0, GlyphWidth, GlyphHeight };
		SDL_Color Color = { 255, 255, 255, 255 };
		SDL_Surface* Surface = TTF_RenderGlyph_Blended(Font, c, Color);
		SDL_Texture* CharTexture = SDL_CreateTextureFromSurface(Renderer, Surface);

		SDL_RenderCopy(Renderer, CharTexture, NULL, &Rect);
		struct GlyphPosition Position = { Rect.x, Rect.y };
		GlyphMapping[i] = Position;
	}
	return Texture;
}

struct CursorToTextMapping
{
	int Line;
	int Index;
	int TotalCharacters;
};

struct CursorToTextMapping FindTextIndexUnderCursor(char* Text, int CursorX, int CursorY)
{
	int Line = CursorY / GlyphHeight;
	int AmountOfCharactersTillCursorPosition = CursorX / GlyphWidth;
	int AmountOfCharactersSinceNewLine = 0;

	int FoundIndex = strlen(Text);
	int TotalCharacters = 0;
	int Lines = 0;
	for (int i = 0; i < strlen(Text); i++)
	{
		char c = Text[i];
		if (c == '\n')
		{
			Lines++;
			AmountOfCharactersSinceNewLine = 0;
			continue;
		}
		if (Lines == Line && AmountOfCharactersSinceNewLine++ == AmountOfCharactersTillCursorPosition)
		{
			FoundIndex = i;
			break;
		}
		TotalCharacters++;
	}

	struct CursorToTextMapping Mapping;
	Mapping.Line = Line;
	Mapping.Index = FoundIndex;
	Mapping.TotalCharacters = TotalCharacters;
	return Mapping;
}


int main(int argc, char* argv)
{
	SDL_Init(SDL_INIT_VIDEO);

	SDL_Window *Window = SDL_CreateWindow("SDL Text Editor", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1920, 1080, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	if (Window == NULL)
	{
		return 1;
	}

	if (TTF_Init() == -1)
	{
		fprintf(stderr, "Failed to init SDL_ttf: %s", TTF_GetError());
		return 1;
	}

	TTF_Font *Font = TTF_OpenFont("font.ttf", 15);
	if (!Font)
	{
		fprintf(stderr, "Failed to load font: %s", TTF_GetError());
		return 1;
	}
	TTF_SizeText(Font, "a", &GlyphWidth, &GlyphHeight);

	int Running = 1;

	int TextSize = 12;
	char *Text = (char*)malloc(sizeof(char) * TextSize);
	strcpy(Text, "Hello World");

	SDL_Renderer *Renderer = SDL_CreateRenderer(Window, -1, SDL_RENDERER_ACCELERATED);

	GlyphAtlas = CreateGlyphAtlas(Renderer, Font);
	SDL_SetRenderTarget(Renderer, NULL);

	int CursorX = 0;
	int CursorY = 0;

	int WindowWidth = 0;
	int WindowHeight = 0;

	while (Running)
	{
		SDL_GetWindowSize(Window, &WindowWidth, &WindowHeight);

		SDL_Event e;
		while (SDL_PollEvent(&e))
		{
			switch (e.type)
			{
			case SDL_QUIT:
				Running = 0;
				break;
			case SDL_KEYDOWN:
				if (e.key.keysym.scancode == SDL_SCANCODE_BACKSPACE)
				{
					struct CursorToTextMapping Mapping = FindTextIndexUnderCursor(Text, CursorX, CursorY);
					char RemovedCharacter = Text[Mapping.Index - 1];

					int XIndexPreviousLine = 0;
					if (RemovedCharacter == '\n')
					{
						int NewLine = 0;
						int Characters = 0;
						for (int i = 0; i <= strlen(Text); i++)
						{
							char c = Text[i];
							if (c == '\n')
							{
								if (++NewLine == Mapping.Line) break;
								Characters = 0;
								continue;
							}
							Characters++;
						}
						XIndexPreviousLine = Characters;
					}

					for (int i = Mapping.Index - 1; i <= strlen(Text); i++)
					{
						Text[i] = Text[i + 1];
					}

					if (RemovedCharacter == '\n')
					{
						CursorY -= GlyphHeight;
						CursorX = XIndexPreviousLine * GlyphWidth;
					}
					else 
					{
						CursorX -= GlyphWidth;
					}
				}
				if (e.key.keysym.scancode == SDL_SCANCODE_RETURN)
				{
					if (strlen(Text) + strlen("\n") >= TextSize)
					{
						TextSize = TextSize + 5 + strlen("\n");
						Text = (char*)realloc(Text, TextSize);
					}

					struct CursorToTextMapping Mapping = FindTextIndexUnderCursor(Text, CursorX, CursorY);
					if (Mapping.Index <= strlen(Text))
					{
						int AddedTextLength = strlen("\n");
						for (int i = strlen(Text) + 1; i >= Mapping.Index + AddedTextLength; i--)
						{
							Text[i] = Text[i - 1];
						}

						for (int i = 0; i < AddedTextLength; i++)
						{
							Text[Mapping.Index + i] = "\n"[i];
						}
					}

					// Move cursor to next line
					CursorX = 0;
					CursorY += GlyphHeight;
				}
				if (e.key.keysym.scancode == SDL_SCANCODE_UP)
				{
					CursorY -= GlyphHeight;
					if (CursorY < 0) CursorY = 0;
				}
				if (e.key.keysym.scancode == SDL_SCANCODE_DOWN)
				{
					CursorY += GlyphHeight;
					if (CursorY > WindowHeight) CursorY = WindowHeight;
				}
				if (e.key.keysym.scancode == SDL_SCANCODE_LEFT)
				{
					CursorX -= GlyphWidth;
					if (CursorX < 0) CursorX = 0;
				}
				if (e.key.keysym.scancode == SDL_SCANCODE_RIGHT)
				{
					CursorX += GlyphWidth;
					if (CursorX > WindowWidth) CursorX = WindowWidth;
				}
				break;
			case SDL_TEXTINPUT:
				// TODO(Brysen): BUG/When on the last cursor position of any line it uses the very end of the text as removal and input index
				if (strlen(Text) + strlen(e.text.text) >= TextSize)
				{
					TextSize = TextSize + 5 + strlen(e.text.text);
					Text = (char*)realloc(Text, TextSize);
				}

				struct CursorToTextMapping Mapping = FindTextIndexUnderCursor(Text, CursorX, CursorY);
				if (Mapping.Index <= strlen(Text))
				{
					int AddedTextLength = strlen(e.text.text);
					for (int i = strlen(Text) + 1; i >= Mapping.Index + AddedTextLength; i--)
					{
						Text[i] = Text[i - 1];
					}

					for (int i = 0; i < AddedTextLength; i++)
					{
						Text[Mapping.Index + i] = e.text.text[i];
					}
				}

				// Move cursor along
				CursorX += strlen(e.text.text) * GlyphWidth;

				break;
			}
		}

		SDL_SetRenderDrawColor(Renderer, 4, 32, 39, 255);
		SDL_RenderClear(Renderer);

		// Blend background
		SDL_SetTextureBlendMode(GlyphAtlas, SDL_BLENDMODE_BLEND);
		// Change text color
		SDL_SetTextureColorMod(GlyphAtlas, 189, 179, 149);

		int Carriage = 0;
		int Line = 0;
		for (int i = 0; i < TextSize; i++)
		{
			char c = Text[i];
			if (c == '\n')
			{
				Carriage = 0;
				Line++;
				continue;
			}

            struct GlyphPosition Position = GlyphMapping[(int)c];
            SDL_Rect SourceRect = { Position.x, Position.y, GlyphWidth, GlyphHeight };
			SDL_Rect DestRect = { Carriage++ * GlyphWidth, GlyphHeight * Line, GlyphWidth, GlyphHeight };
            SDL_RenderCopy(Renderer, GlyphAtlas, &SourceRect, &DestRect);
		}

		int CurrentLine = CursorY / GlyphHeight;

		// Limit cursor on lines with characters
		int AmountOfNewLines = 0;
		for (int i = 0; i < strlen(Text); i++)
		{
			if (Text[i] == '\n') AmountOfNewLines++;
		}

		if (CursorY > GlyphHeight * AmountOfNewLines)
		{
			CursorY = AmountOfNewLines * GlyphHeight;
		}

		if (CursorX < 0) CursorX = 0;
		if (CursorY < 0) CursorY = 0;

		// Limit to last character on CurrentLine
		int NewLines = 0;
		int CharactersOnLine = 0;
		for (int i = 0; i < strlen(Text); i++)
		{
			char c = Text[i];
			if (c == '\n')
			{
				if (NewLines == CurrentLine)
				{
					break;
				}

				NewLines++;
				CharactersOnLine = 0;
				continue;
			}
			CharactersOnLine++;
		}

		if (CursorX > CharactersOnLine * GlyphWidth)
		{
			CursorX = CharactersOnLine * GlyphWidth;
		}

		// Cursor
		SDL_SetRenderDrawColor(Renderer, 255, 255, 255, 100);
		SDL_SetRenderDrawBlendMode(Renderer, SDL_BLENDMODE_ADD);
		SDL_Rect CursorRect = { CursorX, CursorY, GlyphWidth, GlyphHeight};
		SDL_RenderFillRect(Renderer, &CursorRect);

		// CursorLine
		SDL_SetRenderDrawColor(Renderer, 255, 0, 0, 50);
		SDL_SetRenderDrawBlendMode(Renderer, SDL_BLENDMODE_ADD);
		SDL_Rect CursorLineRect = { 0, CursorY, WindowWidth, GlyphHeight };
		SDL_RenderFillRect(Renderer, &CursorLineRect);

		SDL_RenderPresent(Renderer);
	}

	TTF_CloseFont(Font);
	SDL_DestroyWindow(Window);
	SDL_Quit();
	return 0;
}
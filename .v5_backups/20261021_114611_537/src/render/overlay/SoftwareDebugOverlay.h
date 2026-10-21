
#pragma once

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <string>

#include "math/Color.h"
#include "render/backends/software/FrameBuffer.h"
#include "render/api/RenderFrame.h"


namespace ve
{
    class SoftwareDebugOverlay {
        private:
            using Glyph = std::array<std::uint8_t, 7>;

            static constexpr int GlyphWidth = 5;
            static constexpr int GlyphHeight = 7;

            static constexpr int GlyphPixelWidth = 8;
            static constexpr int GlyphPixelHeight = 11;

            static constexpr int CharacterAdvance = 10;
            static constexpr int LineAdvance = 15;

            static constexpr int Padding = 8;
            static constexpr int OriginX = 12;
            static constexpr int OriginY = 12;


            static Glyph glyphFor( char character ) {
                const char upper = static_cast<char>(std::toupper(static_cast<unsigned char>(character)));

                switch (upper) {
                    case 'A': return { 14, 17, 17, 31, 17, 17, 17 };
                    case 'B': return { 30, 17, 17, 30, 17, 17, 30 };
                    case 'C': return { 14, 17, 16, 16, 16, 17, 14 };
                    case 'D': return { 30, 17, 17, 17, 17, 17, 30 };
                    case 'E': return { 31, 16, 16, 30, 16, 16, 31 };
                    case 'F': return { 31, 16, 16, 30, 16, 16, 16 };
                    case 'G': return { 14, 17, 16, 23, 17, 17, 14 };
                    case 'H': return { 17, 17, 17, 31, 17, 17, 17 };
                    case 'I': return { 31, 4, 4, 4, 4, 4, 31 };
                    case 'J': return { 7, 2, 2, 2, 18, 18, 12 };
                    case 'K': return { 17, 18, 20, 24, 20, 18, 17 };
                    case 'L': return { 16, 16, 16, 16, 16, 16, 31 };
                    case 'M': return { 17, 27, 21, 21, 17, 17, 17 };
                    case 'N': return { 17, 25, 21, 19, 17, 17, 17 };
                    case 'O': return { 14, 17, 17, 17, 17, 17, 14 };
                    case 'P': return { 30, 17, 17, 30, 16, 16, 16 };
                    case 'Q': return { 14, 17, 17, 17, 21, 18, 13 };
                    case 'R': return { 30, 17, 17, 30, 20, 18, 17 };
                    case 'S': return { 15, 16, 16, 14, 1, 1, 30 };
                    case 'T': return { 31, 4, 4, 4, 4, 4, 4 };
                    case 'U': return { 17, 17, 17, 17, 17, 17, 14 };
                    case 'V': return { 17, 17, 17, 17, 17, 10, 4 };
                    case 'W': return { 17, 17, 17, 21, 21, 21, 10 };
                    case 'X': return { 17, 17, 10, 4, 10, 17, 17 };
                    case 'Y': return { 17, 17, 10, 4, 4, 4, 4 };
                    case 'Z': return { 31, 1, 2, 4, 8, 16, 31 };

                    case '0': return { 14, 17, 19, 21, 25, 17, 14 };
                    case '1': return { 4, 12, 4, 4, 4, 4, 14 };
                    case '2': return { 14, 17, 1, 2, 4, 8, 31 };
                    case '3': return { 30, 1, 1, 14, 1, 1, 30 };
                    case '4': return { 2, 6, 10, 18, 31, 2, 2 };
                    case '5': return { 31, 16, 30, 1, 1, 17, 14 };
                    case '6': return { 6, 8, 16, 30, 17, 17, 14 };
                    case '7': return { 31, 1, 2, 4, 8, 8, 8 };
                    case '8': return { 14, 17, 17, 14, 17, 17, 14 };
                    case '9': return { 14, 17, 17, 15, 1, 2, 12 };

                    case ' ': return { 0, 0, 0, 0, 0, 0, 0 };
                    case ':': return { 0, 4, 4, 0, 4, 4, 0 };
                    case '.': return { 0, 0, 0, 0, 0, 12, 12 };
                    case ',': return { 0, 0, 0, 0, 4, 4, 8 };
                    case '-': return { 0, 0, 0, 31, 0, 0, 0 };
                    case '+': return { 0, 4, 4, 31, 4, 4, 0 };
                    case '/': return { 1, 1, 2, 4, 8, 16, 16 };
                    case '(': return { 2, 4, 8, 8, 8, 4, 2 };
                    case ')': return { 8, 4, 2, 2, 2, 4, 8 };
                    case '[': return { 14, 8, 8, 8, 8, 8, 14 };
                    case ']': return { 14, 2, 2, 2, 2, 2, 14 };
                    case '=': return { 0, 0, 31, 0, 31, 0, 0 };
                    case '_': return { 0, 0, 0, 0, 0, 0, 31 };
                    case '%': return { 17, 2, 4, 4, 8, 17, 0 };
                    case '?': return { 14, 17, 1, 2, 4, 0, 4 };

                    default: return { 14, 17, 1, 2, 4, 0, 4 };
                }
            }


            static void shadePanel( FrameBuffer& frameBuffer, int x, int y, int width, int height ) {
                const int endX = std::min(x + width, frameBuffer.width());
                const int endY = std::min(y + height, frameBuffer.height());

                for (int py = y; py < endY; ++py) {
                    for (int px = x; px < endX; ++px) {
                        const Color32 old = frameBuffer.pixel(px, py);

                        frameBuffer.setPixel(px, py, makeColor(
                            static_cast<std::uint8_t>(getRed(old) * 2 / 5),
                            static_cast<std::uint8_t>(getGreen(old) * 2 / 5),
                            static_cast<std::uint8_t>(getBlue(old) * 2 / 5)
                        ));
                    }
                }
            }


            static void drawCharacter( FrameBuffer& frameBuffer, int x, int y, char character, Color32 color ) {
                const Glyph glyph = glyphFor(character);

                for (int py = 0; py < GlyphPixelHeight; ++py) {
                    const int sourceY = py * GlyphHeight / GlyphPixelHeight;

                    for (int px = 0; px < GlyphPixelWidth; ++px) {
                        const int sourceX = px * GlyphWidth / GlyphPixelWidth;

                        if ((glyph[sourceY] & (1u << (GlyphWidth - sourceX - 1))) == 0)
                            continue;

                        frameBuffer.setPixel(x + px, y + py, color);
                    }
                }
            }


        public:
            static std::array<std::uint8_t, 7> glyphRows( char character ) {
                return glyphFor(character);
}

            static void draw( FrameBuffer& frameBuffer, const DebugOverlaySettings& settings ) {
                if (!settings.enabled || settings.lines.empty())
                    return;

                const int availableWidth = frameBuffer.width() - OriginX - Padding * 2;
                const int availableHeight = frameBuffer.height() - OriginY - Padding * 2;

                if (availableWidth < CharacterAdvance || availableHeight < LineAdvance)
                    return;

                const int maxCharacters = availableWidth / CharacterAdvance;
                const int maxLines = availableHeight / LineAdvance;
                const std::size_t visibleLines = std::min(settings.lines.size(), static_cast<std::size_t>(maxLines));

                std::size_t widestLine = 0;

                for (std::size_t i = 0; i < visibleLines; ++i)
                    widestLine = std::max(widestLine, std::min(settings.lines[i].size(), static_cast<std::size_t>(maxCharacters)));

                if (widestLine == 0)
                    return;

                const int panelWidth = Padding * 2 + static_cast<int>(widestLine) * CharacterAdvance;
                const int panelHeight = Padding * 2 + static_cast<int>(visibleLines - 1) * LineAdvance + GlyphPixelHeight;

                shadePanel(frameBuffer, OriginX, OriginY, panelWidth, panelHeight);

                const Color32 headerColor = makeColor(115, 220, 255);
                const Color32 textColor = makeColor(255, 255, 255);

                for (std::size_t line = 0; line < visibleLines; ++line) {
                    const std::string& text = settings.lines[line];
                    const std::size_t count = std::min(text.size(), static_cast<std::size_t>(maxCharacters));
                    const Color32 color = line == 0 ? headerColor : textColor;

                    for (std::size_t i = 0; i < count; ++i)
                        drawCharacter(frameBuffer, OriginX + Padding + static_cast<int>(i) * CharacterAdvance,
                                      OriginY + Padding + static_cast<int>(line) * LineAdvance, text[i], color);
                }
            }
    };
}

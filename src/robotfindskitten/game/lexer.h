#pragma once

#include <cctype>
#include <iostream>

#include "Windows.h"

// should inludes alls in this order
// clang-format off
#include <cstdint>
#include <string>
#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"
// Lexilla.h should not be included here as it declares statically linked functions without the __declspec( dllexport )
#include "WordList.h"
#include "PropSetSimple.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "LexerModule.h"
#include "LexerBase.h"
#include "DefaultLexer.h"

#include <random>
#include <array>
#include "config.h"
// clang-format on

static inline constexpr std::pair<int, int> PRINTABLE_RANGE{
    static_cast<int>(' '), static_cast<int>('~')};
static inline constexpr int COLOR_COUNT = 255;
static inline constexpr int MY_STYLE_START = 33;

inline std::array<decltype(RGB(0, 0, 0)),
                  PRINTABLE_RANGE.second - PRINTABLE_RANGE.first - 1>
generate_colors() {
    std::random_device rd{};
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distr(0, COLOR_COUNT);
    auto arr = std::array<decltype(RGB(0, 0, 0)),
                          PRINTABLE_RANGE.second - PRINTABLE_RANGE.first - 1>();
    for(auto& i: arr) {
        i = RGB(distr(gen), distr(gen), distr(gen));
    }
    return arr;
}
static auto ALL_COLORS = generate_colors();

static_assert(ALL_COLORS.size()
              == PRINTABLE_RANGE.second - PRINTABLE_RANGE.first - 1);

inline decltype(RGB(0, 0, 0)) get_color(char c) noexcept {
    return ALL_COLORS[c - PRINTABLE_RANGE.first];
};

#define MAX_STR_LEN 100

// clang-format on
class lexer: public Lexilla::DefaultLexer {
public:
    lexer(): DefaultLexer(PROJECT_NAME, static_cast<int>('#')) {}
    void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position length,
                        int initStyle, Scintilla::IDocument* pAccess) override {
        try {
            Lexilla::LexAccessor styler(pAccess);
            Lexilla::StyleContext ctx(startPos, length, initStyle, styler);

            for(; ctx.More(); ctx.Forward()) {
                if(ctx.Match('#', '-') || ctx.Match('-', '#')) {
                    ctx.SetState(STYLE_DEFAULT);
                    ctx.Forward();
                    ctx.SetState(STYLE_DEFAULT);
                } else if(ctx.ch == '#') {
                    ctx.SetState(STYLE_DEFAULT);
                } else {
                    ctx.SetState((ctx.ch) + MY_STYLE_START);
                }
            }
            ctx.Complete();
        } catch(...) {
            // Should not throw into caller as may be compiled with different
            // compiler or options
            pAccess->SetErrorStatus(SC_STATUS_FAILURE);
        }
    }
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    void SCI_METHOD Fold(Sci_PositionU startPos, Sci_Position length,
                         int initStyle,
                         Scintilla::IDocument* pAccess) override {}
    int SCI_METHOD GetIdentifier() override {
        return static_cast<int>('#');
    }
    const char* SCI_METHOD PropertyGet(const char* /*key*/) override {
        return "";
    };
};

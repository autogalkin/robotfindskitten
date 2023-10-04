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

static inline constexpr std::pair<int, int> printable_range{32, 127};
static inline constexpr int color_count = 255;
static inline constexpr int my_style_start = 100;
static const auto ALL_COLORS = []() {
    std::random_device rd{};
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distr(0, color_count);
    auto arr = std::array<decltype(RGB(0, 0, 0)), 127 - 31>();
    for(auto& i: arr) {
        i = RGB(distr(gen), distr(gen), distr(gen));
    }
    return arr;
}();

inline constexpr bool is_valid(char c) noexcept {
    return printable_range.first <= static_cast<int>(c)
           && static_cast<int>(c) < printable_range.second;
}
inline decltype(RGB(0, 0, 0)) get_color(char c) noexcept {
    return ALL_COLORS[c - printable_range.first - 1];
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
                    ctx.SetState((ctx.ch) + my_style_start);
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

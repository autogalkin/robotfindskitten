#pragma once

#include "Windows.h"
#include <cctype>
#include <iostream>
// should inludes alls in this order
// clang-format off
#include <stdint.h>
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
// clang-format on



static const auto ALL_COLORS = []() {
    std::random_device rd{};
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distr(0, 255);
    auto arr = std::array<decltype(RGB(0, 0, 0)), 127 - 31>();
    for (uint8_t a = 31, i = 0; a < 127; ++a, i++) {
        arr[i] = RGB(distr(gen), distr(gen), distr(gen));
    }
    return arr;
}();

inline bool is_valid(char c) { return 31 < int(c) && int(c) < 127; }
inline decltype(RGB(0, 0, 0)) get_color(char c) { return ALL_COLORS[c - 31]; };

#define MAX_STR_LEN 100

// clang-format on
class lexer : public Lexilla::DefaultLexer {
    enum state : int {
        empty = int(' '),
        character = int('#'),
        identifier = -1,
    };
    static_assert(int(' ') == 32);

    Lexilla::WordList is_character_;

  public:
    lexer() : DefaultLexer("robotfindskitten", int('#')) {
    }
    void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position length,
                        int initStyle, Scintilla::IDocument* pAccess) override {
        try {
            Lexilla::LexAccessor styler(pAccess);
            Lexilla::StyleContext ctx(startPos, length, initStyle, styler);
            // TODO

            for (; ctx.More(); ctx.Forward()) {
                if(ctx.ch == '#'){
                    ctx.SetState(STYLE_DEFAULT);
                }
                if(ctx.Match('#', '-') || ctx.Match('-', '#')){
                    ctx.SetState(STYLE_DEFAULT);
                    ctx.Forward();
                    ctx.SetState(STYLE_DEFAULT);
                } else if (ctx.currentLine < 3) {
                    if (ctx.ch == ' '){
                        ctx.SetState(int(ctx.ch) + 100);
                    } else {
                        ctx.SetState(STYLE_DEFAULT);
                    }
                    continue;
                } else {
                    ctx.SetState(int(ctx.ch) + 100);
                }
            }
            ctx.Complete();
        } catch (...) {
            // Should not throw into caller as may be compiled with different
            // compiler or options
            pAccess->SetErrorStatus(SC_STATUS_FAILURE);
        }
    }
    void SCI_METHOD Fold(Sci_PositionU startPos, Sci_Position length,
                         int initStyle,
                         Scintilla::IDocument* pAccess) override {}
    virtual int SCI_METHOD GetIdentifier() override { return 21; }
    virtual const char* SCI_METHOD PropertyGet(const char* key) override {
        return "";
    };
};
